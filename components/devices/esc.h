#ifndef ESC_H
#define ESC_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "pwm_driver.h"


#define ESC_PWM_FREQ_HZ      50      
#define ESC_PWM_RESOLUTION   LEDC_TIMER_14_BIT
#define ESC_MIN_PULSE_US     1000   
#define ESC_MAX_PULSE_US     2000   
#define ESC_ARM_TIME_MS      2000   

typedef struct {
    pwm_t pwm;
    bool initialized;
    bool armed;
    float speed;  
} esc_t;


esp_err_t esc_init(esc_t *esc, gpio_num_t pin, ledc_timer_t timer, ledc_channel_t channel);

esp_err_t esc_arm(esc_t *esc);


esp_err_t esc_disarm(esc_t *esc);

esp_err_t esc_set_speed(esc_t *esc, float speed);

bool esc_is_armed(const esc_t *esc);

#endif
