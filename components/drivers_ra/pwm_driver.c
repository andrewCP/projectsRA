// Libreria PWM para ESP32
#include "pwm_driver.h"

#include "esp_log.h"
static const char *TAG = "PWM";

esp_err_t pwm_init(pwm_t *pwm, gpio_num_t pin, ledc_timer_t timer, ledc_channel_t channel, uint32_t freq_hz, ledc_timer_bit_t resolution_bits){
    pwm->pin = pin;
    pwm->timer = timer;
    pwm->channel = channel;
    pwm->speed_mode = LEDC_LOW_SPEED_MODE;
    pwm->freq_hz = freq_hz;
    pwm->resolution_bits = resolution_bits;

    ledc_timer_config_t timer_conf = {
        .speed_mode = pwm->speed_mode,
        .timer_num = pwm->timer,
        .duty_resolution = pwm->resolution_bits,
        .freq_hz = pwm->freq_hz,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&timer_conf);
    if (err != ESP_OK) {
        return err;
    }

    ledc_channel_config_t channel_conf = {
        .speed_mode = pwm->speed_mode,
        .channel = pwm->channel,
        .timer_sel = pwm->timer,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = pwm->pin,
        .duty = 0,
        .hpoint = 0,
    };
    return ledc_channel_config(&channel_conf);
}

esp_err_t pwm_set_duty(pwm_t *pwm, uint32_t duty){
    esp_err_t err = ledc_set_duty(pwm->speed_mode, pwm->channel, duty);
    if (err != ESP_OK) {
        return err;
    }
    err = ledc_update_duty(pwm->speed_mode, pwm->channel);
    ESP_LOGI(TAG, "DUTY_SET");
    return err;
}

esp_err_t pwm_set_frequency(pwm_t *pwm, uint32_t freq_hz){
    esp_err_t err = ledc_set_freq(pwm->speed_mode, pwm->timer, freq_hz);
    if (err == ESP_OK) {
        pwm->freq_hz = freq_hz;
        ESP_LOGI(TAG, "FREQ_SET");
    }
    return err;
}

esp_err_t pwm_set_resolution(pwm_t *pwm, ledc_timer_bit_t resolution_bits){
    ledc_timer_config_t timer_conf = {
        .speed_mode = pwm->speed_mode,
        .timer_num = pwm->timer,
        .duty_resolution = resolution_bits,
        .freq_hz = pwm->freq_hz,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&timer_conf);
    if (err == ESP_OK) {
        pwm->resolution_bits = resolution_bits;
        ESP_LOGI(TAG, "RESOLUTION_SET");
    }
    return err;
}
