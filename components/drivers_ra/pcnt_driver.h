/**
 * @file pcnt_driver.h
 * @brief Driver de bajo nivel para el periferico Pulse Counter (PCNT) del ESP32.
 *
 * Capa: DRIVER (acceso directo a hardware).
 * Responsabilidad unica: inicializar, iniciar, detener, leer y limpiar
 * el contador de pulsos usando la API nativa driver/pulse_cnt.h
 * (ESP-IDF >= 5.0, valida para ESP-IDF 6.0.2).
 *
 * No conoce nada sobre el sensor TCRT5000 ni sobre logica de negocio:
 * eso corresponde a las capas Device y Service.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/pulse_cnt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuracion de entrada para inicializar una instancia del driver PCNT.
 */
typedef struct {
    int gpio_pulse_num;        /*!< GPIO conectado a la salida digital del sensor (flanco a contar). */
    int gpio_ctrl_num;         /*!< GPIO de control/gate opcional. Usar -1 si no se utiliza. */
    int16_t low_limit;         /*!< Limite inferior del contador (ej. -1 si no interesa contar en negativo). */
    int16_t high_limit;        /*!< Limite superior del contador antes de hacer wrap-around (ej. 32767). */
    uint32_t glitch_filter_ns; /*!< Ancho minimo de pulso valido en ns para filtrar ruido. 0 = sin filtro. */
    bool count_on_rising;      /*!< true: incrementa en flanco de subida. false: incrementa en flanco de bajada. */
} pcnt_config_t;

/**
 * @brief Estructura de instancia del driver PCNT.
 *
 * Encapsula el estado interno (handles del driver de ESP-IDF) de una
 * unidad PCNT concreta. No se deben acceder a sus campos directamente
 * desde fuera del modulo: se pasa siempre por puntero a las funciones
 * publicas.
 */
typedef struct {
    pcnt_unit_handle_t unit_handle;       /*!< Handle interno de la unidad PCNT (ESP-IDF). */
    pcnt_channel_handle_t channel_handle; /*!< Handle interno del canal PCNT (ESP-IDF). */
    bool initialized;                     /*!< Indica si la instancia fue inicializada correctamente. */
    bool running;                         /*!< Indica si el conteo esta actualmente habilitado. */
} pcnt_t;

/**
 * @brief Inicializa y configura el periferico PCNT, el GPIO de entrada y el modo de conteo.
 *
 * Asigna una unidad y un canal PCNT libres, configura el GPIO de entrada,
 * el filtro anti-glitch (si se solicita) y el modo de conteo (flanco
 * de subida o de bajada). Al finalizar, la unidad queda habilitada pero
 * DETENIDA: para comenzar a contar se debe llamar a pcnt_start().
 *
 * @param[in]  dev     Puntero a la instancia del driver (debe estar en memoria valida,
 *                      normalmente puesta a cero antes de llamar).
 * @param[in]  config  Puntero a la configuracion deseada.
 *
 * @return
 *      - ESP_OK en caso de exito.
 *      - ESP_ERR_INVALID_ARG si dev o config son NULL, o los parametros son invalidos.
 *      - ESP_ERR_NOT_FOUND si no hay unidades/canales PCNT libres.
 *      - Codigo de error propagado desde el driver de ESP-IDF en otros casos.
 */
esp_err_t pcnt_init(pcnt_t *dev, const pcnt_config_t *config);

/**
 * @brief Habilita el contador de pulsos (comienza/reanuda el conteo).
 *
 * @param[in] dev Puntero a la instancia previamente inicializada con pcnt_init().
 *
 * @return
 *      - ESP_OK en caso de exito.
 *      - ESP_ERR_INVALID_ARG si dev es NULL o no fue inicializado.
 *      - Codigo de error propagado desde el driver de ESP-IDF en otros casos.
 */
esp_err_t pcnt_start(pcnt_t *dev);

/**
 * @brief Detiene el conteo de pulsos. El valor acumulado se conserva.
 *
 * @param[in] dev Puntero a la instancia previamente inicializada con pcnt_init().
 *
 * @return
 *      - ESP_OK en caso de exito.
 *      - ESP_ERR_INVALID_ARG si dev es NULL o no fue inicializado.
 *      - Codigo de error propagado desde el driver de ESP-IDF en otros casos.
 */
esp_err_t pcnt_stop(pcnt_t *dev);

/**
 * @brief Devuelve el numero de pulsos contados desde la ultima lectura o reinicio.
 *
 * La lectura NO reinicia el contador (para eso usar pcnt_clear()).
 *
 * @param[in]  dev   Puntero a la instancia previamente inicializada con pcnt_init().
 * @param[out] count Puntero donde se escribira el valor actual del contador.
 *
 * @return
 *      - ESP_OK en caso de exito.
 *      - ESP_ERR_INVALID_ARG si dev o count son NULL, o dev no fue inicializado.
 *      - Codigo de error propagado desde el driver de ESP-IDF en otros casos.
 */
esp_err_t pcnt_get_count(pcnt_t *dev, int *count);

/**
 * @brief Reinicia el contador de pulsos a cero.
 *
 * @param[in] dev Puntero a la instancia previamente inicializada con pcnt_init().
 *
 * @return
 *      - ESP_OK en caso de exito.
 *      - ESP_ERR_INVALID_ARG si dev es NULL o no fue inicializado.
 *      - Codigo de error propagado desde el driver de ESP-IDF en otros casos.
 */
esp_err_t pcnt_clear(pcnt_t *dev);

/**
 * @brief Libera todos los recursos asociados a la instancia (canal y unidad PCNT).
 *
 * Deja la estructura lista para volver a llamar a pcnt_init() si se desea.
 * No es uno de los 5 metodos solicitados originalmente, pero se agrega
 * por buena practica de manejo de recursos (evita fugas si el modulo se
 * reconfigura dinamicamente).
 *
 * @param[in] dev Puntero a la instancia a liberar.
 *
 * @return
 *      - ESP_OK en caso de exito (o si ya estaba des-inicializada).
 *      - Codigo de error propagado desde el driver de ESP-IDF en otros casos.
 */
esp_err_t pcnt_deinit(pcnt_t *dev);

#ifdef __cplusplus
}
#endif
