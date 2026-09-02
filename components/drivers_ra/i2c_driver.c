#include "i2c_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "I2C_DRIVER";
#define I2C_TIMEOUT_MS 1000

// Uso: i2c_driver_init(&i2c, I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_22, 400000);
esp_err_t i2c_driver_init(i2c_driver_t *i2c, i2c_port_num_t port, gpio_num_t sda_pin, gpio_num_t scl_pin, uint32_t clk_speed_hz){
    if (i2c == NULL) return ESP_ERR_INVALID_ARG;

    i2c->sda_pin = sda_pin;
    i2c->scl_pin = scl_pin;
    i2c->clk_speed_hz = clk_speed_hz;

    i2c_master_bus_config_t bus_config = {
        .i2c_port = port,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t err = i2c_new_master_bus(&bus_config, &i2c->bus_handle);
    if (err == ESP_OK) ESP_LOGI(TAG, "I2C_INIT_OK");
    return err;
}

// Helper interno: agrega un device temporal para la dirección dada
static esp_err_t add_device(i2c_driver_t *i2c, uint8_t addr, i2c_master_dev_handle_t *dev){
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = i2c->clk_speed_hz,
    };
    return i2c_master_bus_add_device(i2c->bus_handle, &dev_config, dev);
}

// Uso: i2c_driver_write(&i2c, 0x68, data, 2);
esp_err_t i2c_driver_write(i2c_driver_t *i2c, uint8_t addr, const uint8_t *data, size_t len){
    if (i2c == NULL || data == NULL) return ESP_ERR_INVALID_ARG;

    i2c_master_dev_handle_t dev;
    esp_err_t err = add_device(i2c, addr, &dev);
    if (err != ESP_OK) return err;
    err = i2c_master_transmit(dev, data, len, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    if (err != ESP_OK) ESP_LOGE(TAG, "I2C_WRITE_FAIL");

    i2c_master_bus_rm_device(dev);
    return err;
}

// Uso: i2c_driver_read(&i2c, 0x68, data, 6);
esp_err_t i2c_driver_read(i2c_driver_t *i2c, uint8_t addr, uint8_t *data, size_t len){
    if (i2c == NULL || data == NULL) return ESP_ERR_INVALID_ARG;

    i2c_master_dev_handle_t dev;
    esp_err_t err = add_device(i2c, addr, &dev);
    if (err != ESP_OK) return err;

    err = i2c_master_receive(dev, data, len, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    if (err != ESP_OK) ESP_LOGE(TAG, "I2C_READ_FAIL");

    i2c_master_bus_rm_device(dev);
    return err;
}

// Uso: i2c_driver_write_read(&i2c, 0x68, &reg, 1, data, 6);
esp_err_t i2c_driver_write_read(i2c_driver_t *i2c, uint8_t addr, const uint8_t *write_data, size_t write_len, uint8_t *read_data, size_t read_len){
    if (i2c == NULL || write_data == NULL || read_data == NULL) return ESP_ERR_INVALID_ARG;

    i2c_master_dev_handle_t dev;
    esp_err_t err = add_device(i2c, addr, &dev);
    if (err != ESP_OK) return err;

    err = i2c_master_transmit_receive(dev, write_data, write_len, read_data, read_len, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    if (err != ESP_OK) ESP_LOGE(TAG, "I2C_WRITE_READ_FAIL");

    i2c_master_bus_rm_device(dev);
    return err;
}

// Uso: i2c_driver_scan(&i2c, found_addrs, 128, &found_count);
esp_err_t i2c_driver_scan(i2c_driver_t *i2c, uint8_t *found_addrs, uint8_t max_addrs, uint8_t *found_count){
    if (i2c == NULL || found_addrs == NULL || found_count == NULL) return ESP_ERR_INVALID_ARG;

    *found_count = 0;
    for (uint8_t addr = 1; addr < 127 && *found_count < max_addrs; addr++){
        esp_err_t err = i2c_master_probe(i2c->bus_handle, addr, pdMS_TO_TICKS(50));
        if (err == ESP_OK){
            found_addrs[*found_count] = addr;
            (*found_count)++;
            ESP_LOGI(TAG, "I2C_DEVICE_FOUND");
        }
    }
    return ESP_OK;
}