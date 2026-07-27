#include "gpio_driver.h"

#include "esp_log.h"
static const char *TAG = "GPIO";

void gpio_driver_init(gpio_num_t pin){

     gpio_config_t io_conf ={
       .pin_bit_mask = (1ULL << pin),
       .mode = GPIO_MODE_OUTPUT,
       .pull_up_en = GPIO_PULLUP_DISABLE,
       .pull_down_en = GPIO_PULLDOWN_DISABLE,
       .intr_type = GPIO_INTR_DISABLE,     

    };

    gpio_config(&io_conf);


}

void gpio_driver_on(gpio_num_t pin){
    gpio_set_level(pin,1);
    ESP_LOGI(TAG, "LED_ON");
   


}

void gpio_driver_off(gpio_num_t pin){

    gpio_set_level(pin,0);
    ESP_LOGI(TAG, "LED_OFF");
    

}

void gpio_driver_toggle(gpio_num_t pin){

    

}