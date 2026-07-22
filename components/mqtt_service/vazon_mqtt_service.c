#include "vazon_mqtt_service.h"

#include "esp_crt_bundle.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "mqtt_service";

#define VAZON_MQTT_KEEPALIVE_SEC 15U
#define VAZON_MQTT_QOS_COMMAND   1
#define VAZON_MQTT_QOS_STATUS    1

static bool copy_required_text(char *destination, size_t destination_size,
                               const char *source)
{
    if (destination == NULL || destination_size == 0U || source == NULL) {
        return false;
    }
    const size_t length = strnlen(source, destination_size);
    if (length == 0U || length >= destination_size) return false;
    memcpy(destination, source, length + 1U);
    return true;
}

static bool device_id_valid(const char *device_id)
{
    if (device_id == NULL || device_id[0] == '\0') return false;
    for (const char *cursor = device_id; *cursor != '\0'; ++cursor) {
        const char value = *cursor;
        const bool valid = (value >= 'a' && value <= 'z') ||
                           (value >= 'A' && value <= 'Z') ||
                           (value >= '0' && value <= '9') ||
                           value == '-' || value == '_';
        if (!valid) return false;
    }
    return true;
}

static esp_err_t format_text(char *destination, size_t destination_size,
                             const char *format, const char *device_id,
                             uint16_t port)
{
    const int written = port == 0U
                            ? snprintf(destination, destination_size, format,
                                       device_id)
                            : snprintf(destination, destination_size, format,
                                       device_id, (unsigned int)port);
    return written < 0 || (size_t)written >= destination_size
               ? ESP_ERR_INVALID_SIZE
               : ESP_OK;
}

static void notify_connection(vazon_mqtt_service_t *service,
                              vazon_mqtt_connection_state_t state)
{
    if (service->callbacks.on_connection != NULL) {
        service->callbacks.on_connection(state, service->callbacks.context);
    }
}

static void reset_receive(vazon_mqtt_service_t *service)
{
    service->rx_topic[0] = '\0';
    service->rx_payload[0] = '\0';
    service->rx_total_length = 0U;
    service->rx_received_length = 0U;
    service->rx_in_progress = false;
}

static void receive_data(vazon_mqtt_service_t *service,
                         const esp_mqtt_event_handle_t event)
{
    if (event->topic_len < 0 || event->data_len < 0 ||
        event->total_data_len < 0 || event->current_data_offset < 0) {
        reset_receive(service);
        return;
    }

    const size_t topic_length = (size_t)event->topic_len;
    const size_t fragment_length = (size_t)event->data_len;
    const size_t total_length = (size_t)event->total_data_len;
    const size_t fragment_offset = (size_t)event->current_data_offset;

    if (fragment_offset == 0U) {
        reset_receive(service);
        if (event->topic == NULL || topic_length == 0U ||
            topic_length >= sizeof(service->rx_topic) ||
            total_length > VAZON_MQTT_MAX_PAYLOAD_SIZE) {
            ESP_LOGW(TAG, "Rejected MQTT message: topic or payload too large");
            return;
        }
        memcpy(service->rx_topic, event->topic, topic_length);
        service->rx_topic[topic_length] = '\0';
        service->rx_total_length = total_length;
        service->rx_in_progress = true;
    }

    if (!service->rx_in_progress || fragment_offset != service->rx_received_length ||
        service->rx_received_length > service->rx_total_length ||
        fragment_length > service->rx_total_length - service->rx_received_length ||
        (fragment_length > 0U && event->data == NULL)) {
        ESP_LOGW(TAG, "Rejected invalid MQTT data fragment");
        reset_receive(service);
        return;
    }

    if (fragment_length > 0U) {
        memcpy(service->rx_payload + service->rx_received_length,
               event->data, fragment_length);
    }
    service->rx_received_length += fragment_length;
    if (service->rx_received_length != service->rx_total_length) return;

    service->rx_payload[service->rx_total_length] = '\0';
    if (service->callbacks.on_message != NULL) {
        service->callbacks.on_message(
            service->rx_topic, strlen(service->rx_topic),
            service->rx_payload, service->rx_total_length,
            service->callbacks.context);
    }
    reset_receive(service);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    (void)event_base;
    vazon_mqtt_service_t *service = (vazon_mqtt_service_t *)handler_args;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    if (service == NULL || event == NULL) return;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_BEFORE_CONNECT:
        notify_connection(service, VAZON_MQTT_CONNECTION_CONNECTING);
        break;

    case MQTT_EVENT_CONNECTED: {
        ESP_LOGI(TAG, "Connected; subscribing to %s", service->command_topic);
        const int subscription_id = esp_mqtt_client_subscribe(
            service->client, service->command_topic, VAZON_MQTT_QOS_COMMAND);
        if (subscription_id < 0) {
            ESP_LOGE(TAG, "Command subscription failed");
        }
        const int availability_id = esp_mqtt_client_publish(
            service->client, service->availability_topic, "online", 0,
            VAZON_MQTT_QOS_STATUS, 1);
        if (availability_id < 0) {
            ESP_LOGE(TAG, "Online availability publish failed");
        }
        notify_connection(service, VAZON_MQTT_CONNECTION_CONNECTED);
        break;
    }

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Disconnected");
        notify_connection(service, VAZON_MQTT_CONNECTION_DISCONNECTED);
        break;

    case MQTT_EVENT_DATA:
        receive_data(service, event);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "Transport error");
        break;

    default:
        break;
    }
}

esp_err_t vazon_mqtt_service_init(
    vazon_mqtt_service_t *service,
    const vazon_mqtt_service_config_t *config,
    const vazon_mqtt_service_callbacks_t *callbacks)
{
    if (service == NULL || config == NULL || callbacks == NULL ||
        !device_id_valid(config->device_id) || config->port == 0U) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(service, 0, sizeof(*service));
    if (!copy_required_text(service->device_id, sizeof(service->device_id),
                            config->device_id) ||
        !copy_required_text(service->host, sizeof(service->host), config->host) ||
        !copy_required_text(service->username, sizeof(service->username),
                            config->username) ||
        !copy_required_text(service->password, sizeof(service->password),
                            config->password)) {
        return ESP_ERR_INVALID_ARG;
    }
    service->port = config->port;
    service->callbacks = *callbacks;

    esp_err_t result = format_text(service->uri, sizeof(service->uri),
                                   "mqtts://%s:%u", service->host,
                                   service->port);
    if (result != ESP_OK) return result;
    result = format_text(service->command_topic, sizeof(service->command_topic),
                         "vazon/v3/%s/cmd/#", service->device_id, 0U);
    if (result != ESP_OK) return result;
    result = format_text(service->availability_topic,
                         sizeof(service->availability_topic),
                         "vazon/v3/%s/availability", service->device_id, 0U);
    if (result != ESP_OK) return result;

    service->initialized = true;
    return ESP_OK;
}

esp_err_t vazon_mqtt_service_start(vazon_mqtt_service_t *service)
{
    if (service == NULL || !service->initialized) return ESP_ERR_INVALID_ARG;
    if (service->started || service->client != NULL) return ESP_ERR_INVALID_STATE;

    const esp_mqtt_client_config_t config = {
        .broker = {
            .address.uri = service->uri,
            .verification.crt_bundle_attach = esp_crt_bundle_attach,
        },
        .credentials = {
            .username = service->username,
            .authentication.password = service->password,
            .client_id = service->device_id,
        },
        .session = {
            .keepalive = VAZON_MQTT_KEEPALIVE_SEC,
            .disable_clean_session = false,
            .last_will = {
                .topic = service->availability_topic,
                .msg = "offline",
                .msg_len = 0,
                .qos = VAZON_MQTT_QOS_STATUS,
                .retain = 1,
            },
        },
    };

    service->client = esp_mqtt_client_init(&config);
    if (service->client == NULL) return ESP_ERR_NO_MEM;

    esp_err_t result = esp_mqtt_client_register_event(
        service->client, ESP_EVENT_ANY_ID, mqtt_event_handler, service);
    if (result == ESP_OK) result = esp_mqtt_client_start(service->client);
    if (result != ESP_OK) {
        esp_mqtt_client_destroy(service->client);
        service->client = NULL;
        return result;
    }

    service->started = true;
    notify_connection(service, VAZON_MQTT_CONNECTION_CONNECTING);
    ESP_LOGI(TAG, "Started secure MQTT transport for device %s",
             service->device_id);
    return ESP_OK;
}

esp_err_t vazon_mqtt_service_publish(vazon_mqtt_service_t *service,
                                     const char *topic,
                                     const char *payload,
                                     int qos,
                                     bool retain)
{
    if (service == NULL || !service->started || service->client == NULL ||
        topic == NULL || topic[0] == '\0' || payload == NULL ||
        qos < 0 || qos > 2) {
        return ESP_ERR_INVALID_ARG;
    }
    const int message_id = esp_mqtt_client_publish(
        service->client, topic, payload, 0, qos, retain ? 1 : 0);
    return message_id < 0 ? ESP_FAIL : ESP_OK;
}

esp_err_t vazon_mqtt_service_stop(vazon_mqtt_service_t *service)
{
    if (service == NULL || !service->initialized) return ESP_ERR_INVALID_ARG;
    if (!service->started || service->client == NULL) return ESP_OK;

    esp_err_t result = esp_mqtt_client_stop(service->client);
    const esp_err_t destroy_result = esp_mqtt_client_destroy(service->client);
    service->client = NULL;
    service->started = false;
    notify_connection(service, VAZON_MQTT_CONNECTION_DISCONNECTED);
    return result != ESP_OK ? result : destroy_result;
}
