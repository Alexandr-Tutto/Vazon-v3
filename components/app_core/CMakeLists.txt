idf_component_register(
    SRCS "vazon_app_core.c"
    INCLUDE_DIRS "include"
    REQUIRES adc_service gpio_inputs gpio_outputs i2c_service onewire_service
)
