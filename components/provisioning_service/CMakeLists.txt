idf_component_register(
    SRCS "vazon_provisioning_service.c"
    INCLUDE_DIRS "include"
    REQUIRES connection_config esp_http_server esp_netif esp_system esp_wifi
)
