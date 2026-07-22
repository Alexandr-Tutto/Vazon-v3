idf_component_register(
    SRCS "vazon_light_module.c"
    INCLUDE_DIRS "include"
    REQUIRES command_router esp_common runtime_core
)
