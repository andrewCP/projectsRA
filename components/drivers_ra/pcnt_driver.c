/**
 * @file pcnt_driver.c
 * @brief Implementacion del driver PCNT (ver pcnt_driver.h).
 */

#include "pcnt_driver.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "pcnt_driver";

esp_err_t pcnt_init(pcnt_t *dev, const pcnt_config_t *config)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev es NULL");
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config es NULL");
    ESP_RETURN_ON_FALSE(config->gpio_pulse_num >= 0, ESP_ERR_INVALID_ARG, TAG, "gpio_pulse_num invalido");
    ESP_RETURN_ON_FALSE(!dev->initialized, ESP_ERR_INVALID_STATE, TAG, "la instancia ya fue inicializada");

    esp_err_t ret = ESP_OK;

    /* 1) Crear la unidad PCNT con los limites de conteo */
    pcnt_unit_config_t unit_config = {
        .low_limit = config->low_limit,
        .high_limit = config->high_limit,
        .flags.accum_count = true, /* acumula al cruzar los limites, evita perder pulsos */
    };
    ESP_RETURN_ON_ERROR(pcnt_new_unit(&unit_config, &dev->unit_handle), TAG,
                         "fallo al crear la unidad PCNT");

    /* 2) Configurar filtro anti-glitch (opcional) */
    if (config->glitch_filter_ns > 0) {
        pcnt_glitch_filter_config_t filter_config = {
            .max_glitch_ns = config->glitch_filter_ns,
        };
        ret = pcnt_unit_set_glitch_filter(dev->unit_handle, &filter_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "fallo al configurar el filtro anti-glitch");
            goto cleanup_unit;
        }
    }

    /* 3) Crear el canal PCNT asociado al GPIO de pulso (y opcionalmente al de control) */
    pcnt_chan_config_t chan_config = {
        .edge_gpio_num = config->gpio_pulse_num,
        .level_gpio_num = config->gpio_ctrl_num, /* -1 si no se usa */
    };
    ret = pcnt_new_channel(dev->unit_handle, &chan_config, &dev->channel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "fallo al crear el canal PCNT");
        goto cleanup_unit;
    }

    /* 4) Definir el modo de conteo (flanco de subida o de bajada) */
    if (config->count_on_rising) {
        ret = pcnt_channel_set_edge_action(dev->channel_handle,
                                            PCNT_CHANNEL_EDGE_ACTION_INCREASE,  /* flanco subida -> +1 */
                                            PCNT_CHANNEL_EDGE_ACTION_HOLD);     /* flanco bajada -> sin cambio */
    } else {
        ret = pcnt_channel_set_edge_action(dev->channel_handle,
                                            PCNT_CHANNEL_EDGE_ACTION_HOLD,      /* flanco subida -> sin cambio */
                                            PCNT_CHANNEL_EDGE_ACTION_INCREASE); /* flanco bajada -> +1 */
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "fallo al configurar el modo de conteo (edge action)");
        goto cleanup_channel;
    }

    /* Si no se usa GPIO de control, el nivel se ignora (KEEP en ambos niveles) */
    ret = pcnt_channel_set_level_action(dev->channel_handle,
                                         PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                         PCNT_CHANNEL_LEVEL_ACTION_KEEP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "fallo al configurar la accion de nivel");
        goto cleanup_channel;
    }

    /* 5) Registrar puntos de vigilancia en los limites para permitir el wrap-around */
    (void)pcnt_unit_add_watch_point(dev->unit_handle, config->high_limit);
    (void)pcnt_unit_add_watch_point(dev->unit_handle, config->low_limit);

    /* 6) Habilitar la unidad (queda lista pero detenida) */
    ret = pcnt_unit_enable(dev->unit_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "fallo al habilitar la unidad PCNT");
        goto cleanup_channel;
    }

    ret = pcnt_unit_clear_count(dev->unit_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "fallo al limpiar el contador inicial");
        goto cleanup_enable;
    }

    dev->initialized = true;
    dev->running = false;
    ESP_LOGI(TAG, "PCNT inicializado en GPIO %d (ctrl=%d)", config->gpio_pulse_num, config->gpio_ctrl_num);
    return ESP_OK;

cleanup_enable:
    pcnt_unit_disable(dev->unit_handle);
cleanup_channel:
    pcnt_del_channel(dev->channel_handle);
    dev->channel_handle = NULL;
cleanup_unit:
    pcnt_del_unit(dev->unit_handle);
    dev->unit_handle = NULL;
    return ret;
}

esp_err_t pcnt_start(pcnt_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev es NULL");
    ESP_RETURN_ON_FALSE(dev->initialized, ESP_ERR_INVALID_STATE, TAG, "la instancia no fue inicializada");

    if (dev->running) {
        return ESP_OK; /* ya estaba corriendo, operacion idempotente */
    }

    esp_err_t ret = pcnt_unit_start(dev->unit_handle);
    if (ret == ESP_OK) {
        dev->running = true;
    }
    return ret;
}

esp_err_t pcnt_stop(pcnt_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev es NULL");
    ESP_RETURN_ON_FALSE(dev->initialized, ESP_ERR_INVALID_STATE, TAG, "la instancia no fue inicializada");

    if (!dev->running) {
        return ESP_OK; /* ya estaba detenido, operacion idempotente */
    }

    esp_err_t ret = pcnt_unit_stop(dev->unit_handle);
    if (ret == ESP_OK) {
        dev->running = false;
    }
    return ret;
}

esp_err_t pcnt_get_count(pcnt_t *dev, int *count)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev es NULL");
    ESP_RETURN_ON_FALSE(count != NULL, ESP_ERR_INVALID_ARG, TAG, "count es NULL");
    ESP_RETURN_ON_FALSE(dev->initialized, ESP_ERR_INVALID_STATE, TAG, "la instancia no fue inicializada");

    return pcnt_unit_get_count(dev->unit_handle, count);
}

esp_err_t pcnt_clear(pcnt_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev es NULL");
    ESP_RETURN_ON_FALSE(dev->initialized, ESP_ERR_INVALID_STATE, TAG, "la instancia no fue inicializada");

    return pcnt_unit_clear_count(dev->unit_handle);
}

esp_err_t pcnt_deinit(pcnt_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev es NULL");

    if (!dev->initialized) {
        return ESP_OK;
    }

    esp_err_t ret = ESP_OK;

    if (dev->running) {
        ret = pcnt_unit_stop(dev->unit_handle);
        dev->running = false;
    }

    esp_err_t err_disable = pcnt_unit_disable(dev->unit_handle);
    esp_err_t err_del_chan = pcnt_del_channel(dev->channel_handle);
    esp_err_t err_del_unit = pcnt_del_unit(dev->unit_handle);

    dev->channel_handle = NULL;
    dev->unit_handle = NULL;
    dev->initialized = false;

    if (ret != ESP_OK) return ret;
    if (err_disable != ESP_OK) return err_disable;
    if (err_del_chan != ESP_OK) return err_del_chan;
    return err_del_unit;
}
