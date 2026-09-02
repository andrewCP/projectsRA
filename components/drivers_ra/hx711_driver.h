#ifndef HX711_DRIVER_H
#define HX711_DRIVER_H

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Ganancia y canal del amplificador interno.
 * El valor numerico corresponde a los pulsos de reloj adicionales
 * que se envian despues de los 24 bits de datos.
 */
typedef enum {
    HX711_GAIN_128 = 1,  /**< Canal A, ganancia 128 (25 pulsos) */
    HX711_GAIN_32  = 2,  /**< Canal B, ganancia 32  (26 pulsos) */
    HX711_GAIN_64  = 3,  /**< Canal A, ganancia 64  (27 pulsos) */
} hx711_gain_t;

/** Configuracion de una instancia del driver. */
typedef struct {
    gpio_num_t   dout;    /**< Pin conectado a DT/DOUT del HX711 */
    gpio_num_t   pd_sck;  /**< Pin conectado a SCK/PD_SCK del HX711 */
    hx711_gain_t gain;    /**< Ganancia inicial */
} hx711_config_t;

/** Instancia del driver. Los campos son de uso interno. */
typedef struct {
    gpio_num_t   dout;
    gpio_num_t   pd_sck;
    hx711_gain_t gain;
    bool         initialized;
    bool         powered;
} hx711_t;

/**
 * Configura los pines GPIO, enciende el chip y establece la ganancia.
 *
 * @param dev    Instancia a inicializar.
 * @param config Parametros de configuracion.
 * @return ESP_OK si la inicializacion fue correcta.
 */
esp_err_t hx711_init(hx711_t *dev, const hx711_config_t *config);

/**
 * Indica si el HX711 termino una conversion (DOUT en nivel bajo).
 *
 * @param dev   Instancia del driver.
 * @param ready Salida: true si hay un dato disponible.
 */
esp_err_t hx711_is_ready(hx711_t *dev, bool *ready);

/**
 * Lee el valor bruto de 24 bits con signo, sin calibracion.
 * Requiere que el dato este disponible; de lo contrario retorna
 * ESP_ERR_NOT_FINISHED.
 *
 * @param dev Instancia del driver.
 * @param raw Salida: valor bruto en complemento a dos (-8388608..8388607).
 */
esp_err_t hx711_read_raw(hx711_t *dev, int32_t *raw);

/**
 * Selecciona canal y ganancia. El cambio se aplica al terminar la
 * siguiente llamada a hx711_read_raw(), por lo que esa primera lectura
 * todavia corresponde a la ganancia anterior y debe descartarse.
 */
esp_err_t hx711_set_gain(hx711_t *dev, hx711_gain_t gain);

/** Coloca el HX711 en modo de bajo consumo. */
esp_err_t hx711_power_down(hx711_t *dev);

/** Reactiva el HX711 para continuar con las conversiones. */
esp_err_t hx711_power_up(hx711_t *dev);

#ifdef __cplusplus
}
#endif