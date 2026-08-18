
#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    adc_oneshot_unit_handle_t unit_handle;   /*!< Handle de la unidad ADC (driver oneshot) */
    adc_cali_handle_t         cali_handle;   /*!< Handle del esquema de calibración */
    adc_unit_t                unit;          /*!< Unidad ADC (ADC_UNIT_1 / ADC_UNIT_2) */
    adc_channel_t             channel;       /*!< Canal ADC dentro de la unidad */
    adc_atten_t                atten;         /*!< Atenuación configurada */
    adc_bitwidth_t             bitwidth;      /*!< Resolución configurada (bits) */
    bool                       is_calibrated; /*!< true si adc_calibrate() tuvo éxito */
} adc_t;


esp_err_t adc_init(adc_t *self,
                    adc_unit_t unit,
                    adc_channel_t channel,
                    adc_atten_t atten,
                    adc_bitwidth_t bitwidth);
esp_err_t adc_calibrate(adc_t *self);
esp_err_t adc_read_raw(adc_t *self, int *raw_value);

esp_err_t adc_read_voltage(adc_t *self, int *voltage_mv);


esp_err_t adc_deinit(adc_t *self);

#ifdef __cplusplus
}
#endif