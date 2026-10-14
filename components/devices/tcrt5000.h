/**
 * @file tcrt5000.h
 * @brief Capa DEVICE: representa un sensor optico reflectivo TCRT5000
 *        cuya salida digital genera un pulso por cada objeto/marca detectada.
 *
 * Responsabilidad unica: modelar el dispositivo fisico. Delega el conteo
 * de pulsos en la capa Driver (pcnt_driver). No contiene logica de negocio
 * (eso es responsabilidad de la capa Service).
 */

#pragma once

#include "esp_err.h"
#include "pcnt_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuracion de instancia del sensor TCRT5000.
 */
typedef struct {
    int gpio_out;               /*!< GPIO conectado al pin OUT (digital) del modulo TCRT5000. */
    uint32_t glitch_filter_ns;  /*!< Filtro anti-rebote/ruido en ns (recomendado ~1000 ns). */
} tcrt5000_config_t;

/**
 * @brief Instancia de un sensor TCRT5000.
 */
typedef struct {
    pcnt_t pcnt; /*!< Instancia del driver PCNT usada internamente por este dispositivo. */
} tcrt5000_t;

/**
 * @brief Inicializa el dispositivo TCRT5000 (configura el PCNT subyacente).
 *
 * @param[in] dev    Puntero a la instancia del dispositivo.
 * @param[in] config Configuracion del sensor (GPIO, filtro de ruido).
 * @return esp_err_t ESP_OK en exito, codigo de error en caso contrario.
 */
esp_err_t tcrt5000_init(tcrt5000_t *dev, const tcrt5000_config_t *config);

/** @brief Comienza a contar los pulsos detectados por el sensor. */
esp_err_t tcrt5000_start(tcrt5000_t *dev);

/** @brief Detiene el conteo de pulsos. */
esp_err_t tcrt5000_stop(tcrt5000_t *dev);

/** @brief Obtiene la cantidad de detecciones (pulsos) acumuladas. */
esp_err_t tcrt5000_get_count(tcrt5000_t *dev, int *count);

/** @brief Reinicia el contador de detecciones a cero. */
esp_err_t tcrt5000_clear(tcrt5000_t *dev);

#ifdef __cplusplus
}
#endif
