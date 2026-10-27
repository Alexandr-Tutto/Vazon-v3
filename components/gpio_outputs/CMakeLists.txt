idf_component_register(
    SRCS "vazon_gpio_outputs.c"
    INCLUDE_DIRS "include"
    REQUIRES board_config
    PRIV_REQUIRES esp_driver_ledc
)
