#pragma once

#include "esp_err.h"
#include "mqtt_client.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VAZON_MQTT_DEVICE_ID_SIZE   48
#define VAZON_MQTT_HOST_SIZE        64
#define VAZON_MQTT_USERNAME_SIZE    64
#define VAZON_MQTT_PASSWORD_SIZE    96
#define VAZON_MQTT_URI_SIZE         192
#define VAZON_MQTT_TOPIC_SIZE       160
#define VAZON_MQTT_MAX_PAYLOAD_SIZE 1024

typedef enum {
    VAZON_MQTT_CONNECTION_DISCONNECTED = 0,
    VAZON_MQTT_CONNECTION_CONNECTING,
    VAZON_MQTT_CONNECTION_CONNECTED,
} vazon_mqtt_connection_state_t;

typedef struct {
    const char *device_id;
    const char *host;
    uint16_t port;
    const char *username;
    const char *password;
} vazon_mqtt_service_config_t;

typedef void (*vazon_mqtt_connection_callback_t)(
    vazon_mqtt_connection_state_t state, void *context);
typedef void (*vazon_mqtt_message_callback_t)(
    const char *topic, size_t topic_length,
    const char *payload, size_t payload_length,
    void *context);

typedef struct {
    vazon_mqtt_connection_callback_t on_connection;
    vazon_mqtt_message_callback_t on_message;
    void *context;
} vazon_mqtt_service_callbacks_t;

typedef struct {
    esp_mqtt_client_handle_t client;
    vazon_mqtt_service_callbacks_t callbacks;
    char device_id[VAZON_MQTT_DEVICE_ID_SIZE];
    char host[VAZON_MQTT_HOST_SIZE];
    char username[VAZON_MQTT_USERNAME_SIZE];
    char password[VAZON_MQTT_PASSWORD_SIZE];
    char uri[VAZON_MQTT_URI_SIZE];
    char command_topic[VAZON_MQTT_TOPIC_SIZE];
    char availability_topic[VAZON_MQTT_TOPIC_SIZE];
    char rx_topic[VAZON_MQTT_TOPIC_SIZE];
    char rx_payload[VAZON_MQTT_MAX_PAYLOAD_SIZE + 1U];
    size_t rx_total_length;
    size_t rx_received_length;
    uint16_t port;
    bool rx_in_progress;
    bool initialized;
    bool started;
} vazon_mqtt_service_t;

esp_err_t vazon_mqtt_service_init(
    vazon_mqtt_service_t *service,
    const vazon_mqtt_service_config_t *config,
    const vazon_mqtt_service_callbacks_t *callbacks);
esp_err_t vazon_mqtt_service_start(vazon_mqtt_service_t *service);
esp_err_t vazon_mqtt_service_publish(vazon_mqtt_service_t *service,
                                     const char *topic,
                                     const char *payload,
                                     int qos,
                                     bool retain);
esp_err_t vazon_mqtt_service_stop(vazon_mqtt_service_t *service);

#ifdef __cplusplus
}
#endif
