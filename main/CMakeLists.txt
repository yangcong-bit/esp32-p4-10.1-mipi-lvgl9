idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES "esp_lcd"
                             "esp_lcd_jd9366"
                             "esp_lcd_touch"
                             "esp_lcd_touch_jd9366"
                             "esp_timer"
                             "driver"
                             "esp_hw_support")
