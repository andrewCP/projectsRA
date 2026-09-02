#include "adc_driver.h"
#include "esp_log.h"

static const char *TAG = "adc_driver";
static esp_err_t adc_driver_create_cali_scheme(adc_t *self)
{
    esp_err_t ret = ESP_ERR_NOT_SUPPORTED;
    self->cali_handle = NULL;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (ret != ESP_OK) {
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = self->unit,
            .chan = self->channel,
            .atten = self->atten,
            .bitwidth = self->bitwidth,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &self->cali_handle);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Calibracion creada con esquema Curve Fitting");
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (ret != ESP_OK) {
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = self->unit,
            .atten = self->atten,
            .bitwidth = self->bitwidth,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &self->cali_handle);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Calibracion creada con esquema Line Fitting");
        }
    }
#endif

    return ret;
}

/* ------------------------------------------------------------------------ */
/* API pública                                                              */
/* ------------------------------------------------------------------------ */

esp_err_t adc_init(adc_t *self,
                    adc_unit_t unit,
                    adc_channel_t channel,
                    adc_atten_t atten,
                    adc_bitwidth_t bitwidth)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    self->unit_handle   = NULL;
    self->cali_handle   = NULL;
    self->unit          = unit;
    self->channel       = channel;
    self->atten         = atten;
    self->bitwidth      = bitwidth;
    self->is_calibrated = false;

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = unit,
    };

    esp_err_t ret = adc_oneshot_new_unit(&unit_cfg, &self->unit_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error creando unidad ADC: %s", esp_err_to_name(ret));
        return ret;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = atten,
        .bitwidth = bitwidth,
    };

    ret = adc_oneshot_config_channel(self->unit_handle, channel, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error configurando canal ADC: %s", esp_err_to_name(ret));
        adc_oneshot_del_unit(self->unit_handle);
        self->unit_handle = NULL;
        return ret;
    }

    ESP_LOGI(TAG, "ADC inicializado (unit=%d, channel=%d, atten=%d, bitwidth=%d)",
             unit, channel, atten, bitwidth);

    return ESP_OK;
}

esp_err_t adc_calibrate(adc_t *self)
{
    if (self == NULL || self->unit_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = adc_driver_create_cali_scheme(self);
    self->is_calibrated = (ret == ESP_OK);

    if (!self->is_calibrated) {
        ESP_LOGW(TAG, "No fue posible calibrar el ADC: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t adc_read_raw(adc_t *self, int *raw_value)
{
    if (self == NULL || raw_value == NULL || self->unit_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return adc_oneshot_read(self->unit_handle, self->channel, raw_value);
}

esp_err_t adc_read_voltage(adc_t *self, int *voltage_mv)
{
    if (self == NULL || voltage_mv == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!self->is_calibrated || self->cali_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    int raw_value = 0;
    esp_err_t ret = adc_read_raw(self, &raw_value);
    if (ret != ESP_OK) {
        return ret;
    }

    return adc_cali_raw_to_voltage(self->cali_handle, raw_value, voltage_mv);
}

esp_err_t adc_deinit(adc_t *self)
{
    if (self == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_OK;

    if (self->cali_handle != NULL) {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        adc_cali_delete_scheme_curve_fitting(self->cali_handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        adc_cali_delete_scheme_line_fitting(self->cali_handle);
#endif
        self->cali_handle = NULL;
        self->is_calibrated = false;
    }

    if (self->unit_handle != NULL) {
        ret = adc_oneshot_del_unit(self->unit_handle);
        self->unit_handle = NULL;
    }

    return ret;
}