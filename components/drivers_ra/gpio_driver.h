#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H
// #include "driver/ledc.h"
#include "driver/gpio.h"



void gpio_driver_init(gpio_num_t pin);
void gpio_driver_on(gpio_num_t pin);
void gpio_driver_off(gpio_num_t pin);
void gpio_driver_toggle(gpio_num_t pin);

#endif