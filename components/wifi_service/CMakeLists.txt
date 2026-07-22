idf_component_register(
    SRCS "vazon_wifi_service.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_event esp_netif esp_timer esp_wifi
    PRIV_REQUIRES esp_hw_support
)
