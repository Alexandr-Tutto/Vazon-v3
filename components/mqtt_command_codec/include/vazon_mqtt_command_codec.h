#pragma once

#include "esp_err.h"
#include "vazon_command_router.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VAZON_MQTT_CODEC_DEVICE_ID_SIZE 48
#define VAZON_MQTT_CODEC_TOPIC_SIZE     160
#define VAZON_MQTT_CODEC_PAYLOAD_SIZE   384

typedef struct {
    bool available;
    char topic[VAZON_MQTT_CODEC_TOPIC_SIZE];
    char payload[VAZON_MQTT_CODEC_PAYLOAD_SIZE];
} vazon_mqtt_command_response_t;

typedef struct {
    const vazon_command_router_t *router;
    char device_id[VAZON_MQTT_CODEC_DEVICE_ID_SIZE];
    char command_prefix[VAZON_MQTT_CODEC_TOPIC_SIZE];
    bool initialized;
} vazon_mqtt_command_codec_t;

esp_err_t vazon_mqtt_command_codec_init(
    vazon_mqtt_command_codec_t *codec,
    const char *device_id,
    const vazon_command_router_t *router);
esp_err_t vazon_mqtt_command_codec_handle(
    const vazon_mqtt_command_codec_t *codec,
    const char *topic,
    size_t topic_length,
    const char *payload,
    size_t payload_length,
    vazon_mqtt_command_response_t *response);

#ifdef __cplusplus
}
#endif
