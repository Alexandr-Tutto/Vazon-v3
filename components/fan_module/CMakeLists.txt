idf_component_register(
    SRCS "vazon_fan_module.c"
    INCLUDE_DIRS "include"
    REQUIRES command_router door_module esp_common runtime_core
)
