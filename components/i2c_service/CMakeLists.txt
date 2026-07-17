idf_component_register(
    SRCS "vazon_i2c_service.c"
    INCLUDE_DIRS "include"
    REQUIRES board_config esp_driver_i2c
)
