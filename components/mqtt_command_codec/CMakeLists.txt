idf_component_register(
    SRCS "vazon_mqtt_command_codec.c"
    INCLUDE_DIRS "include"
    REQUIRES climate_module command_router esp_timer fan_module
             humidifier_module json light_module pot_module runtime_core
)
