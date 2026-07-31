#pragma once

#include "esp_err.h"
#include "vazon_climate_module.h"
#include "vazon_door_module.h"
#include "vazon_fan_module.h"
#include "vazon_humidifier_module.h"
#include "vazon_light_module.h"
#include "vazon_pot_module.h"
#include "vazon_runtime_core.h"
#include "vazon_system_status.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VAZON_MQTT_STATE_DEVICE_ID_SIZE 48U
#define VAZON_MQTT_STATE_TOPIC_COUNT    23U

typedef esp_err_t (*vazon_mqtt_state_publish_fn)(
    const char *topic, const char *payload, int qos, bool retain,
    void *context);

typedef struct {
    vazon_global_context_t global_context;
    vazon_system_status_snapshot_t system_status;
    vazon_climate_snapshot_t climate;
    vazon_door_snapshot_t door;
    vazon_light_snapshot_t light;
    vazon_fan_snapshot_t fan;
    vazon_humidifier_snapshot_t humidifier;
    vazon_pot_snapshot_t pots[2];
} vazon_mqtt_state_bundle_t;

typedef struct {
    char device_id[VAZON_MQTT_STATE_DEVICE_ID_SIZE];
    vazon_mqtt_state_publish_fn publish;
    void *publish_context;
    uint32_t payload_hashes[VAZON_MQTT_STATE_TOPIC_COUNT];
    uint32_t valid_hash_mask;
    bool initialized;
} vazon_mqtt_state_publisher_t;

esp_err_t vazon_mqtt_state_publisher_init(
    vazon_mqtt_state_publisher_t *publisher,
    const char *device_id,
    vazon_mqtt_state_publish_fn publish,
    void *publish_context);

esp_err_t vazon_mqtt_state_publisher_publish(
    vazon_mqtt_state_publisher_t *publisher,
    const vazon_mqtt_state_bundle_t *bundle,
    bool force);

#ifdef __cplusplus
}
#endif
