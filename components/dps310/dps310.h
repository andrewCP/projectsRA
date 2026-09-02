#ifndef DPS310_H
#define DPS310_H

#include "i2c_driver.h"
#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

/* Dirección I2C típica del DPS310 */
#define DPS310_I2C_ADDRESS_0 0x77
#define DPS310_I2C_ADDRESS_1 0x76

/* ID del DPS310 */
#define DPS310_PRODUCT_ID 0x10

/* ============================================================
   COEFICIENTES DE CALIBRACIÓN INTERNOS DEL DPS310
   ============================================================ */

typedef struct
{
    int32_t c0;
    int32_t c1;

    int32_t c00;
    int32_t c10;

    int32_t c01;
    int32_t c11;
    int32_t c20;
    int32_t c21;
    int32_t c30;

} dps310_coefficients_t;


/* ============================================================
   ESTRUCTURA PRINCIPAL DEL SENSOR
   ============================================================ */

typedef struct
{
    i2c_driver_t *i2c;

    uint8_t address;

    dps310_coefficients_t coefficients;

    /*
     * Presión atmosférica utilizada como referencia
     * para calcular la altura.
     */
    float pressure_reference;

    /*
     * Corrección adicional de altura.
     */
    float altitude_offset;

} dps310_t;


/* ============================================================
   FUNCIONES
   ============================================================ */

/*
 * Inicializa el DPS310.
 *
 * Ejemplo:
 *
 * dps310_t sensor;
 *
 * dps310_init(
 *     &sensor,
 *     &i2c,
 *     DPS310_I2C_ADDRESS_0
 * );
 */
esp_err_t dps310_init(
    dps310_t *sensor,
    i2c_driver_t *i2c,
    uint8_t address
);


/*
 * Lee la presión atmosférica en Pa.
 */
esp_err_t dps310_read_pressure(
    dps310_t *sensor,
    float *pressure_pa
);


/*
 * Lee la temperatura en °C.
 */
esp_err_t dps310_read_temperature(
    dps310_t *sensor,
    float *temperature_c
);


/*
 * Lee presión y temperatura.
 */
esp_err_t dps310_read_measurement(
    dps310_t *sensor,
    float *pressure_pa,
    float *temperature_c
);


/*
 * Convierte presión atmosférica en altura.
 *
 * pressure_pa:
 * presión medida en Pa.
 *
 * altitude_m:
 * altura calculada en metros.
 */
esp_err_t dps310_pressure_to_altitude(
    dps310_t *sensor,
    float pressure_pa,
    float *altitude_m
);


/*
 * Calibra el sensor tomando varias muestras
 * en la posición actual.
 *
 * samples:
 * número de muestras utilizadas.
 */
esp_err_t dps310_calibrate(
    dps310_t *sensor,
    uint16_t samples
);


/*
 * Lee directamente la altura.
 */
esp_err_t dps310_read_altitude(
    dps310_t *sensor,
    float *altitude_m
);


/*
 * Establece manualmente la presión de referencia.
 */
esp_err_t dps310_set_reference_pressure(
    dps310_t *sensor,
    float pressure_pa
);


/*
 * Establece manualmente un offset de altura.
 */
esp_err_t dps310_set_altitude_offset(
    dps310_t *sensor,
    float offset_m
);

#endif