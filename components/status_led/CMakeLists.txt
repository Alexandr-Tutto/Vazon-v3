idf_component_register(
    SRCS "vazon_status_led.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_common runtime_core system_status
)
