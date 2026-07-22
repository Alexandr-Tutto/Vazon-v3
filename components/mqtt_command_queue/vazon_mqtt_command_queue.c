#include "vazon_mqtt_command_queue.h"

#include "esp_log.h"

#include <string.h>

static const char *TAG = "mqtt_cmd_queue";

#define COMMAND_RESULT_QOS 1

esp_err_t vazon_mqtt_command_queue_init(
    vazon_mqtt_command_queue_t *queue,
    const vazon_mqtt_command_queue_config_t *config,
    const vazon_mqtt_command_codec_t *codec)
{
    if (queue == NULL || config == NULL || codec == NULL ||
        !codec->initialized || config->depth == 0U ||
        config->depth > VAZON_MQTT_COMMAND_QUEUE_MAX_DEPTH ||
        config->lock_runtime == NULL || config->unlock_runtime == NULL ||
        config->publish == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(queue, 0, sizeof(*queue));
    queue->handle = xQueueCreate(config->depth,
                                 sizeof(vazon_mqtt_queued_command_t));
    if (queue->handle == NULL) return ESP_ERR_NO_MEM;
    queue->codec = codec;
    queue->config = *config;
    queue->initialized = true;
    return ESP_OK;
}

esp_err_t vazon_mqtt_command_queue_enqueue(
    vazon_mqtt_command_queue_t *queue,
    const char *topic,
    size_t topic_length,
    const char *payload,
    size_t payload_length)
{
    if (queue == NULL || !queue->initialized || topic == NULL ||
        payload == NULL || topic_length == 0U ||
        topic_length >= VAZON_MQTT_TOPIC_SIZE ||
        payload_length > VAZON_MQTT_MAX_PAYLOAD_SIZE) {
        if (queue != NULL && queue->initialized) ++queue->dropped_count;
        return ESP_ERR_INVALID_ARG;
    }

    vazon_mqtt_queued_command_t command = {
        .topic_length = topic_length,
        .payload_length = payload_length,
    };
    memcpy(command.topic, topic, topic_length);
    command.topic[topic_length] = '\0';
    if (payload_length > 0U) {
        memcpy(command.payload, payload, payload_length);
    }
    command.payload[payload_length] = '\0';

    if (xQueueSend(queue->handle, &command, 0) != pdTRUE) {
        ++queue->dropped_count;
        return ESP_ERR_NO_MEM;
    }
    ++queue->accepted_count;
    return ESP_OK;
}

esp_err_t vazon_mqtt_command_queue_process_next(
    vazon_mqtt_command_queue_t *queue,
    TickType_t wait_ticks)
{
    if (queue == NULL || !queue->initialized) return ESP_ERR_INVALID_ARG;

    vazon_mqtt_queued_command_t command;
    if (xQueueReceive(queue->handle, &command, wait_ticks) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t result =
        queue->config.lock_runtime(queue->config.runtime_lock_context);
    if (result != ESP_OK) return result;

    vazon_mqtt_command_response_t response;
    result = vazon_mqtt_command_codec_handle(
        queue->codec, command.topic, command.topic_length,
        command.payload, command.payload_length, &response);
    queue->config.unlock_runtime(queue->config.runtime_lock_context);

    if (result != ESP_OK) {
        ESP_LOGW(TAG, "Command envelope rejected before routing: %s",
                 esp_err_to_name(result));
        return result;
    }
    if (!response.available) return ESP_ERR_INVALID_RESPONSE;

    result = queue->config.publish(
        response.topic, response.payload, COMMAND_RESULT_QOS, false,
        queue->config.publish_context);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Command result publish failed: %s",
                 esp_err_to_name(result));
    }
    return result;
}

void vazon_mqtt_command_queue_on_message(
    const char *topic,
    size_t topic_length,
    const char *payload,
    size_t payload_length,
    void *context)
{
    vazon_mqtt_command_queue_t *queue =
        (vazon_mqtt_command_queue_t *)context;
    const esp_err_t result = vazon_mqtt_command_queue_enqueue(
        queue, topic, topic_length, payload, payload_length);
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "Incoming command dropped: %s",
                 esp_err_to_name(result));
    }
}
