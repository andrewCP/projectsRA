#pragma once

/**
 * @file acs770.h
 * @brief Driver del sensor de corriente Allegro ACS770 para ESP-IDF.
 *
 * Usa el driver ADC propio (adc_driver.h / adc_t) para leer el voltaje de salida
 * del sensor (VIOUT) y convertirlo a corriente en amperios:
 *
 *      I [A] = (V_sensor [mV] - offset [mV]) / sensibilidad [mV/A]
 *
 * donde V_sensor = V_pin_ADC / divider_ratio.
 *
 * IMPORTANTE (hardware): el ACS770 se alimenta a 5 V y su salida puede superar
 * los 3.3 V que tolera el ESP. Es OBLIGATORIO usar un divisor resistivo entre
 * VIOUT y el pin del ADC (ver divider_ratio).
 *
 * Los valores de sensibilidad y offset de los modelos preconfigurados son los
 * TIPICOS del datasheet a Vcc = 5 V. Verifica que coincidan con la variante
 * exacta que compraste.
 */

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "adc_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Variantes del ACS770 (sufijo B = bidireccional, U = unidireccional).
 *
 *  Modelo  | Rango      | Sensibilidad tipica | Offset (Qvo) a Vcc = 5 V
 *  --------|------------|---------------------|--------------------------
 *  050B    | -50..+50 A | 40   mV/A           | 0.5  * Vcc (2.5 V)
 *  100B    | -100..+100 | 20   mV/A           | 0.5  * Vcc (2.5 V)
 *  150B    | -150..+150 | 13.3 mV/A           | 0.5  * Vcc (2.5 V)
 *  200B    | -200..+200 | 10   mV/A           | 0.5  * Vcc (2.5 V)
 *  050U    | 0..50 A    | 60   mV/A           | 0.12 * Vcc (0.6 V)
 *  100U    | 0..100 A   | 30   mV/A           | 0.12 * Vcc (0.6 V)
 *  150U    | 0..150 A   | 20   mV/A           | 0.12 * Vcc (0.6 V)
 *  200U    | 0..200 A   | 15   mV/A           | 0.12 * Vcc (0.6 V)
 */
typedef enum {
    ACS770_MODEL_050B = 0,
    ACS770_MODEL_100B,
    ACS770_MODEL_150B,
    ACS770_MODEL_200B,
    ACS770_MODEL_050U,
    ACS770_MODEL_100U,
    ACS770_MODEL_150U,
    ACS770_MODEL_200U,
    ACS770_MODEL_CUSTOM      /*!< Usa custom_sensitivity_mv_per_a y custom_offset_mv */
} acs770_model_t;

/**
 * @brief Configuracion de inicializacion del sensor.
 */
typedef struct {
    acs770_model_t model;                 /*!< Variante del sensor */
    adc_unit_t     unit;                  /*!< Unidad ADC (recomendado ADC_UNIT_1) */
    adc_channel_t  channel;               /*!< Canal ADC donde llega el divisor */
    adc_atten_t    atten;                 /*!< Atenuacion (ADC_ATTEN_DB_12 => ~0..3.1 V) */
    adc_bitwidth_t bitwidth;              /*!< Resolucion (ADC_BITWIDTH_DEFAULT recomendado) */

    float    vcc_mv;                      /*!< Alimentacion real del sensor en mV. 0 => 5000 */
    float    divider_ratio;               /*!< V_pin / V_sensor = R2 / (R1 + R2), en (0, 1]. 0 => 1.0 (sin divisor) */
    uint16_t samples;                     /*!< Muestras a promediar por lectura. 0 => 64 */

    float custom_sensitivity_mv_per_a;    /*!< Solo ACS770_MODEL_CUSTOM: sensibilidad en mV/A */
    float custom_offset_mv;               /*!< Solo ACS770_MODEL_CUSTOM: voltaje a 0 A en mV */
} acs770_config_t;

/**
 * @brief Instancia del sensor (estado interno). No modificar campos a mano.
 */
typedef struct {
    adc_t          adc;                   /*!< Driver ADC subyacente */
    acs770_model_t model;

    float    vcc_mv;
    float    divider_ratio;
    uint16_t samples;

    float    offset_mv;                   /*!< Voltaje del sensor a 0 A (en la salida VIOUT) */
    float    sensitivity_mv_per_a;        /*!< Sensibilidad activa */
    float    nominal_offset_mv;           /*!< Offset teorico (solo para diagnostico) */
    float    nominal_sensitivity_mv_per_a;/*!< Sensibilidad teorica (solo para diagnostico) */

    bool     initialized;
    bool     offset_calibrated;
    bool     sensitivity_calibrated;
} acs770_t;

/**
 * @brief Inicializa el ADC (adc_init + adc_calibrate) y carga los valores tipicos del modelo.
 * @return ESP_OK, ESP_ERR_INVALID_ARG o el error del driver ADC.
 */
esp_err_t acs770_init(acs770_t *self, const acs770_config_t *cfg);

/**
 * @brief Lee el voltaje de salida del sensor (VIOUT), ya compensado por el divisor.
 * @param sensor_mv Voltaje promedio en mV en la salida del sensor.
 */
esp_err_t acs770_read_voltage_mv(acs770_t *self, float *sensor_mv);

/**
 * @brief Lee la corriente en amperios (promedio de cfg.samples muestras).
 *        Positiva cuando la corriente circula de IP+ a IP- (ver datasheet).
 */
esp_err_t acs770_read_current(acs770_t *self, float *current_a);

/**
 * @brief Calibra el OFFSET midiendo el voltaje con 0 A circulando por el sensor.
 * @param samples Muestras a promediar. 0 => 256.
 */
esp_err_t acs770_calibrate_offset(acs770_t *self, uint16_t samples);

/**
 * @brief Calibra la SENSIBILIDAD con una corriente conocida circulando por el sensor.
 *        Debe llamarse DESPUES de tener el offset correcto (nominal o calibrado).
 *        Tambien compensa la tolerancia de las resistencias del divisor.
 * @param known_current_a Corriente de referencia en A (>= 0.5 A en valor absoluto; ideal 25..50 % del rango).
 * @param samples         Muestras a promediar. 0 => 256.
 */
esp_err_t acs770_calibrate_sensitivity(acs770_t *self, float known_current_a, uint16_t samples);

/** @brief Fija el offset manualmente (mV en VIOUT), p. ej. al cargarlo desde NVS. */
esp_err_t acs770_set_offset_mv(acs770_t *self, float offset_mv);

/** @brief Fija la sensibilidad manualmente (mV/A), p. ej. al cargarla desde NVS. */
esp_err_t acs770_set_sensitivity(acs770_t *self, float sensitivity_mv_per_a);

/** @brief Offset activo en mV (0 si self es NULL). */
float acs770_get_offset_mv(const acs770_t *self);

/** @brief Sensibilidad activa en mV/A (0 si self es NULL). */
float acs770_get_sensitivity(const acs770_t *self);

/** @brief Libera el ADC. */
esp_err_t acs770_deinit(acs770_t *self);

#ifdef __cplusplus
}
#endif