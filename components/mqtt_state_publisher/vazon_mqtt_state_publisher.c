#include "vazon_mqtt_state_publisher.h"

#include <stdio.h>
#include <string.h>

#define MQTT_STATE_QOS          1
#define MQTT_STATE_PAYLOAD_SIZE 1024U
#define MQTT_STATE_TOPIC_SIZE   160U

typedef enum {
    TOPIC_SYSTEM_STATE = 0,
    TOPIC_SYSTEM_STATUS,
    TOPIC_SYSTEM_SETTINGS,
    TOPIC_CLIMATE_STATE,
    TOPIC_CLIMATE_STATUS,
    TOPIC_CLIMATE_SETTINGS,
    TOPIC_DOOR_STATE,
    TOPIC_DOOR_STATUS,
    TOPIC_POT_0_STATE,
    TOPIC_POT_0_STATUS,
    TOPIC_POT_0_SETTINGS,
    TOPIC_POT_1_STATE,
    TOPIC_POT_1_STATUS,
    TOPIC_POT_1_SETTINGS,
    TOPIC_LIGHT_STATE,
    TOPIC_LIGHT_STATUS,
    TOPIC_LIGHT_SETTINGS,
    TOPIC_FAN_STATE,
    TOPIC_FAN_STATUS,
    TOPIC_FAN_SETTINGS,
    TOPIC_HUMIDIFIER_STATE,
    TOPIC_HUMIDIFIER_STATUS,
    TOPIC_HUMIDIFIER_SETTINGS,
} topic_slot_t;

static esp_err_t publish_payload(vazon_mqtt_state_publisher_t *publisher,
                                 topic_slot_t slot,
                                 const char *topic_class,
                                 const char *entity,
                                 const char *payload,
                                 bool force);

static const char *bool_text(bool value)
{
    return value ? "true" : "false";
}

static const char *tristate_text(vazon_tristate_t value)
{
    if (value == VAZON_TRISTATE_TRUE) return "true";
    if (value == VAZON_TRISTATE_FALSE) return "false";
    return "null";
}

static const char *connection_text(vazon_connection_state_t value)
{
    switch (value) {
    case VAZON_CONNECTION_DISCONNECTED: return "disconnected";
    case VAZON_CONNECTION_CONNECTING: return "connecting";
    case VAZON_CONNECTION_CONNECTED: return "connected";
    case VAZON_CONNECTION_UNKNOWN:
    default: return "unknown";
    }
}

static const char *maintenance_reason_text(vazon_maintenance_reason_t value)
{
    switch (value) {
    case VAZON_MAINTENANCE_REASON_MANUAL: return "manual";
    case VAZON_MAINTENANCE_REASON_EXTERNAL: return "external";
    case VAZON_MAINTENANCE_REASON_DOOR: return "door";
    case VAZON_MAINTENANCE_REASON_UNKNOWN:
    default: return "unknown";
    }
}

static const char *system_status_text(vazon_system_status_value_t value)
{
    switch (value) {
    case VAZON_SYSTEM_STATUS_OK: return "ok";
    case VAZON_SYSTEM_STATUS_WARNING: return "warning";
    case VAZON_SYSTEM_STATUS_ERROR: return "error";
    case VAZON_SYSTEM_STATUS_UNKNOWN:
    default: return "unknown";
    }
}

static const char *affected_text(vazon_system_affected_t value)
{
    switch (value) {
    case VAZON_SYSTEM_AFFECTED_CLIMATE: return "climate";
    case VAZON_SYSTEM_AFFECTED_POT: return "pot";
    case VAZON_SYSTEM_AFFECTED_LIGHT: return "light";
    case VAZON_SYSTEM_AFFECTED_FAN: return "fan";
    case VAZON_SYSTEM_AFFECTED_HUMIDIFIER: return "humidifier";
    case VAZON_SYSTEM_AFFECTED_DOOR: return "door";
    case VAZON_SYSTEM_AFFECTED_CONNECTION: return "connection";
    case VAZON_SYSTEM_AFFECTED_SYSTEM: return "system";
    case VAZON_SYSTEM_AFFECTED_NONE:
    default: return NULL;
    }
}

static const char *climate_status_text(vazon_climate_status_t value)
{
    switch (value) {
    case VAZON_CLIMATE_STATUS_OK: return "ok";
    case VAZON_CLIMATE_STATUS_WARNING: return "warning";
    case VAZON_CLIMATE_STATUS_ERROR: return "error";
    case VAZON_CLIMATE_STATUS_INACTIVE: return "inactive";
    case VAZON_CLIMATE_STATUS_UNKNOWN:
    default: return "unknown";
    }
}

static const char *climate_sensor_status_text(vazon_climate_sensor_status_t value)
{
    switch (value) {
    case VAZON_CLIMATE_SENSOR_OK: return "ok";
    case VAZON_CLIMATE_SENSOR_WARNING: return "warning";
    case VAZON_CLIMATE_SENSOR_ERROR: return "error";
    case VAZON_CLIMATE_SENSOR_UNKNOWN:
    default: return "unknown";
    }
}

static const char *climate_reason_text(vazon_climate_status_reason_t value)
{
    switch (value) {
    case VAZON_CLIMATE_REASON_SHT31_0X44_MISSING: return "sht31_0x44_missing";
    case VAZON_CLIMATE_REASON_SHT31_0X45_MISSING: return "sht31_0x45_missing";
    case VAZON_CLIMATE_REASON_SHT31_STALE: return "sht31_stale";
    case VAZON_CLIMATE_REASON_INVALID_VALUE: return "invalid_value";
    case VAZON_CLIMATE_REASON_TEMPERATURE_OUT_OF_RANGE: return "temperature_out_of_range";
    case VAZON_CLIMATE_REASON_HUMIDITY_OUT_OF_RANGE: return "humidity_out_of_range";
    case VAZON_CLIMATE_REASON_TEMPERATURE_DELTA_HIGH: return "temperature_delta_high";
    case VAZON_CLIMATE_REASON_TEMPERATURE_DELTA_CRITICAL: return "temperature_delta_critical";
    case VAZON_CLIMATE_REASON_NONE:
    default: return NULL;
    }
}

static const char *door_state_text(vazon_door_state_t value)
{
    switch (value) {
    case VAZON_DOOR_STATE_CLOSED: return "closed";
    case VAZON_DOOR_STATE_OPEN: return "open";
    case VAZON_DOOR_STATE_UNKNOWN:
    default: return "unknown";
    }
}

static const char *door_status_text(vazon_door_status_t value)
{
    switch (value) {
    case VAZON_DOOR_STATUS_OK: return "ok";
    case VAZON_DOOR_STATUS_WARNING: return "warning";
    case VAZON_DOOR_STATUS_ERROR: return "error";
    case VAZON_DOOR_STATUS_INACTIVE: return "inactive";
    case VAZON_DOOR_STATUS_UNKNOWN:
    default: return "unknown";
    }
}

static const char *pot_sensor_status_text(vazon_pot_sensor_status_t value)
{
    switch (value) {
    case VAZON_POT_SENSOR_OK: return "ok";
    case VAZON_POT_SENSOR_WARNING: return "warning";
    case VAZON_POT_SENSOR_ERROR: return "error";
    case VAZON_POT_SENSOR_INACTIVE: return "inactive";
    case VAZON_POT_SENSOR_UNKNOWN:
    default: return "unknown";
    }
}

static const char *pot_status_text(vazon_pot_status_t value)
{
    switch (value) {
    case VAZON_POT_STATUS_OK: return "ok";
    case VAZON_POT_STATUS_WARNING: return "warning";
    case VAZON_POT_STATUS_ERROR: return "error";
    case VAZON_POT_STATUS_INACTIVE: return "inactive";
    case VAZON_POT_STATUS_UNKNOWN:
    default: return "unknown";
    }
}

static const char *pot_class_text(vazon_pot_moisture_class_t value)
{
    switch (value) {
    case VAZON_POT_MOISTURE_CLASS_DRY: return "dry";
    case VAZON_POT_MOISTURE_CLASS_NORMAL: return "normal";
    case VAZON_POT_MOISTURE_CLASS_WET: return "wet";
    case VAZON_POT_MOISTURE_CLASS_OVERWET: return "overwet";
    case VAZON_POT_MOISTURE_CLASS_UNKNOWN:
    default: return "unknown";
    }
}

static const char *pot_reason_text(vazon_pot_status_reason_t value)
{
    switch (value) {
    case VAZON_POT_REASON_CALIBRATION_INVALID: return "calibration_invalid";
    case VAZON_POT_REASON_MOISTURE_READ_FAILED: return "moisture_read_failed";
    case VAZON_POT_REASON_MOISTURE_STALE: return "moisture_stale";
    case VAZON_POT_REASON_TEMPERATURE_MISSING: return "temperature_missing";
    case VAZON_POT_REASON_TEMPERATURE_READ_FAILED: return "temperature_read_failed";
    case VAZON_POT_REASON_TEMPERATURE_STALE: return "temperature_stale";
    case VAZON_POT_REASON_TEMPERATURE_INVALID: return "temperature_invalid";
    case VAZON_POT_REASON_TEMPERATURE_OUT_OF_RANGE: return "temperature_out_of_range";
    case VAZON_POT_REASON_NONE:
    default: return NULL;
    }
}

static const char *command_result_text(vazon_command_result_status_t value)
{
    switch (value) {
    case VAZON_COMMAND_RESULT_ACCEPTED: return "accepted";
    case VAZON_COMMAND_RESULT_REJECTED: return "rejected";
    case VAZON_COMMAND_RESULT_FAILED: return "failed";
    case VAZON_COMMAND_RESULT_UNKNOWN:
    default: return "unknown";
    }
}

static const char *output_confirmation_text(vazon_tristate_t value)
{
    if (value == VAZON_TRISTATE_TRUE) return "true";
    if (value == VAZON_TRISTATE_FALSE) return "false";
    return "unknown";
}

static const char *light_output_text(vazon_light_output_t value)
{
    switch (value) {
    case VAZON_LIGHT_OUTPUT_OFF: return "off";
    case VAZON_LIGHT_OUTPUT_ON: return "on";
    case VAZON_LIGHT_OUTPUT_UNKNOWN:
    default: return "unknown";
    }
}

static const char *light_status_text(vazon_light_status_t value)
{
    switch (value) {
    case VAZON_LIGHT_STATUS_OK: return "ok";
    case VAZON_LIGHT_STATUS_WARNING: return "warning";
    case VAZON_LIGHT_STATUS_ERROR: return "error";
    case VAZON_LIGHT_STATUS_INACTIVE: return "inactive";
    case VAZON_LIGHT_STATUS_UNKNOWN:
    default: return "unknown";
    }
}

static const char *light_reason_text(vazon_light_status_reason_t value)
{
    switch (value) {
    case VAZON_LIGHT_STATUS_REASON_MAINTENANCE_SERVICE_LIGHT: return "maintenance_service_light";
    case VAZON_LIGHT_STATUS_REASON_DAY_WINDOW_UNKNOWN: return "day_window_unknown";
    case VAZON_LIGHT_STATUS_REASON_OUTPUT_FAILED: return "output_failed";
    case VAZON_LIGHT_STATUS_REASON_NONE:
    default: return NULL;
    }
}

static const char *fan_output_text(vazon_fan_output_t value)
{
    switch (value) {
    case VAZON_FAN_OUTPUT_OFF: return "off";
    case VAZON_FAN_OUTPUT_ON: return "on";
    case VAZON_FAN_OUTPUT_UNKNOWN:
    default: return "unknown";
    }
}

static const char *fan_auto_state_text(vazon_fan_auto_state_t value)
{
    switch (value) {
    case VAZON_FAN_AUTO_STATE_OFF: return "off";
    case VAZON_FAN_AUTO_STATE_RUNNING: return "running";
    case VAZON_FAN_AUTO_STATE_PAUSE: return "pause";
    case VAZON_FAN_AUTO_STATE_BLOCKED: return "blocked";
    case VAZON_FAN_AUTO_STATE_ALERT: return "alert";
    case VAZON_FAN_AUTO_STATE_UNKNOWN:
    default: return "unknown";
    }
}

static const char *fan_status_text(vazon_fan_status_t value)
{
    switch (value) {
    case VAZON_FAN_STATUS_OK: return "ok";
    case VAZON_FAN_STATUS_WARNING: return "warning";
    case VAZON_FAN_STATUS_ERROR: return "error";
    case VAZON_FAN_STATUS_UNKNOWN:
    default: return "unknown";
    }
}

static const char *fan_reason_text(vazon_fan_status_reason_t value)
{
    switch (value) {
    case VAZON_FAN_STATUS_REASON_DOOR_OPEN: return "door_open";
    case VAZON_FAN_STATUS_REASON_DOOR_UNKNOWN: return "door_unknown";
    case VAZON_FAN_STATUS_REASON_CLIMATE_INVALID: return "climate_invalid";
    case VAZON_FAN_STATUS_REASON_DAY_WINDOW_UNKNOWN: return "day_window_unknown";
    case VAZON_FAN_STATUS_REASON_OUTPUT_FAILED: return "output_failed";
    case VAZON_FAN_STATUS_REASON_CONFIRMATION_FAILED: return "confirmation_failed";
    case VAZON_FAN_STATUS_REASON_NONE:
    default: return NULL;
    }
}

static const char *humidifier_water_text(vazon_humidifier_water_status_t value)
{
    switch (value) {
    case VAZON_HUMIDIFIER_WATER_EMPTY: return "empty";
    case VAZON_HUMIDIFIER_WATER_PRESENT: return "present";
    case VAZON_HUMIDIFIER_WATER_UNKNOWN:
    default: return "unknown";
    }
}

static const char *humidifier_output_text(vazon_humidifier_output_t value)
{
    switch (value) {
    case VAZON_HUMIDIFIER_OUTPUT_OFF: return "off";
    case VAZON_HUMIDIFIER_OUTPUT_ON: return "on";
    case VAZON_HUMIDIFIER_OUTPUT_UNKNOWN:
    default: return "unknown";
    }
}

static const char *humidifier_fan_text(vazon_humidifier_fan_output_t value)
{
    switch (value) {
    case VAZON_HUMIDIFIER_FAN_OFF: return "off";
    case VAZON_HUMIDIFIER_FAN_ON: return "on";
    case VAZON_HUMIDIFIER_FAN_POST_RUN: return "post_run";
    case VAZON_HUMIDIFIER_FAN_UNKNOWN:
    default: return "unknown";
    }
}

static const char *humidifier_status_text(vazon_humidifier_status_t value)
{
    switch (value) {
    case VAZON_HUMIDIFIER_STATUS_OK: return "ok";
    case VAZON_HUMIDIFIER_STATUS_WARNING: return "warning";
    case VAZON_HUMIDIFIER_STATUS_ERROR: return "error";
    case VAZON_HUMIDIFIER_STATUS_INACTIVE: return "inactive";
    case VAZON_HUMIDIFIER_STATUS_UNKNOWN:
    default: return "unknown";
    }
}

static const char *humidifier_reason_text(vazon_humidifier_status_reason_t value)
{
    switch (value) {
    case VAZON_HUMIDIFIER_STATUS_REASON_DOOR_OPEN: return "door_open";
    case VAZON_HUMIDIFIER_STATUS_REASON_DOOR_UNKNOWN: return "door_unknown";
    case VAZON_HUMIDIFIER_STATUS_REASON_NO_WATER: return "no_water";
    case VAZON_HUMIDIFIER_STATUS_REASON_WATER_UNKNOWN: return "water_unknown";
    case VAZON_HUMIDIFIER_STATUS_REASON_DAY_WINDOW_UNKNOWN: return "day_window_unknown";
    case VAZON_HUMIDIFIER_STATUS_REASON_CLIMATE_INVALID: return "climate_invalid";
    case VAZON_HUMIDIFIER_STATUS_REASON_OUTPUT_FAILED: return "output_failed";
    case VAZON_HUMIDIFIER_STATUS_REASON_NONE:
    default: return NULL;
    }
}

static esp_err_t publish_status_payload(vazon_mqtt_state_publisher_t *publisher,
                                        topic_slot_t slot,
                                        const char *entity,
                                        const char *status,
                                        const char *reason,
                                        bool force)
{
    char payload[MQTT_STATE_PAYLOAD_SIZE];
    const int written = snprintf(
        payload, sizeof(payload),
        "{\"status\":\"%s\",\"status_reason\":%s%s%s}", status,
        reason == NULL ? "" : "\"", reason == NULL ? "null" : reason,
        reason == NULL ? "" : "\"");
    if (written < 0 || (size_t)written >= sizeof(payload)) {
        return ESP_ERR_INVALID_SIZE;
    }
    return publish_payload(publisher, slot, "status", entity, payload, force);
}

static uint32_t payload_hash(const char *payload)
{
    uint32_t hash = 2166136261U;
    for (const unsigned char *cursor = (const unsigned char *)payload;
         *cursor != '\0'; ++cursor) {
        hash ^= *cursor;
        hash *= 16777619U;
    }
    return hash;
}

static esp_err_t publish_payload(vazon_mqtt_state_publisher_t *publisher,
                                 topic_slot_t slot,
                                 const char *topic_class,
                                 const char *entity,
                                 const char *payload,
                                 bool force)
{
    const uint32_t bit = 1UL << (unsigned int)slot;
    const uint32_t hash = payload_hash(payload);
    if (!force && (publisher->valid_hash_mask & bit) != 0U &&
        publisher->payload_hashes[slot] == hash) {
        return ESP_OK;
    }

    char topic[MQTT_STATE_TOPIC_SIZE];
    const int written = snprintf(topic, sizeof(topic), "vazon/v3/%s/%s/%s",
                                 publisher->device_id, topic_class, entity);
    if (written < 0 || (size_t)written >= sizeof(topic)) {
        return ESP_ERR_INVALID_SIZE;
    }
    const esp_err_t result = publisher->publish(
        topic, payload, MQTT_STATE_QOS, true, publisher->publish_context);
    if (result == ESP_OK) {
        publisher->payload_hashes[slot] = hash;
        publisher->valid_hash_mask |= bit;
    }
    return result;
}

static esp_err_t publish_system(vazon_mqtt_state_publisher_t *publisher,
                                const vazon_mqtt_state_bundle_t *bundle,
                                bool force)
{
    char payload[MQTT_STATE_PAYLOAD_SIZE];
    const vazon_global_context_t *context = &bundle->global_context;
    int written = snprintf(
        payload, sizeof(payload),
        "{\"maintenance\":{\"active\":%s,\"manual_active\":%s,\"external_active\":%s,\"door_active\":%s,\"reason\":\"%s\"},"
        "\"day_window\":{\"active\":%s,\"schedule_enabled\":%s,\"time_on\":\"%s\",\"time_off\":\"%s\"},"
        "\"connection\":{\"wifi_state\":\"%s\",\"mqtt_state\":\"%s\"},"
        "\"global_interlocks\":{\"maintenance_active\":%s,\"sensor_missing\":%s,\"actuator_failed\":%s}}",
        bool_text(context->maintenance.active),
        bool_text(context->maintenance.manual_active),
        bool_text(context->maintenance.external_active),
        bool_text(context->maintenance.door_active),
        maintenance_reason_text(context->maintenance.reason),
        tristate_text(context->day_window.active),
        bool_text(context->day_window.schedule_enabled),
        context->day_window.time_on, context->day_window.time_off,
        connection_text(context->connection.wifi_state),
        connection_text(context->connection.mqtt_state),
        bool_text(context->global_interlocks.maintenance_active),
        bool_text(context->global_interlocks.sensor_missing),
        bool_text(context->global_interlocks.actuator_failed));
    if (written < 0 || (size_t)written >= sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    esp_err_t result = publish_payload(publisher, TOPIC_SYSTEM_STATE,
                                       "state", "system", payload, force);
    if (result != ESP_OK) return result;

    const vazon_system_status_snapshot_t *status = &bundle->system_status;
    const char *reason = status->status_reason;
    const char *primary = affected_text(status->primary_fault);
    const char *affected = affected_text(status->affected_system);
    written = snprintf(
        payload, sizeof(payload),
        "{\"status\":\"%s\",\"status_reason\":%s%s%s,\"primary_fault\":%s%s%s,\"affected_system\":%s%s%s,\"sensor_missing\":%s,\"actuator_failed\":%s}",
        system_status_text(status->status),
        reason == NULL || strcmp(reason, "none") == 0 ? "" : "\"",
        reason == NULL || strcmp(reason, "none") == 0 ? "null" : reason,
        reason == NULL || strcmp(reason, "none") == 0 ? "" : "\"",
        primary == NULL ? "" : "\"", primary == NULL ? "null" : primary,
        primary == NULL ? "" : "\"",
        affected == NULL ? "" : "\"", affected == NULL ? "null" : affected,
        affected == NULL ? "" : "\"",
        bool_text(status->sensor_missing), bool_text(status->actuator_failed));
    if (written < 0 || (size_t)written >= sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    result = publish_payload(publisher, TOPIC_SYSTEM_STATUS,
                             "status", "system", payload, force);
    if (result != ESP_OK) return result;

    written = snprintf(payload, sizeof(payload),
                       "{\"schedule_enabled\":%s,\"time_on\":\"%s\",\"time_off\":\"%s\"}",
                       bool_text(context->day_window.schedule_enabled),
                       context->day_window.time_on, context->day_window.time_off);
    if (written < 0 || (size_t)written >= sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    return publish_payload(publisher, TOPIC_SYSTEM_SETTINGS,
                           "settings", "system", payload, force);
}

static void number_or_null(char *buffer, size_t size, bool valid, float value)
{
    if (valid) {
        (void)snprintf(buffer, size, "%.2f", (double)value);
    } else {
        (void)snprintf(buffer, size, "null");
    }
}

static esp_err_t publish_climate(vazon_mqtt_state_publisher_t *publisher,
                                 const vazon_climate_snapshot_t *snapshot,
                                 bool force)
{
    char t44[24], h44[24], t45[24], h45[24], delta_t[24], delta_h[24];
    number_or_null(t44, sizeof(t44), snapshot->sensor_0x44.has_valid_sample,
                   snapshot->sensor_0x44.temperature_c);
    number_or_null(h44, sizeof(h44), snapshot->sensor_0x44.has_valid_sample,
                   snapshot->sensor_0x44.humidity_pct);
    number_or_null(t45, sizeof(t45), snapshot->sensor_0x45.has_valid_sample,
                   snapshot->sensor_0x45.temperature_c);
    number_or_null(h45, sizeof(h45), snapshot->sensor_0x45.has_valid_sample,
                   snapshot->sensor_0x45.humidity_pct);
    number_or_null(delta_t, sizeof(delta_t), snapshot->data_valid,
                   snapshot->temperature_delta_c);
    number_or_null(delta_h, sizeof(delta_h), snapshot->data_valid,
                   snapshot->rh_delta_pct);

    char payload[MQTT_STATE_PAYLOAD_SIZE];
    int written = snprintf(
        payload, sizeof(payload),
        "{\"sensor_0x44\":{\"temperature_c\":%s,\"humidity_pct\":%s,\"status\":\"%s\"},"
        "\"sensor_0x45\":{\"temperature_c\":%s,\"humidity_pct\":%s,\"status\":\"%s\"},"
        "\"rh_delta_pct\":%s,\"temperature_delta_c\":%s,\"data_stale\":%s}",
        t44, h44, climate_sensor_status_text(snapshot->sensor_0x44.status),
        t45, h45, climate_sensor_status_text(snapshot->sensor_0x45.status),
        delta_h, delta_t, bool_text(snapshot->data_stale));
    if (written < 0 || (size_t)written >= sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    esp_err_t result = publish_payload(publisher, TOPIC_CLIMATE_STATE,
                                       "state", "climate", payload, force);
    if (result != ESP_OK) return result;

    const char *reason = climate_reason_text(snapshot->status_reason);
    written = snprintf(payload, sizeof(payload),
                       "{\"status\":\"%s\",\"status_reason\":%s%s%s}",
                       climate_status_text(snapshot->status),
                       reason == NULL ? "" : "\"", reason == NULL ? "null" : reason,
                       reason == NULL ? "" : "\"");
    if (written < 0 || (size_t)written >= sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    result = publish_payload(publisher, TOPIC_CLIMATE_STATUS,
                             "status", "climate", payload, force);
    if (result != ESP_OK) return result;

    const vazon_climate_settings_t *settings = &snapshot->settings;
    written = snprintf(
        payload, sizeof(payload),
        "{\"temperature_low_warn\":%.2f,\"temperature_high_warn\":%.2f,\"humidity_low_warn\":%.2f,\"humidity_high_warn\":%.2f,\"sht31_stale_timeout_sec\":%lu,\"temperature_delta_warn\":%.2f,\"temperature_delta_error\":%.2f}",
        (double)settings->temperature_low_warn,
        (double)settings->temperature_high_warn,
        (double)settings->humidity_low_warn,
        (double)settings->humidity_high_warn,
        (unsigned long)settings->sht31_stale_timeout_sec,
        (double)settings->temperature_delta_warn,
        (double)settings->temperature_delta_error);
    if (written < 0 || (size_t)written >= sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    return publish_payload(publisher, TOPIC_CLIMATE_SETTINGS,
                           "settings", "climate", payload, force);
}

static esp_err_t publish_door(vazon_mqtt_state_publisher_t *publisher,
                              const vazon_door_snapshot_t *snapshot,
                              bool force)
{
    char payload[MQTT_STATE_PAYLOAD_SIZE];
    int written = snprintf(payload, sizeof(payload),
                           "{\"state\":\"%s\"}",
                           door_state_text(snapshot->state));
    if (written < 0 || (size_t)written >= sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    esp_err_t result = publish_payload(publisher, TOPIC_DOOR_STATE,
                                       "state", "door", payload, force);
    if (result != ESP_OK) return result;

    const char *reason = snapshot->status_reason ==
                                 VAZON_DOOR_STATUS_REASON_DOOR_UNSTABLE
                             ? "door_unstable"
                             : NULL;
    written = snprintf(payload, sizeof(payload),
                       "{\"status\":\"%s\",\"status_reason\":%s%s%s}",
                       door_status_text(snapshot->status),
                       reason == NULL ? "" : "\"", reason == NULL ? "null" : reason,
                       reason == NULL ? "" : "\"");
    if (written < 0 || (size_t)written >= sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    return publish_payload(publisher, TOPIC_DOOR_STATUS,
                           "status", "door", payload, force);
}

static esp_err_t publish_pot(vazon_mqtt_state_publisher_t *publisher,
                             const vazon_pot_snapshot_t *snapshot,
                             unsigned int pot_id,
                             bool force)
{
    const topic_slot_t state_slot = pot_id == 0U ? TOPIC_POT_0_STATE
                                                 : TOPIC_POT_1_STATE;
    const topic_slot_t status_slot = pot_id == 0U ? TOPIC_POT_0_STATUS
                                                  : TOPIC_POT_1_STATUS;
    const topic_slot_t settings_slot = pot_id == 0U ? TOPIC_POT_0_SETTINGS
                                                    : TOPIC_POT_1_SETTINGS;
    const char *entity = pot_id == 0U ? "pot/0" : "pot/1";
    char moisture_value[24];
    if (snapshot->soil_moisture.value_pct == VAZON_POT_VALUE_UNKNOWN) {
        (void)snprintf(moisture_value, sizeof(moisture_value), "null");
    } else {
        (void)snprintf(moisture_value, sizeof(moisture_value), "%d",
                       snapshot->soil_moisture.value_pct);
    }
    char temperature[24];
    number_or_null(temperature, sizeof(temperature),
                   snapshot->soil_temperature.present &&
                       (snapshot->soil_temperature.status == VAZON_POT_SENSOR_OK ||
                        snapshot->soil_temperature.status == VAZON_POT_SENSOR_WARNING),
                   snapshot->soil_temperature.temperature_c);

    char payload[MQTT_STATE_PAYLOAD_SIZE];
    int written = snprintf(
        payload, sizeof(payload),
        "{\"soil_moisture\":{\"value\":%s,\"class\":\"%s\",\"status\":\"%s\"},"
        "\"soil_temperature\":{\"temperature_c\":%s,\"status\":\"%s\"}}",
        moisture_value, pot_class_text(snapshot->soil_moisture.moisture_class),
        pot_sensor_status_text(snapshot->soil_moisture.status),
        temperature, pot_sensor_status_text(snapshot->soil_temperature.status));
    if (written < 0 || (size_t)written >= sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    esp_err_t result = publish_payload(publisher, state_slot, "state", entity,
                                       payload, force);
    if (result != ESP_OK) return result;

    const char *reason = pot_reason_text(snapshot->status_reason);
    written = snprintf(payload, sizeof(payload),
                       "{\"status\":\"%s\",\"status_reason\":%s%s%s}",
                       pot_status_text(snapshot->status),
                       reason == NULL ? "" : "\"", reason == NULL ? "null" : reason,
                       reason == NULL ? "" : "\"");
    if (written < 0 || (size_t)written >= sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    result = publish_payload(publisher, status_slot, "status", entity,
                             payload, force);
    if (result != ESP_OK) return result;

    const vazon_pot_settings_t *settings = &snapshot->settings;
    written = snprintf(
        payload, sizeof(payload),
        "{\"soil_moisture_enabled\":%s,\"soil_temperature_enabled\":%s,\"moisture_stale_timeout_sec\":%lu,\"temperature_stale_timeout_sec\":%lu,\"temperature_low_warn_c\":%.2f,\"temperature_high_warn_c\":%.2f}",
        bool_text(settings->soil_moisture_enabled),
        bool_text(settings->soil_temperature_enabled),
        (unsigned long)settings->moisture_stale_timeout_sec,
        (unsigned long)settings->temperature_stale_timeout_sec,
        (double)settings->temperature_low_warn_c,
        (double)settings->temperature_high_warn_c);
    if (written < 0 || (size_t)written >= sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    return publish_payload(publisher, settings_slot, "settings", entity,
                           payload, force);
}

static esp_err_t publish_light(vazon_mqtt_state_publisher_t *publisher,
                               const vazon_light_snapshot_t *snapshot,
                               bool force)
{
    char payload[MQTT_STATE_PAYLOAD_SIZE];
    int written = snprintf(
        payload, sizeof(payload),
        "{\"output\":\"%s\",\"output_request\":\"%s\",\"last_command_result\":\"%s\",\"last_output_confirmed\":\"%s\"}",
        light_output_text(snapshot->output),
        light_output_text(snapshot->output_request),
        command_result_text(snapshot->last_command_result),
        output_confirmation_text(snapshot->last_output_confirmed));
    if (written < 0 || (size_t)written >= sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    esp_err_t result = publish_payload(publisher, TOPIC_LIGHT_STATE,
                                       "state", "light", payload, force);
    if (result != ESP_OK) return result;
    result = publish_status_payload(
        publisher, TOPIC_LIGHT_STATUS, "light",
        light_status_text(snapshot->status),
        light_reason_text(snapshot->status_reason), force);
    if (result != ESP_OK) return result;

    written = snprintf(payload, sizeof(payload),
                       "{\"mode\":\"%s\",\"manual_state\":\"%s\"}",
                       snapshot->settings.mode == VAZON_LIGHT_MODE_MANUAL
                           ? "manual" : "auto",
                       snapshot->settings.manual_state ? "on" : "off");
    if (written < 0 || (size_t)written >= sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    return publish_payload(publisher, TOPIC_LIGHT_SETTINGS,
                           "settings", "light", payload, force);
}

static esp_err_t publish_fan(vazon_mqtt_state_publisher_t *publisher,
                             const vazon_fan_snapshot_t *snapshot,
                             bool force)
{
    char applied_power[16];
    if (snapshot->applied_power_pct == VAZON_FAN_POWER_UNKNOWN) {
        (void)snprintf(applied_power, sizeof(applied_power), "null");
    } else {
        (void)snprintf(applied_power, sizeof(applied_power), "%d",
                       snapshot->applied_power_pct);
    }
    char rpm[16];
    if (snapshot->rpm == VAZON_FAN_RPM_UNKNOWN) {
        (void)snprintf(rpm, sizeof(rpm), "null");
    } else {
        (void)snprintf(rpm, sizeof(rpm), "%ld", (long)snapshot->rpm);
    }

    char payload[MQTT_STATE_PAYLOAD_SIZE];
    int written = snprintf(
        payload, sizeof(payload),
        "{\"output\":\"%s\",\"applied_power_pct\":%s,\"rpm\":%s,\"auto_state\":\"%s\",\"last_command_result\":\"%s\",\"last_output_confirmed\":\"%s\"}",
        fan_output_text(snapshot->output), applied_power, rpm,
        fan_auto_state_text(snapshot->auto_state),
        command_result_text(snapshot->last_command_result),
        output_confirmation_text(snapshot->last_output_confirmed));
    if (written < 0 || (size_t)written >= sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    esp_err_t result = publish_payload(publisher, TOPIC_FAN_STATE,
                                       "state", "fan", payload, force);
    if (result != ESP_OK) return result;
    result = publish_status_payload(
        publisher, TOPIC_FAN_STATUS, "fan",
        fan_status_text(snapshot->status),
        fan_reason_text(snapshot->status_reason), force);
    if (result != ESP_OK) return result;

    const vazon_fan_settings_t *settings = &snapshot->settings;
    written = snprintf(
        payload, sizeof(payload),
        "{\"mode\":\"%s\",\"runtime\":\"%s\",\"auto_strategy\":\"%s\",\"manual_duration_sec\":%lu,\"auto_delta_on_pct\":%.2f,\"auto_delta_off_pct\":%.2f,\"auto_timer_on_sec\":%lu,\"auto_timer_off_sec\":%lu,\"power_level_pct\":%u}",
        settings->mode == VAZON_FAN_MODE_MANUAL ? "manual" : "auto",
        settings->runtime == VAZON_FAN_RUNTIME_ALWAYS ? "always" : "day",
        settings->auto_strategy == VAZON_FAN_AUTO_STRATEGY_TIMER
            ? "timer" : "delta",
        (unsigned long)settings->manual_duration_sec,
        (double)settings->auto_delta_on_pct,
        (double)settings->auto_delta_off_pct,
        (unsigned long)settings->auto_timer_on_sec,
        (unsigned long)settings->auto_timer_off_sec,
        (unsigned int)settings->power_level_pct);
    if (written < 0 || (size_t)written >= sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    return publish_payload(publisher, TOPIC_FAN_SETTINGS,
                           "settings", "fan", payload, force);
}

static esp_err_t publish_humidifier(
    vazon_mqtt_state_publisher_t *publisher,
    const vazon_humidifier_snapshot_t *snapshot,
    bool force)
{
    char payload[MQTT_STATE_PAYLOAD_SIZE];
    int written = snprintf(
        payload, sizeof(payload),
        "{\"water_status\":\"%s\",\"mist_output\":\"%s\",\"fan_output\":\"%s\"}",
        humidifier_water_text(snapshot->water_status),
        humidifier_output_text(snapshot->mist_output),
        humidifier_fan_text(snapshot->fan_output));
    if (written < 0 || (size_t)written >= sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    esp_err_t result = publish_payload(publisher, TOPIC_HUMIDIFIER_STATE,
                                       "state", "humidifier", payload, force);
    if (result != ESP_OK) return result;
    result = publish_status_payload(
        publisher, TOPIC_HUMIDIFIER_STATUS, "humidifier",
        humidifier_status_text(snapshot->status),
        humidifier_reason_text(snapshot->status_reason), force);
    if (result != ESP_OK) return result;

    const vazon_humidifier_settings_t *settings = &snapshot->settings;
    const char *mist_power =
        settings->mist_power_level == VAZON_HUMIDIFIER_MIST_POWER_LOW
            ? "low"
            : settings->mist_power_level == VAZON_HUMIDIFIER_MIST_POWER_HIGH
                  ? "high" : "medium";
    written = snprintf(
        payload, sizeof(payload),
        "{\"mode\":\"%s\",\"runtime\":\"%s\",\"rh_start\":%.2f,\"rh_stop\":%.2f,\"rh_delta\":%.2f,\"mist_power_level\":\"%s\",\"manual_mist_duration_sec\":%lu,\"post_fan_sec\":%lu}",
        settings->mode == VAZON_HUMIDIFIER_MODE_MANUAL ? "manual" : "auto",
        settings->runtime == VAZON_HUMIDIFIER_RUNTIME_ALWAYS ? "always" : "day",
        (double)settings->rh_start, (double)settings->rh_stop,
        (double)settings->rh_delta, mist_power,
        (unsigned long)settings->manual_mist_duration_sec,
        (unsigned long)settings->post_fan_sec);
    if (written < 0 || (size_t)written >= sizeof(payload)) return ESP_ERR_INVALID_SIZE;
    return publish_payload(publisher, TOPIC_HUMIDIFIER_SETTINGS,
                           "settings", "humidifier", payload, force);
}

esp_err_t vazon_mqtt_state_publisher_init(
    vazon_mqtt_state_publisher_t *publisher,
    const char *device_id,
    vazon_mqtt_state_publish_fn publish,
    void *publish_context)
{
    if (publisher == NULL || device_id == NULL || device_id[0] == '\0' ||
        publish == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    const size_t device_id_length = strnlen(
        device_id, VAZON_MQTT_STATE_DEVICE_ID_SIZE);
    if (device_id_length >= VAZON_MQTT_STATE_DEVICE_ID_SIZE) {
        return ESP_ERR_INVALID_SIZE;
    }

    memset(publisher, 0, sizeof(*publisher));
    memcpy(publisher->device_id, device_id, device_id_length + 1U);
    publisher->publish = publish;
    publisher->publish_context = publish_context;
    publisher->initialized = true;
    return ESP_OK;
}

esp_err_t vazon_mqtt_state_publisher_publish(
    vazon_mqtt_state_publisher_t *publisher,
    const vazon_mqtt_state_bundle_t *bundle,
    bool force)
{
    if (publisher == NULL || !publisher->initialized || bundle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t result = publish_system(publisher, bundle, force);
    if (result == ESP_OK) result = publish_climate(publisher, &bundle->climate, force);
    if (result == ESP_OK) result = publish_door(publisher, &bundle->door, force);
    if (result == ESP_OK) result = publish_pot(publisher, &bundle->pots[0], 0U, force);
    if (result == ESP_OK) result = publish_pot(publisher, &bundle->pots[1], 1U, force);
    if (result == ESP_OK) result = publish_light(publisher, &bundle->light, force);
    if (result == ESP_OK) result = publish_fan(publisher, &bundle->fan, force);
    if (result == ESP_OK) {
        result = publish_humidifier(publisher, &bundle->humidifier, force);
    }
    return result;
}
