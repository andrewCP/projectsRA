idf_component_register(
    SRCS
        "gpio_driver.c"
        "i2c_driver.c"
        "uart_driver.c"
        "adc_driver.c"
        "pwm_driver.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        esp_driver_gpio
        esp_driver_i2c
        esp_driver_ledc
        esp_adc
)