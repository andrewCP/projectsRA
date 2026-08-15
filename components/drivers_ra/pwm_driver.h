#ifndef PWM_DRIVER_H
#define PWM_DRIVER_H

#include "driver/ledc.h"
#include "esp_err.h"

typedef struct {
    gpio_num_t pin;
    ledc_timer_t timer;
    ledc_channel_t channel;
    ledc_mode_t speed_mode;
    uint32_t freq_hz;
    ledc_timer_bit_t resolution_bits;
} pwm_t;

esp_err_t pwm_init(pwm_t *pwm, gpio_num_t pin, ledc_timer_t timer, ledc_channel_t channel, uint32_t freq_hz, ledc_timer_bit_t resolution_bits);
esp_err_t pwm_set_duty(pwm_t *pwm, uint32_t duty);
esp_err_t pwm_set_frequency(pwm_t *pwm, uint32_t freq_hz);
esp_err_t pwm_set_resolution(pwm_t *pwm, ledc_timer_bit_t resolution_bits);

#endif
