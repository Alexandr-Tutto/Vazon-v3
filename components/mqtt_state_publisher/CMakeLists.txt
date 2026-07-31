idf_component_register(
    SRCS "vazon_mqtt_state_publisher.c"
    INCLUDE_DIRS "include"
    REQUIRES climate_module door_module esp_common fan_module humidifier_module
             light_module pot_module runtime_core system_status
)
