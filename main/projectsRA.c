#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "i2c_driver.h"
#include "dps310.h"


static const char *TAG = "MAIN";


void app_main(void)
{
    // 1. Crear el driver I2C
    i2c_driver_t i2c;

    // 2. Inicializar I2C
    esp_err_t err = i2c_driver_init(
        &i2c,
        I2C_NUM_0,
        GPIO_NUM_21,
        GPIO_NUM_22,
        400000
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error inicializando I2C");
        return;
    }

    ESP_LOGI(TAG, "I2C inicializado correctamente");


    // 3. Crear estructura del DPS310
    dps310_t sensor;


    // 4. Inicializar DPS310
    err = dps310_init(
        &sensor,
        &i2c,
        DPS310_I2C_ADDRESS_0
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error inicializando DPS310");
        return;
    }

    ESP_LOGI(TAG, "DPS310 inicializado correctamente");


    // 5. Calibrar el sensor
    ESP_LOGI(TAG, "Iniciando calibracion...");

    err = dps310_calibrate(
        &sensor,
        10
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error durante la calibracion");
        return;
    }

    ESP_LOGI(TAG, "Calibracion terminada");


    // 6. Leer y mostrar datos continuamente
    while (1)
    {
        float pressure_pa = 0.0f;
        float temperature_c = 0.0f;
        float altitude_m = 0.0f;


        // Leer presión y temperatura
        err = dps310_read_measurement(
            &sensor,
            &pressure_pa,
            &temperature_c
        );

        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error leyendo DPS310");

            vTaskDelay(pdMS_TO_TICKS(1000));

            continue;
        }


        // Convertir presión a altura
        err = dps310_pressure_to_altitude(
            &sensor,
            pressure_pa,
            &altitude_m
        );

        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error convirtiendo presion a altura");

            vTaskDelay(pdMS_TO_TICKS(1000));

            continue;
        }


        // Mostrar resultados
        ESP_LOGI(
            TAG,
            "Presion: %.2f Pa | Temperatura: %.2f C | Altura: %.2f m",
            pressure_pa,
            temperature_c,
            altitude_m
        );


        // Esperar 1 segundo
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
