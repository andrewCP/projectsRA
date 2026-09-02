#include "hx711_driver.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"

/* Ancho minimo de cada semiciclo de PD_SCK (datasheet: 0.2 us a 50 us). */
#define HX711_CLK_PULSE_US   1

/* PD_SCK debe permanecer en alto mas de 60 us para entrar en bajo consumo. */
#define HX711_POWER_DOWN_US  80

/* Tiempo de estabilizacion del oscilador interno tras encender. */
#define HX711_POWER_UP_MS    10

/* Protege la secuencia de pulsos: si el planificador interrumpe con PD_SCK
   en alto por mas de 60 us, el chip entra en bajo consumo y se pierde el dato. */
static portMUX_TYPE hx711_spinlock = portMUX_INITIALIZER_UNLOCKED;

static inline void hx711_clock_pulse(const hx711_t *dev)
{
    gpio_set_level(dev->pd_sck, 1);
    esp_rom_delay_us(HX711_CLK_PULSE_US);
    gpio_set_level(dev->pd_sck, 0);
    esp_rom_delay_us(HX711_CLK_PULSE_US);
}

static bool hx711_gain_is_valid(hx711_gain_t gain)
{
    return gain == HX711_GAIN_128 || gain == HX711_GAIN_64 || gain == HX711_GAIN_32;
}

esp_err_t hx711_init(hx711_t *dev, const hx711_config_t *config)
{
    if (dev == NULL || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!GPIO_IS_VALID_GPIO(config->dout) || !GPIO_IS_VALID_OUTPUT_GPIO(config->pd_sck)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!hx711_gain_is_valid(config->gain)) {
        return ESP_ERR_INVALID_ARG;
    }

    const gpio_config_t sck_cfg = {
        .pin_bit_mask = 1ULL << config->pd_sck,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&sck_cfg);
    if (err != ESP_OK) {
        return err;
    }

    const gpio_config_t dout_cfg = {
        .pin_bit_mask = 1ULL << config->dout,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&dout_cfg);
    if (err != ESP_OK) {
        return err;
    }

    dev->dout        = config->dout;
    dev->pd_sck      = config->pd_sck;
    dev->gain        = config->gain;
    dev->initialized = true;
    dev->powered     = false;

    return hx711_power_up(dev);
}

esp_err_t hx711_is_ready(hx711_t *dev, bool *ready)
{
    if (dev == NULL || ready == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!dev->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    *ready = dev->powered && (gpio_get_level(dev->dout) == 0);
    return ESP_OK;
}

esp_err_t hx711_read_raw(hx711_t *dev, int32_t *raw)
{
    if (dev == NULL || raw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!dev->initialized || !dev->powered) {
        return ESP_ERR_INVALID_STATE;
    }
    if (gpio_get_level(dev->dout) != 0) {
        return ESP_ERR_NOT_FINISHED;
    }

    uint32_t value = 0;

    taskENTER_CRITICAL(&hx711_spinlock);

    /* 24 bits, MSB primero. El dato es valido mientras PD_SCK esta en alto. */
    for (int i = 0; i < 24; i++) {
        gpio_set_level(dev->pd_sck, 1);
        esp_rom_delay_us(HX711_CLK_PULSE_US);
        value = (value << 1) | (uint32_t)gpio_get_level(dev->dout);
        gpio_set_level(dev->pd_sck, 0);
        esp_rom_delay_us(HX711_CLK_PULSE_US);
    }

    /* Pulsos adicionales que fijan canal y ganancia de la proxima conversion. */
    for (int i = 0; i < (int)dev->gain; i++) {
        hx711_clock_pulse(dev);
    }

    taskEXIT_CRITICAL(&hx711_spinlock);

    /* Extension de signo de 24 a 32 bits (complemento a dos). */
    if (value & 0x00800000U) {
        value |= 0xFF000000U;
    }

    *raw = (int32_t)value;
    return ESP_OK;
}

esp_err_t hx711_set_gain(hx711_t *dev, hx711_gain_t gain)
{
    if (dev == NULL || !hx711_gain_is_valid(gain)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!dev->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    dev->gain = gain;
    return ESP_OK;
}

esp_err_t hx711_power_down(hx711_t *dev)
{
    if (dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!dev->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    gpio_set_level(dev->pd_sck, 0);
    esp_rom_delay_us(HX711_CLK_PULSE_US);
    gpio_set_level(dev->pd_sck, 1);
    esp_rom_delay_us(HX711_POWER_DOWN_US);

    dev->powered = false;
    return ESP_OK;
}

esp_err_t hx711_power_up(hx711_t *dev)
{
    if (dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!dev->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Al encender, el chip vuelve por defecto a canal A ganancia 128;
       la ganancia configurada se restablece con los pulsos extra de la
       siguiente lectura. */
    gpio_set_level(dev->pd_sck, 0);
    vTaskDelay(pdMS_TO_TICKS(HX711_POWER_UP_MS));

    dev->powered = true;
    return ESP_OK;
}