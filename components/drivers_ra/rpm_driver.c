
rpm_driver.c

#include "rpm_driver.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "speed_service";

esp_err_t speed_service_init(speed_service_t *self, pcnt_t *pcnt, uint32_t ppr)
{
    ESP_RETURN_ON_FALSE(self != NULL, ESP_ERR_INVALID_ARG, TAG, "self es NULL");
    ESP_RETURN_ON_FALSE(pcnt != NULL, ESP_ERR_INVALID_ARG, TAG, "pcnt es NULL");
    ESP_RETURN_ON_FALSE(ppr > 0, ESP_ERR_INVALID_ARG, TAG, "ppr debe ser > 0");

    self->pcnt        = pcnt;
    self->ppr         = ppr;
    self->last_count  = 0;
    self->t_last_us   = 0;
    self->initialized = true;
    return ESP_OK;
}

esp_err_t speed_service_start(speed_service_t *self)
{
    ESP_RETURN_ON_FALSE(self != NULL, ESP_ERR_INVALID_ARG, TAG, "self es NULL");
    ESP_RETURN_ON_FALSE(self->initialized, ESP_ERR_INVALID_STATE, TAG, "no inicializado");

    ESP_RETURN_ON_ERROR(pcnt_clear(self->pcnt), TAG, "fallo al limpiar el contador");
    self->last_count = 0;
    self->t_last_us  = esp_timer_get_time();
    return pcnt_start(self->pcnt);
}

esp_err_t speed_service_stop(speed_service_t *self)
{
    ESP_RETURN_ON_FALSE(self != NULL, ESP_ERR_INVALID_ARG, TAG, "self es NULL");
    ESP_RETURN_ON_FALSE(self->initialized, ESP_ERR_INVALID_STATE, TAG, "no inicializado");

    return pcnt_stop(self->pcnt);
}

esp_err_t speed_service_get_rpm(speed_service_t *self, float *rpm)
{
    ESP_RETURN_ON_FALSE(self != NULL, ESP_ERR_INVALID_ARG, TAG, "self es NULL");
    ESP_RETURN_ON_FALSE(rpm != NULL, ESP_ERR_INVALID_ARG, TAG, "rpm es NULL");
    ESP_RETURN_ON_FALSE(self->initialized, ESP_ERR_INVALID_STATE, TAG, "no inicializado");

    int count = 0;
    ESP_RETURN_ON_ERROR(pcnt_get_count(self->pcnt, &count), TAG, "fallo al leer el conteo");
    int64_t now = esp_timer_get_time();

    float dt_s  = (float)(now - self->t_last_us) / 1e6f;
    int   delta = count - self->last_count;

    /* Actualizar referencia siempre, para no arrastrar errores */
    self->last_count = count;
    self->t_last_us  = now;

    ESP_RETURN_ON_FALSE(dt_s > 0.0f, ESP_ERR_INVALID_STATE, TAG, "dt invalido");
    ESP_RETURN_ON_FALSE(delta >= 0, ESP_ERR_INVALID_STATE, TAG, "conteo reiniciado externamente");

    *rpm = ((float)delta / (float)self->ppr) * 60.0f / dt_s;
    return ESP_OK;
}