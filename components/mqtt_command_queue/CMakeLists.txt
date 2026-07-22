idf_component_register(
    SRCS "vazon_mqtt_command_queue.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos mqtt_command_codec mqtt_service
)
