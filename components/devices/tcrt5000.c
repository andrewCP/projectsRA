/**
 * @file tcrt5000.c
 * @brief Implementacion de la capa Device para el sensor TCRT5000.
 */

#include "tcrt5000.h"
#include "esp_check.h"

static const char *TAG = "tcrt5000";

esp_err_t tcrt5000_init(tcrt5000_t *dev, const tcrt5000_config_t *config)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev es NULL");
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config es NULL");

    pcnt_config_t pcnt_cfg = {
        .gpio_pulse_num = config->gpio_out,
        .gpio_ctrl_num = -1,          /* TCRT5000 no usa GPIO de control/gate */
        .low_limit = -1,              /* solo contamos en sentido positivo */
        .high_limit = 32767,          /* maximo permitido (16 bits con signo) */
        .glitch_filter_ns = config->glitch_filter_ns,
        .count_on_rising = true,      /* contar cuando el objeto interrumpe el haz (flanco de subida) */
    };

    return pcnt_init(&dev->pcnt, &pcnt_cfg);
}

esp_err_t tcrt5000_start(tcrt5000_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev es NULL");
    return pcnt_start(&dev->pcnt);
}

esp_err_t tcrt5000_stop(tcrt5000_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev es NULL");
    return pcnt_stop(&dev->pcnt);
}

esp_err_t tcrt5000_get_count(tcrt5000_t *dev, int *count)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev es NULL");
    return pcnt_get_count(&dev->pcnt, count);
}

esp_err_t tcrt5000_clear(tcrt5000_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev es NULL");
    return pcnt_clear(&dev->pcnt);
}
