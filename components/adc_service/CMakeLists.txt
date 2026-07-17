idf_component_register(
    SRCS "vazon_adc_service.c"
    INCLUDE_DIRS "include"
    REQUIRES board_config esp_adc
)
