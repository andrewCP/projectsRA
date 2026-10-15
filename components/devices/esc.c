#include "esc.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ESC";

#define ESC_PERIOD_US (1000000UL / ESC_PWM_FREQ_HZ)

static uint32_t pulse_us_to_duty(const esc_t *esc, uint32_t pulse_us)
{
    uint32_t max_duty = (1UL << esc->pwm.resolution_bits);
    return (uint32_t)(((uint64_t)pulse_us * max_duty) / ESC_PERIOD_US);
}

static esp_err_t esc_write_pulse_us(esc_t *esc, uint32_t pulse_us)
{
    return pwm_set_duty(&esc->pwm, pulse_us_to_duty(esc, pulse_us));
}

esp_err_t esc_init(esc_t *esc, gpio_num_t pin, ledc_timer_t timer, ledc_channel_t channel)
{
    if (esc == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = pwm_init(&esc->pwm, pin, timer, channel,
                             ESC_PWM_FREQ_HZ, ESC_PWM_RESOLUTION);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "pwm_init fallo: %s", esp_err_to_name(err));
        return err;
    }

    esc->initialized = true;
    esc->armed = false;
    esc->speed = 0.0f;

    /* Salida segura: pulso minimo */
    return esc_write_pulse_us(esc, ESC_MIN_PULSE_US);
}

esp_err_t esc_arm(esc_t *esc)
{
    if (esc == NULL || !esc->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = esc_write_pulse_us(esc, ESC_MIN_PULSE_US);
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "Armando ESC...");
    vTaskDelay(pdMS_TO_TICKS(ESC_ARM_TIME_MS));

    esc->armed = true;
    esc->speed = 0.0f;
    ESP_LOGI(TAG, "ESC armado");
    return ESP_OK;
}

esp_err_t esc_disarm(esc_t *esc)
{
    if (esc == NULL || !esc->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = esc_write_pulse_us(esc, ESC_MIN_PULSE_US);
    esc->armed = false;
    esc->speed = 0.0f;
    ESP_LOGI(TAG, "ESC desarmado");
    return err;
}

esp_err_t esc_set_speed(esc_t *esc, float speed)
{
    if (esc == NULL || !esc->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!esc->armed) {
        ESP_LOGW(TAG, "ESC no esta armado");
        return ESP_ERR_INVALID_STATE;
    }

    if (speed < 0.0f) speed = 0.0f;
    if (speed > 1.0f) speed = 1.0f;

    uint32_t pulse_us = ESC_MIN_PULSE_US +
                        (uint32_t)(speed * (ESC_MAX_PULSE_US - ESC_MIN_PULSE_US));

    esp_err_t err = esc_write_pulse_us(esc, pulse_us);
    if (err == ESP_OK) {
        esc->speed = speed;
    }
    return err;
}

bool esc_is_armed(const esc_t *esc)
{
    return esc != NULL && esc->armed;
}
