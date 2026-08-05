idf_component_register(
    SRCS "vazon_app_core.c"
    INCLUDE_DIRS "include"
    REQUIRES adc_service board_config climate_module command_router connection_config
             door_module esp_common esp_event esp_hw_support esp_netif
             esp_system esp_timer
             fan_module gpio_inputs gpio_outputs humidifier_module i2c_service
             light_module lwip mqtt_command_codec mqtt_command_queue mqtt_service
             mqtt_state_publisher nvs_flash onewire_service pot_module provisioning_button
             provisioning_service ota_service runtime_core status_led system_status
             wifi_service
)
