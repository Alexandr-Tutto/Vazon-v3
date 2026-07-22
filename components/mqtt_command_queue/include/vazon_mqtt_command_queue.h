#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "vazon_mqtt_command_codec.h"
#include "vazon_mqtt_service.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VAZON_MQTT_COMMAND_QUEUE_MAX_DEPTH 8U

typedef esp_err_t (*vazon_mqtt_command_lock_fn)(void *context);
typedef void (*vazon_mqtt_command_unlock_fn)(void *context);
typedef esp_err_t (*vazon_mqtt_command_publish_fn)(
    const char *topic, const char *payload, int qos, bool retain,
    void *context);

typedef struct {
    uint8_t depth;
    vazon_mqtt_command_lock_fn lock_runtime;
    vazon_mqtt_command_unlock_fn unlock_runtime;
    void *runtime_lock_context;
    vazon_mqtt_command_publish_fn publish;
    void *publish_context;
} vazon_mqtt_command_queue_config_t;

typedef struct {
    char topic[VAZON_MQTT_TOPIC_SIZE];
    char payload[VAZON_MQTT_MAX_PAYLOAD_SIZE + 1U];
    size_t topic_length;
    size_t payload_length;
} vazon_mqtt_queued_command_t;

typedef struct {
    QueueHandle_t handle;
    const vazon_mqtt_command_codec_t *codec;
    vazon_mqtt_command_queue_config_t config;
    uint32_t accepted_count;
    uint32_t dropped_count;
    bool initialized;
} vazon_mqtt_command_queue_t;

esp_err_t vazon_mqtt_command_queue_init(
    vazon_mqtt_command_queue_t *queue,
    const vazon_mqtt_command_queue_config_t *config,
    const vazon_mqtt_command_codec_t *codec);
esp_err_t vazon_mqtt_command_queue_enqueue(
    vazon_mqtt_command_queue_t *queue,
    const char *topic,
    size_t topic_length,
    const char *payload,
    size_t payload_length);
esp_err_t vazon_mqtt_command_queue_process_next(
    vazon_mqtt_command_queue_t *queue,
    TickType_t wait_ticks);
void vazon_mqtt_command_queue_on_message(
    const char *topic,
    size_t topic_length,
    const char *payload,
    size_t payload_length,
    void *context);

#ifdef __cplusplus
}
#endif
