idf_component_register(
    SRCS "vazon_ota_service.c"
    INCLUDE_DIRS "include"
    REQUIRES app_update command_router esp-tls esp_http_client esp_https_ota json
)
