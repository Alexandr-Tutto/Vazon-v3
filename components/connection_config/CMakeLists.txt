idf_component_register(
    SRCS "vazon_connection_config.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash
    PRIV_REQUIRES esp_hw_support
)
