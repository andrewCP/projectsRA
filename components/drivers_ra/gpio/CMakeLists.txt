idf_component_register(
    SRCS    
            "gpio_driver.c" 


    INCLUDE_DIRS "."
    REQUIRES 
        esp_driver_gpio 
              
)