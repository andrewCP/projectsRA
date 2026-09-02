#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include "driver/i2c_master.h"
#include "esp_err.h"

typedef struct {
    i2c_master_bus_handle_t bus_handle;
    gpio_num_t sda_pin;
    gpio_num_t scl_pin;
    uint32_t clk_speed_hz;
} i2c_driver_t;
// Uso: i2c_driver_init(&i2c, I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_22, 400000);
esp_err_t i2c_driver_init(i2c_driver_t *i2c, i2c_port_num_t port, gpio_num_t sda_pin, gpio_num_t scl_pin, uint32_t clk_speed_hz);

// Uso: i2c_driver_write(&i2c, 0x68, data, 2);
esp_err_t i2c_driver_write(i2c_driver_t *i2c, uint8_t addr, const uint8_t *data, size_t len);

// Uso: i2c_driver_read(&i2c, 0x68, data, 6);
esp_err_t i2c_driver_read(i2c_driver_t *i2c, uint8_t addr, uint8_t *data, size_t len);

// Uso: i2c_driver_write_read(&i2c, 0x68, &reg, 1, data, 6);
esp_err_t i2c_driver_write_read(i2c_driver_t *i2c, uint8_t addr, const uint8_t *write_data, size_t write_len, uint8_t *read_data, size_t read_len);

// Uso: i2c_driver_scan(&i2c, found_addrs, 128, &found_count);
esp_err_t i2c_driver_scan(i2c_driver_t *i2c, uint8_t *found_addrs, uint8_t max_addrs, uint8_t *found_count);

#endif