#include "acs770.h"

#include <math.h>
#include <stddef.h>
#include <string.h>
#include "esp_log.h"

static const char *TAG = "acs770";

/* ------------------------------------------------------------------------ */
/* Constantes                                                               */
/* ------------------------------------------------------------------------ */

#define ACS770_NOMINAL_VCC_MV        5000.0f  /* Vcc al que estan dadas las specs del datasheet */
#define ACS770_DEFAULT_SAMPLES       64
#define ACS770_DEFAULT_CAL_SAMPLES   256
#define ACS770_ADC_SAFE_LIMIT_MV     3100.0f  /* Tope aprox. del ADC con ADC_ATTEN_DB_12 */
#define ACS770_MIN_SENSITIVITY       0.001f   /* mV/A: por debajo se considera invalido */
#define ACS770_MIN_CAL_CURRENT_A     0.5f     /* corriente minima para calibrar sensibilidad */
#define ACS770_OFFSET_WARN_FRACTION  0.10f    /* aviso si el offset se aleja >10 % de Vcc del teorico */
#define ACS770_SENS_WARN_FRACTION    0.25f    /* aviso si la sensibilidad se aleja >25 % de la teorica */

/* Valores tipicos por modelo a Vcc = 5 V (ratiometricos respecto a Vcc). */
typedef struct {
    float sensitivity_mv_per_a;  /* mV/A a Vcc = 5 V */
    float offset_ratio;          /* Qvo / Vcc */
} acs770_preset_t;

static const acs770_preset_t s_presets[] = {
    [ACS770_MODEL_050B] = { 40.0f, 0.50f },
    [ACS770_MODEL_100B] = { 20.0f, 0.50f },
    [ACS770_MODEL_150B] = { 13.3f, 0.50f },
    [ACS770_MODEL_200B] = { 10.0f, 0.50f },
    [ACS770_MODEL_050U] = { 60.0f, 0.12f },
    [ACS770_MODEL_100U] = { 30.0f, 0.12f },
    [ACS770_MODEL_150U] = { 20.0f, 0.12f },
    [ACS770_MODEL_200U] = { 15.0f, 0.12f },
};

/* ------------------------------------------------------------------------ */
/* Funciones internas                                                       */
/* ------------------------------------------------------------------------ */

/**
 * Verifica que la instancia sea valida y este inicializada.
 */
static esp_err_t acs770_check(const acs770_t *self)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!self->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

/**
 * Promedia 'samples' lecturas de voltaje en el PIN del ADC (mV, ya calibrado).
 */
static esp_err_t acs770_read_avg_pin_mv(acs770_t *self, uint16_t samples, float *pin_mv)
{
    if (samples == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    int64_t sum = 0;
    for (uint16_t i = 0; i < samples; i++) {
        int mv = 0;
        esp_err_t ret = adc_read_voltage(&self->adc, &mv);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error leyendo voltaje del ADC: %s", esp_err_to_name(ret));
            return ret;
        }
        sum += mv;
    }

    *pin_mv = (float)sum / (float)samples;
    return ESP_OK;
}

/**
 * Promedia 'samples' lecturas y compensa el divisor: devuelve el voltaje en VIOUT (mV).
 */
static esp_err_t acs770_read_avg_sensor_mv(acs770_t *self, uint16_t samples, float *sensor_mv)
{
    float pin_mv = 0.0f;
    esp_err_t ret = acs770_read_avg_pin_mv(self, samples, &pin_mv);
    if (ret != ESP_OK) {
        return ret;
    }

    *sensor_mv = pin_mv / self->divider_ratio;
    return ESP_OK;
}

/* ------------------------------------------------------------------------ */
/* API publica                                                              */
/* ------------------------------------------------------------------------ */

esp_err_t acs770_init(acs770_t *self, const acs770_config_t *cfg)
{
    if (self == NULL || cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if ((int)cfg->model < 0 || cfg->model > ACS770_MODEL_CUSTOM) {
        ESP_LOGE(TAG, "Modelo de sensor invalido: %d", (int)cfg->model);
        return ESP_ERR_INVALID_ARG;
    }

    memset(self, 0, sizeof(*self));

    /* --- Parametros generales con valores por defecto --- */
    self->model         = cfg->model;
    self->vcc_mv        = (cfg->vcc_mv > 0.0f)        ? cfg->vcc_mv        : ACS770_NOMINAL_VCC_MV;
    self->divider_ratio = (cfg->divider_ratio > 0.0f) ? cfg->divider_ratio : 1.0f;
    self->samples       = (cfg->samples > 0)          ? cfg->samples       : ACS770_DEFAULT_SAMPLES;

    if (self->divider_ratio > 1.0f) {
        ESP_LOGE(TAG, "divider_ratio debe estar en (0, 1], recibido %.3f", self->divider_ratio);
        return ESP_ERR_INVALID_ARG;
    }

    /* --- Valores teoricos de sensibilidad y offset --- */
    if (cfg->model == ACS770_MODEL_CUSTOM) {
        if (fabsf(cfg->custom_sensitivity_mv_per_a) < ACS770_MIN_SENSITIVITY) {
            ESP_LOGE(TAG, "custom_sensitivity_mv_per_a invalida");
            return ESP_ERR_INVALID_ARG;
        }
        self->sensitivity_mv_per_a = cfg->custom_sensitivity_mv_per_a;
        self->offset_mv            = cfg->custom_offset_mv;
    } else {
        /* Ratiometrico: sensibilidad y offset escalan con Vcc/5V */
        const acs770_preset_t *p = &s_presets[cfg->model];
        float scale = self->vcc_mv / ACS770_NOMINAL_VCC_MV;
        self->sensitivity_mv_per_a = p->sensitivity_mv_per_a * scale;
        self->offset_mv            = p->offset_ratio * self->vcc_mv;
    }
    self->nominal_sensitivity_mv_per_a = self->sensitivity_mv_per_a;
    self->nominal_offset_mv            = self->offset_mv;

    /* --- Aviso si el voltaje maximo posible en el pin podria pasar el rango del ADC --- */
    if (self->vcc_mv * self->divider_ratio > ACS770_ADC_SAFE_LIMIT_MV) {
        ESP_LOGW(TAG,
                 "Con Vcc=%.0f mV y divider_ratio=%.3f la salida podria llegar a %.0f mV en el pin "
                 "(> %.0f mV). Revisa el divisor para no saturar/danar el ADC",
                 self->vcc_mv, self->divider_ratio,
                 self->vcc_mv * self->divider_ratio, ACS770_ADC_SAFE_LIMIT_MV);
    }

    /* --- ADC: init + calibracion de fabrica --- */
    esp_err_t ret = adc_init(&self->adc, cfg->unit, cfg->channel, cfg->atten, cfg->bitwidth);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = adc_calibrate(&self->adc);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "El ADC no se pudo calibrar (%s); no se puede leer voltaje en mV",
                 esp_err_to_name(ret));
        adc_deinit(&self->adc);
        return ret;
    }

    self->initialized = true;

    ESP_LOGI(TAG, "ACS770 listo: modelo=%d, Vcc=%.0f mV, divisor=%.3f, sens=%.2f mV/A, offset=%.1f mV, muestras=%u",
             (int)self->model, self->vcc_mv, self->divider_ratio,
             self->sensitivity_mv_per_a, self->offset_mv, (unsigned)self->samples);

    return ESP_OK;
}

esp_err_t acs770_read_voltage_mv(acs770_t *self, float *sensor_mv)
{
    if (sensor_mv == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = acs770_check(self);
    if (ret != ESP_OK) {
        return ret;
    }

    return acs770_read_avg_sensor_mv(self, self->samples, sensor_mv);
}

esp_err_t acs770_read_current(acs770_t *self, float *current_a)
{
    if (current_a == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = acs770_check(self);
    if (ret != ESP_OK) {
        return ret;
    }

    float sensor_mv = 0.0f;
    ret = acs770_read_avg_sensor_mv(self, self->samples, &sensor_mv);
    if (ret != ESP_OK) {
        return ret;
    }

    *current_a = (sensor_mv - self->offset_mv) / self->sensitivity_mv_per_a;
    return ESP_OK;
}

esp_err_t acs770_calibrate_offset(acs770_t *self, uint16_t samples)
{
    esp_err_t ret = acs770_check(self);
    if (ret != ESP_OK) {
        return ret;
    }
    if (samples == 0) {
        samples = ACS770_DEFAULT_CAL_SAMPLES;
    }

    /* Con 0 A por el sensor, el voltaje medido ES el offset. */
    float sensor_mv = 0.0f;
    ret = acs770_read_avg_sensor_mv(self, samples, &sensor_mv);
    if (ret != ESP_OK) {
        return ret;
    }

    if (fabsf(sensor_mv - self->nominal_offset_mv) > ACS770_OFFSET_WARN_FRACTION * self->vcc_mv) {
        ESP_LOGW(TAG,
                 "Offset medido (%.1f mV) muy lejos del teorico (%.1f mV): "
                 "verifica que no circule corriente, el divisor, la alimentacion y el modelo elegido",
                 sensor_mv, self->nominal_offset_mv);
    }

    self->offset_mv         = sensor_mv;
    self->offset_calibrated = true;

    ESP_LOGI(TAG, "Offset calibrado: %.2f mV (teorico %.2f mV)", self->offset_mv, self->nominal_offset_mv);
    return ESP_OK;
}

esp_err_t acs770_calibrate_sensitivity(acs770_t *self, float known_current_a, uint16_t samples)
{
    esp_err_t ret = acs770_check(self);
    if (ret != ESP_OK) {
        return ret;
    }
    if (fabsf(known_current_a) < ACS770_MIN_CAL_CURRENT_A) {
        ESP_LOGE(TAG, "Corriente de referencia demasiado baja (%.3f A); usa >= %.1f A",
                 known_current_a, ACS770_MIN_CAL_CURRENT_A);
        return ESP_ERR_INVALID_ARG;
    }
    if (samples == 0) {
        samples = ACS770_DEFAULT_CAL_SAMPLES;
    }

    float sensor_mv = 0.0f;
    ret = acs770_read_avg_sensor_mv(self, samples, &sensor_mv);
    if (ret != ESP_OK) {
        return ret;
    }

    /* sensibilidad = (V - V_offset) / I */
    float sens = (sensor_mv - self->offset_mv) / known_current_a;

    if (fabsf(sens) < ACS770_MIN_SENSITIVITY) {
        ESP_LOGE(TAG, "Sensibilidad calculada invalida (%.4f mV/A): no se detecta corriente. "
                      "Revisa conexion de IP+/IP- y la corriente aplicada", sens);
        return ESP_ERR_INVALID_RESPONSE;
    }
    if (sens < 0.0f) {
        ESP_LOGW(TAG, "Sensibilidad negativa (%.2f mV/A): el sensor parece conectado al reves "
                      "(IP+ / IP- invertidos). Se acepta, pero conviene corregir el cableado", sens);
    }
    if (fabsf(fabsf(sens) - fabsf(self->nominal_sensitivity_mv_per_a)) >
        ACS770_SENS_WARN_FRACTION * fabsf(self->nominal_sensitivity_mv_per_a)) {
        ESP_LOGW(TAG, "Sensibilidad medida (%.2f mV/A) muy lejos de la teorica (%.2f mV/A): "
                      "verifica modelo, divisor y corriente de referencia",
                 sens, self->nominal_sensitivity_mv_per_a);
    }

    self->sensitivity_mv_per_a   = sens;
    self->sensitivity_calibrated = true;

    ESP_LOGI(TAG, "Sensibilidad calibrada: %.3f mV/A (teorica %.3f mV/A)",
             self->sensitivity_mv_per_a, self->nominal_sensitivity_mv_per_a);
    return ESP_OK;
}

esp_err_t acs770_set_offset_mv(acs770_t *self, float offset_mv)
{
    esp_err_t ret = acs770_check(self);
    if (ret != ESP_OK) {
        return ret;
    }

    self->offset_mv         = offset_mv;
    self->offset_calibrated = true;
    return ESP_OK;
}

esp_err_t acs770_set_sensitivity(acs770_t *self, float sensitivity_mv_per_a)
{
    esp_err_t ret = acs770_check(self);
    if (ret != ESP_OK) {
        return ret;
    }
    if (fabsf(sensitivity_mv_per_a) < ACS770_MIN_SENSITIVITY) {
        return ESP_ERR_INVALID_ARG;
    }

    self->sensitivity_mv_per_a   = sensitivity_mv_per_a;
    self->sensitivity_calibrated = true;
    return ESP_OK;
}

float acs770_get_offset_mv(const acs770_t *self)
{
    return (self != NULL) ? self->offset_mv : 0.0f;
}

float acs770_get_sensitivity(const acs770_t *self)
{
    return (self != NULL) ? self->sensitivity_mv_per_a : 0.0f;
}

esp_err_t acs770_deinit(acs770_t *self)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_OK;
    if (self->initialized) {
        ret = adc_deinit(&self->adc);
    }

    self->initialized            = false;
    self->offset_calibrated      = false;
    self->sensitivity_calibrated = false;
    return ret;
}