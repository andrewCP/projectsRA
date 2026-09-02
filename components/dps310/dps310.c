#include "dps310.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <math.h>
#include <stdint.h>

static const char *TAG = "DPS310";

/* ============================================================
   REGISTROS DEL DPS310
   ============================================================ */

#define DPS310_REG_PRS_B2       0x00
#define DPS310_REG_TMP_B2       0x03

#define DPS310_REG_PRS_CFG      0x06
#define DPS310_REG_TMP_CFG      0x07
#define DPS310_REG_MEAS_CFG     0x08
#define DPS310_REG_CFG_REG      0x09

#define DPS310_REG_PRODUCT_ID   0x0D
#define DPS310_REG_COEF         0x10

/* ============================================================
   CONFIGURACIÓN
   ============================================================ */

#define DPS310_KP 253952.0f
#define DPS310_KT 3670016.0f

/* Constantes para cálculo de altura */
#define DPS310_T0 288.15f
#define DPS310_G  9.80665f
#define DPS310_L  0.0065f
#define DPS310_R  287.05f


/* ============================================================
   LECTURA DE REGISTROS
   ============================================================ */

static esp_err_t dps310_read_registers(
    dps310_t *sensor,
    uint8_t reg,
    uint8_t *data,
    size_t len)
{
    if (sensor == NULL || data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_driver_write_read(
        sensor->i2c,
        sensor->address,
        &reg,
        1,
        data,
        len
    );
}


/* ============================================================
   ESCRITURA DE REGISTRO
   ============================================================ */

static esp_err_t dps310_write_register(
    dps310_t *sensor,
    uint8_t reg,
    uint8_t value)
{
    if (sensor == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2];

    data[0] = reg;
    data[1] = value;

    return i2c_driver_write(
        sensor->i2c,
        sensor->address,
        data,
        2
    );
}


/* ============================================================
   CONVERSIÓN SIGNED 24 BITS
   ============================================================ */

static int32_t dps310_raw_to_signed24(
    const uint8_t *data)
{
    uint32_t raw;

    raw =
        ((uint32_t)data[0] << 16) |
        ((uint32_t)data[1] << 8) |
        data[2];

    /*
     * El DPS310 utiliza datos con signo
     * en complemento a dos.
     */

    if (raw & 0x800000)
    {
        raw |= 0xFF000000;
    }

    return (int32_t)raw;
}


/* ============================================================
   LECTURA DE COEFICIENTES
   ============================================================ */

static esp_err_t dps310_read_coefficients(
    dps310_t *sensor)
{
    uint8_t data[18];

    esp_err_t err =
        dps310_read_registers(
            sensor,
            DPS310_REG_COEF,
            data,
            sizeof(data)
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Error leyendo coeficientes"
        );

        return err;
    }

    /*
     * c0 - 12 bits
     */

    uint32_t c0 =
        ((uint32_t)data[0] << 4) |
        ((data[1] >> 4) & 0x0F);

    /*
     * c1 - 12 bits
     */

    uint32_t c1 =
        ((uint32_t)(data[1] & 0x0F) << 8) |
        data[2];

    /*
     * c00 - 20 bits
     */

    uint32_t c00 =
        ((uint32_t)data[3] << 12) |
        ((uint32_t)data[4] << 4) |
        ((data[5] >> 4) & 0x0F);

    /*
     * c10 - 20 bits
     */

    uint32_t c10 =
        ((uint32_t)(data[5] & 0x0F) << 16) |
        ((uint32_t)data[6] << 8) |
        data[7];

    /*
     * Coeficientes de 16 bits
     */

    uint32_t c01 =
        ((uint32_t)data[8] << 8) |
        data[9];

    uint32_t c11 =
        ((uint32_t)data[10] << 8) |
        data[11];

    uint32_t c20 =
        ((uint32_t)data[12] << 8) |
        data[13];

    uint32_t c21 =
        ((uint32_t)data[14] << 8) |
        data[15];

    uint32_t c30 =
        ((uint32_t)data[16] << 8) |
        data[17];


    /*
     * Extensión de signo
     */

    if (c0 & 0x800)
    {
        c0 |= 0xFFFFF000;
    }

    if (c1 & 0x800)
    {
        c1 |= 0xFFFFF000;
    }

    if (c00 & 0x80000)
    {
        c00 |= 0xFFF00000;
    }

    if (c10 & 0x80000)
    {
        c10 |= 0xFFF00000;
    }


    sensor->coefficients.c0 =
        (int32_t)c0;

    sensor->coefficients.c1 =
        (int32_t)c1;

    sensor->coefficients.c00 =
        (int32_t)c00;

    sensor->coefficients.c10 =
        (int32_t)c10;

    sensor->coefficients.c01 =
        (int16_t)c01;

    sensor->coefficients.c11 =
        (int16_t)c11;

    sensor->coefficients.c20 =
        (int16_t)c20;

    sensor->coefficients.c21 =
        (int16_t)c21;

    sensor->coefficients.c30 =
        (int16_t)c30;


    ESP_LOGI(
        TAG,
        "Coeficientes DPS310 cargados"
    );

    return ESP_OK;
}


/* ============================================================
   INICIALIZACIÓN
   ============================================================ */

esp_err_t dps310_init(
    dps310_t *sensor,
    i2c_driver_t *i2c,
    uint8_t address)
{
    if (sensor == NULL || i2c == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    sensor->i2c = i2c;
    sensor->address = address;

    /*
     * Presión atmosférica estándar inicial.
     * Será reemplazada cuando calibremos.
     */

    sensor->pressure_reference =
        101325.0f;

    sensor->altitude_offset =
        0.0f;


    ESP_LOGI(
        TAG,
        "Inicializando DPS310..."
    );


    /* --------------------------------------------------------
       Leer Product ID
       -------------------------------------------------------- */

    uint8_t product_id;

    esp_err_t err =
        dps310_read_registers(
            sensor,
            DPS310_REG_PRODUCT_ID,
            &product_id,
            1
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "No se pudo leer Product ID"
        );

        return err;
    }


    ESP_LOGI(
        TAG,
        "Product ID: 0x%02X",
        product_id
    );


    if (product_id != DPS310_PRODUCT_ID)
    {
        ESP_LOGE(
            TAG,
            "Sensor incorrecto. ID=0x%02X",
            product_id
        );

        return ESP_ERR_NOT_FOUND;
    }


    /* --------------------------------------------------------
       Leer coeficientes
       -------------------------------------------------------- */

    err =
        dps310_read_coefficients(
            sensor
        );

    if (err != ESP_OK)
    {
        return err;
    }


    /* --------------------------------------------------------
       Configuración de presión
       16x oversampling
       -------------------------------------------------------- */

    err =
        dps310_write_register(
            sensor,
            DPS310_REG_PRS_CFG,
            0x04
        );

    if (err != ESP_OK)
    {
        return err;
    }


    /* --------------------------------------------------------
       Configuración de temperatura
       16x oversampling
       -------------------------------------------------------- */

    err =
        dps310_write_register(
            sensor,
            DPS310_REG_TMP_CFG,
            0x04
        );

    if (err != ESP_OK)
    {
        return err;
    }


    /* --------------------------------------------------------
       Medición continua de presión y temperatura
       -------------------------------------------------------- */

    err =
        dps310_write_register(
            sensor,
            DPS310_REG_MEAS_CFG,
            0x07
        );

    if (err != ESP_OK)
    {
        return err;
    }


    /* --------------------------------------------------------
       Configuración de shift
       -------------------------------------------------------- */

    err =
        dps310_write_register(
            sensor,
            DPS310_REG_CFG_REG,
            0x0C
        );

    if (err != ESP_OK)
    {
        return err;
    }


    ESP_LOGI(
        TAG,
        "DPS310 inicializado correctamente"
    );

    return ESP_OK;
}


/* ============================================================
   LECTURA DE TEMPERATURA
   ============================================================ */

esp_err_t dps310_read_temperature(
    dps310_t *sensor,
    float *temperature_c)
{
    if (sensor == NULL ||
        temperature_c == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[3];

    esp_err_t err =
        dps310_read_registers(
            sensor,
            DPS310_REG_TMP_B2,
            data,
            3
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Error leyendo temperatura"
        );

        return err;
    }


    int32_t raw_temperature =
        dps310_raw_to_signed24(data);


    /*
     * Temperatura escalada.
     */

    float tmp_sc =
        (float)raw_temperature /
        DPS310_KT;


    /*
     * T = c0 / 2 + c1 * Tsc
     */

    *temperature_c =
        ((float)sensor->coefficients.c0 * 0.5f)
        +
        ((float)sensor->coefficients.c1 *
         tmp_sc);


    return ESP_OK;
}


/* ============================================================
   LECTURA DE PRESIÓN
   ============================================================ */

esp_err_t dps310_read_pressure(
    dps310_t *sensor,
    float *pressure_pa)
{
    if (sensor == NULL ||
        pressure_pa == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[3];

    esp_err_t err =
        dps310_read_registers(
            sensor,
            DPS310_REG_PRS_B2,
            data,
            3
        );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Error leyendo presión"
        );

        return err;
    }


    int32_t raw_pressure =
        dps310_raw_to_signed24(data);


    /*
     * Presión escalada.
     */

    float p_sc =
        (float)raw_pressure /
        DPS310_KP;


    /*
     * Leer temperatura para
     * compensación.
     */

    float temperature_c;

    err =
        dps310_read_temperature(
            sensor,
            &temperature_c
        );

    if (err != ESP_OK)
    {
        return err;
    }


    /*
     * Temperatura escalada.
     */

    float t_sc =
        temperature_c /
        DPS310_KT;


    /*
     * Ecuación de compensación.
     */

    float pressure =
        (float)sensor->coefficients.c00;


    pressure +=
        p_sc *
        (
            (float)sensor->coefficients.c10
            +
            p_sc *
            (
                (float)sensor->coefficients.c20
                +
                p_sc *
                (float)sensor->coefficients.c30
            )
        );


    pressure +=
        t_sc *
        (float)sensor->coefficients.c01;


    pressure +=
        t_sc *
        p_sc *
        (
            (float)sensor->coefficients.c11
            +
            p_sc *
            (float)sensor->coefficients.c21
        );


    *pressure_pa =
        pressure;


    return ESP_OK;
}


/* ============================================================
   LECTURA DE PRESIÓN + TEMPERATURA
   ============================================================ */

esp_err_t dps310_read_measurement(
    dps310_t *sensor,
    float *pressure_pa,
    float *temperature_c)
{
    if (sensor == NULL ||
        pressure_pa == NULL ||
        temperature_c == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }


    esp_err_t err =
        dps310_read_temperature(
            sensor,
            temperature_c
        );

    if (err != ESP_OK)
    {
        return err;
    }


    return dps310_read_pressure(
        sensor,
        pressure_pa
    );
}


/* ============================================================
   PRESIÓN → ALTURA
   ============================================================ */

esp_err_t dps310_pressure_to_altitude(
    dps310_t *sensor,
    float pressure_pa,
    float *altitude_m)
{
    if (sensor == NULL ||
        altitude_m == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }


    if (pressure_pa <= 0.0f ||
        sensor->pressure_reference <= 0.0f)
    {
        return ESP_ERR_INVALID_ARG;
    }


    /*
     * Relación de presiones.
     */

    float ratio =
        pressure_pa /
        sensor->pressure_reference;


    /*
     * Exponente barométrico.
     */

    float exponent =
        (DPS310_R * DPS310_L) /
        DPS310_G;


    /*
     * Fórmula:
     *
     * h = T0/L *
     *     [1 - (P/P0)^((R*L)/g)]
     */

    float altitude =
        (DPS310_T0 /
         DPS310_L)
        *
        (
            1.0f -
            powf(
                ratio,
                exponent
            )
        );


    /*
     * Aplicar offset.
     */

    altitude +=
        sensor->altitude_offset;


    *altitude_m =
        altitude;


    return ESP_OK;
}


/* ============================================================
   CALIBRACIÓN
   ============================================================ */

esp_err_t dps310_calibrate(
    dps310_t *sensor,
    uint16_t samples)
{
    if (sensor == NULL ||
        samples == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }


    float pressure_sum = 0.0f;


    ESP_LOGI(
        TAG,
        "Iniciando calibración..."
    );


    for (uint16_t i = 0;
         i < samples;
         i++)
    {
        float pressure;


        esp_err_t err =
            dps310_read_pressure(
                sensor,
                &pressure
            );


        if (err != ESP_OK)
        {
            return err;
        }


        pressure_sum +=
            pressure;


        vTaskDelay(
            pdMS_TO_TICKS(100)
        );
    }


    /*
     * Promedio de presión.
     */

    sensor->pressure_reference =
        pressure_sum /
        (float)samples;


    /*
     * La posición actual será
     * considerada como altura 0.
     */

    sensor->altitude_offset =
        0.0f;


    ESP_LOGI(
        TAG,
        "Calibración terminada"
    );

    ESP_LOGI(
        TAG,
        "Presión referencia: %.2f Pa",
        sensor->pressure_reference
    );


    return ESP_OK;
}


/* ============================================================
   LECTURA DE ALTURA
   ============================================================ */

esp_err_t dps310_read_altitude(
    dps310_t *sensor,
    float *altitude_m)
{
    if (sensor == NULL ||
        altitude_m == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }


    float pressure;


    esp_err_t err =
        dps310_read_pressure(
            sensor,
            &pressure
        );


    if (err != ESP_OK)
    {
        return err;
    }


    return dps310_pressure_to_altitude(
        sensor,
        pressure,
        altitude_m
    );
}


/* ============================================================
   PRESIÓN DE REFERENCIA
   ============================================================ */

esp_err_t dps310_set_reference_pressure(
    dps310_t *sensor,
    float pressure_pa)
{
    if (sensor == NULL ||
        pressure_pa <= 0.0f)
    {
        return ESP_ERR_INVALID_ARG;
    }


    sensor->pressure_reference =
        pressure_pa;


    return ESP_OK;
}


/* ============================================================
   OFFSET DE ALTURA
   ============================================================ */

esp_err_t dps310_set_altitude_offset(
    dps310_t *sensor,
    float offset_m)
{
    if (sensor == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }


    sensor->altitude_offset =
        offset_m;


    return ESP_OK;
}