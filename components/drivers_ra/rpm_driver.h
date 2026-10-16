/**
 * @file rpm_driver.h
 * @brief RPM a partir del conteo de pulsos del PCNT
 * Capa: SERVICE (logica de negocio).
 * Responsabilidad unica: convertir el conteo de pulsos y el tiempo
 * transcurrido en revoluciones por minuto (RPM). No accede al hardware
 * directamente: usa el driver pcnt_t que se le inyecta.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "pcnt_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estructura de instancia del servicio de velocidad.
 */
typedef struct {
    pcnt_t  *pcnt;        /*!< Driver PCNT ya inicializado (inyectado). */
    uint32_t ppr;         /*!< Pulsos contados por revolucion. */
    int      last_count;  /*!< Conteo en la lectura anterior. */
    int64_t  t_last_us;   /*!< Instante de la lectura anterior (us). */
    bool     initialized; /*!< Indica si la instancia fue inicializada. */
} speed_service_t;

/**
 * @brief Inicializa el servicio de velocidad.
 *
 * @param[in] self Puntero a la instancia del servicio.
 * @param[in] pcnt Puntero al driver PCNT ya inicializado con pcnt_init().
 * @param[in] ppr  Pulsos por revolucion (marcas del disco si se cuenta un flanco). Debe ser > 0.
 *
 * @return
 *      - ESP_OK en caso de exito.
 *      - ESP_ERR_INVALID_ARG si self o pcnt son NULL, o ppr es 0.
 */
esp_err_t speed_service_init(speed_service_t *self, pcnt_t *pcnt, uint32_t ppr);

/**
 * @brief Reinicia la referencia de conteo/tiempo e inicia el conteo de pulsos.
 *
 * @param[in] self Puntero a la instancia inicializada.
 *
 * @return
 *      - ESP_OK en caso de exito.
 *      - ESP_ERR_INVALID_ARG si self es NULL.
 *      - ESP_ERR_INVALID_STATE si self no fue inicializado.
 *      - Codigo de error propagado desde el driver PCNT.
 */
esp_err_t speed_service_start(speed_service_t *self);

/**
 * @brief Detiene el conteo de pulsos.
 *
 * @param[in] self Puntero a la instancia inicializada.
 *
 * @return
 *      - ESP_OK en caso de exito.
 *      - ESP_ERR_INVALID_ARG si self es NULL.
 *      - ESP_ERR_INVALID_STATE si self no fue inicializado.
 *      - Codigo de error propagado desde el driver PCNT.
 */
esp_err_t speed_service_stop(speed_service_t *self);

/**
 * @brief Calcula las RPM promedio desde la lectura anterior.
 *
 * RPM = (pulsos / PPR) * 60 / dt_s
 * La ventana de medicion es el tiempo entre dos llamadas consecutivas.
 *
 * @param[in]  self Puntero a la instancia inicializada.
 * @param[out] rpm  Puntero donde se escribe el resultado en RPM.
 *
 * @return
 *      - ESP_OK en caso de exito.
 *      - ESP_ERR_INVALID_ARG si self o rpm son NULL.
 *      - ESP_ERR_INVALID_STATE si no esta inicializado, dt <= 0 o el contador se reinicio externamente.
 *      - Codigo de error propagado desde el driver PCNT.
 */
esp_err_t speed_service_get_rpm(speed_service_t *self, float *rpm);

#ifdef __cplusplus
}
#endif