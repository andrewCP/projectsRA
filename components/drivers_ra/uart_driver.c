/**
 * @file uart_driver.c
 * @brief Implementación del driver UART (capa Hardware)
 */

#include "uart_driver.h"

#include <inttypes.h>
#include "esp_log.h"

static const char *TAG = "uart_driver";

esp_err_t uart_driver_init(uart_driver_t *drv, const uart_driver_config_t *config)
{
    if (drv == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uart_config_t idf_config = {
        .baud_rate = (int)config->baudrate,
        .data_bits = config->data_bits,
        .parity    = config->parity,
        .stop_bits = config->stop_bits,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    /* Habilita control de flujo por hardware si se indicaron pines RTS/CTS */
    if (config->rts_pin != UART_PIN_NO_CHANGE || config->cts_pin != UART_PIN_NO_CHANGE) {
        idf_config.flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS;
        idf_config.rx_flow_ctrl_thresh = 122;
    }

    esp_err_t err = uart_param_config(config->port, &idf_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_set_pin(config->port, config->tx_pin, config->rx_pin,
                        config->rts_pin, config->cts_pin);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(err));
        return err;
    }

    QueueHandle_t queue_handle = NULL;
    err = uart_driver_install(config->port,
                               config->rx_buffer_size,
                               config->tx_buffer_size,
                               config->event_queue_size,
                               config->event_queue_size > 0 ? &queue_handle : NULL,
                               0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    drv->port = config->port;
    drv->event_queue = queue_handle;
    drv->initialized = true;

    ESP_LOGI(TAG, "UART%d inicializado a %" PRIu32 " baudios", config->port, config->baudrate);
    return ESP_OK;
}

esp_err_t uart_driver_deinit(uart_driver_t *drv)
{
    if (drv == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!drv->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = uart_driver_delete(drv->port);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_delete failed: %s", esp_err_to_name(err));
        return err;
    }

    drv->event_queue = NULL;
    drv->initialized = false;
    return ESP_OK;
}

esp_err_t uart_driver_write(uart_driver_t *drv, const uint8_t *data, size_t len, size_t *bytes_written)
{
    if (drv == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!drv->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    int written = uart_write_bytes(drv->port, (const char *)data, len);
    if (written < 0) {
        return ESP_FAIL;
    }

    if (bytes_written != NULL) {
        *bytes_written = (size_t)written;
    }
    return ESP_OK;
}

esp_err_t uart_driver_read(uart_driver_t *drv, uint8_t *data, size_t len,
                            size_t *bytes_read, TickType_t timeout_ticks)
{
    if (drv == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!drv->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    int read = uart_read_bytes(drv->port, data, len, timeout_ticks);
    if (read < 0) {
        return ESP_FAIL;
    }

    if (bytes_read != NULL) {
        *bytes_read = (size_t)read;
    }
    return ESP_OK;
}

esp_err_t uart_driver_available(uart_driver_t *drv, size_t *available)
{
    if (drv == NULL || available == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!drv->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    size_t buffered_len = 0;
    esp_err_t err = uart_get_buffered_data_len(drv->port, &buffered_len);
    if (err != ESP_OK) {
        return err;
    }

    *available = buffered_len;
    return ESP_OK;
}

esp_err_t uart_driver_flush(uart_driver_t *drv, bool flush_tx, bool flush_rx)
{
    if (drv == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!drv->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = ESP_OK;

    if (flush_tx) {
        err = uart_wait_tx_done(drv->port, portMAX_DELAY);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "uart_wait_tx_done failed: %s", esp_err_to_name(err));
            return err;
        }
    }

    if (flush_rx) {
        err = uart_flush_input(drv->port);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "uart_flush_input failed: %s", esp_err_to_name(err));
            return err;
        }
    }

    return ESP_OK;
}
