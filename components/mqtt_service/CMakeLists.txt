idf_component_register(
    SRCS "vazon_mqtt_service.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_event mqtt
    PRIV_REQUIRES mbedtls
)
