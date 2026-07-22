idf_component_register(
    SRCS "vazon_humidifier_module.c"
    INCLUDE_DIRS "include"
    REQUIRES board_config command_router door_module esp_common runtime_core
)
