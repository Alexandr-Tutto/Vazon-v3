idf_component_register(
    SRCS "vazon_onewire_service.c"
    INCLUDE_DIRS "include"
    REQUIRES board_config onewire_bus ds18b20
)
