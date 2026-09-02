/**
 * @file uart_driver.h
 * @brief Driver de comunicación UART (capa Hardware)
 *
 * Configura y administra la comunicación UART: transmisión, recepción,
 * baudrate y manejo de buffers, encapsulando el estado interno en una
 * estructura de instancia (sin variables globales).
 */

#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuración de inicialización del driver UART
 */
typedef struct {
    uart_port_t     port;          /**< Puerto UART a usar (UART_NUM_0, UART_NUM_1, UART_NUM_2) */
    int             tx_pin;        /**< Pin GPIO de transmisión (TX) */
    int             rx_pin;        /**< Pin GPIO de recepción (RX) */
    int             rts_pin;       /**< Pin GPIO de RTS (usar UART_PIN_NO_CHANGE si no se usa) */
    int             cts_pin;       /**< Pin GPIO de CTS (usar UART_PIN_NO_CHANGE si no se usa) */
    uint32_t        baudrate;      /**< Velocidad en baudios (ej. 115200) */
    uart_word_length_t data_bits;  /**< Bits de datos (UART_DATA_8_BITS, etc.) */
    uart_parity_t   parity;        /**< Paridad (UART_PARITY_DISABLE, UART_PARITY_EVEN, UART_PARITY_ODD) */
    uart_stop_bits_t stop_bits;    /**< Bits de parada (UART_STOP_BITS_1, UART_STOP_BITS_2) */
    int             rx_buffer_size;/**< Tamaño del buffer interno de recepción (bytes) */
    int             tx_buffer_size;/**< Tamaño del buffer interno de transmisión (bytes, 0 = modo bloqueante en TX) */
    int             event_queue_size; /**< Tamaño de la cola de eventos UART (0 = sin cola de eventos) */
} uart_driver_config_t;

/**
 * @brief Instancia de un driver UART
 *
 * Representa el estado interno de un periférico UART configurado.
 * Debe pasarse como primer parámetro a todas las funciones públicas.
 */
typedef struct {
    uart_port_t     port;          /**< Puerto UART asociado a esta instancia */
    QueueHandle_t   event_queue;   /**< Cola de eventos del driver UART (NULL si no se configuró) */
    bool            initialized;   /**< Indica si la instancia fue inicializada correctamente */
} uart_driver_t;

/**
 * @brief Inicializa y configura el periférico UART
 *
 * Configura puerto, baudrate, bits de datos, paridad, bits de parada,
 * pines y buffers internos del driver UART de ESP-IDF.
 *
 * @param[out] drv     Puntero a la instancia a inicializar
 * @param[in]  config  Puntero a la configuración deseada
 * @return ESP_OK en éxito, código de error en caso contrario
 */
esp_err_t uart_driver_init(uart_driver_t *drv, const uart_driver_config_t *config);

/**
 * @brief Desinicializa el periférico UART y libera sus recursos
 *
 * @param[in,out] drv Puntero a la instancia a desinicializar
 * @return ESP_OK en éxito, código de error en caso contrario
 */
esp_err_t uart_driver_deinit(uart_driver_t *drv);

/**
 * @brief Envía uno o varios bytes por el puerto UART
 *
 * @param[in]  drv           Puntero a la instancia UART
 * @param[in]  data          Buffer con los datos a enviar
 * @param[in]  len           Cantidad de bytes a enviar
 * @param[out] bytes_written Cantidad de bytes efectivamente escritos (puede ser NULL si no interesa)
 * @return ESP_OK en éxito, código de error en caso contrario
 */
esp_err_t uart_driver_write(uart_driver_t *drv, const uint8_t *data, size_t len, size_t *bytes_written);

/**
 * @brief Lee uno o varios bytes del puerto UART
 *
 * Operación bloqueante hasta @p timeout_ticks. Usar 0 para lectura no
 * bloqueante (poll) y portMAX_DELAY para bloquear indefinidamente.
 *
 * @param[in]  drv           Puntero a la instancia UART
 * @param[out] data          Buffer donde se almacenarán los datos leídos
 * @param[in]  len           Cantidad máxima de bytes a leer
 * @param[out] bytes_read    Cantidad de bytes efectivamente leídos (puede ser NULL si no interesa)
 * @param[in]  timeout_ticks Tiempo máximo de espera en ticks de FreeRTOS
 * @return ESP_OK en éxito, código de error en caso contrario
 */
esp_err_t uart_driver_read(uart_driver_t *drv, uint8_t *data, size_t len,
                            size_t *bytes_read, TickType_t timeout_ticks);

/**
 * @brief Devuelve la cantidad de bytes disponibles en el buffer de recepción
 *
 * Permite saber si hay datos pendientes sin necesidad de leerlos.
 *
 * @param[in]  drv       Puntero a la instancia UART
 * @param[out] available Cantidad de bytes disponibles para leer
 * @return ESP_OK en éxito, código de error en caso contrario
 */
esp_err_t uart_driver_available(uart_driver_t *drv, size_t *available);

/**
 * @brief Vacía el/los buffer(es) de la UART, descartando datos pendientes
 *
 * Útil para limpiar la comunicación antes de una nueva operación o
 * para recuperarse de errores.
 *
 * @param[in] drv        Puntero a la instancia UART
 * @param[in] flush_tx   true para vaciar el buffer de transmisión
 * @param[in] flush_rx   true para vaciar el buffer de recepción
 * @return ESP_OK en éxito, código de error en caso contrario
 */
esp_err_t uart_driver_flush(uart_driver_t *drv, bool flush_tx, bool flush_rx);

#ifdef __cplusplus
}
#endif

#endif /* UART_DRIVER_H */
