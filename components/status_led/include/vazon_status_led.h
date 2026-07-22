#pragma once

#include "esp_err.h"
#include "vazon_runtime_core.h"
#include "vazon_system_status.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VAZON_STATUS_LED_PATTERN_UNKNOWN = 0,
    VAZON_STATUS_LED_PATTERN_NORMAL,
    VAZON_STATUS_LED_PATTERN_PROVISIONING,
    VAZON_STATUS_LED_PATTERN_SYSTEM_ERROR,
    VAZON_STATUS_LED_PATTERN_LOCAL_WARNING,
    VAZON_STATUS_LED_PATTERN_MQTT_OFFLINE,
    VAZON_STATUS_LED_PATTERN_WIFI_OFFLINE,
} vazon_status_led_pattern_t;

typedef struct {
    bool provisioning_active;
    const vazon_system_status_snapshot_t *system_status;
    vazon_connection_state_t wifi_state;
    vazon_connection_state_t mqtt_state;
} vazon_status_led_inputs_t;

typedef struct {
    vazon_status_led_pattern_t pattern;
    bool green_on;
    bool red_on;
} vazon_status_led_snapshot_t;

typedef esp_err_t (*vazon_status_led_apply_fn)(bool green_on,
                                               bool red_on,
                                               void *context);

typedef struct {
    vazon_status_led_snapshot_t snapshot;
    vazon_status_led_apply_fn apply;
    void *apply_context;
    bool initialized;
} vazon_status_led_t;

esp_err_t vazon_status_led_init(vazon_status_led_t *module,
                                vazon_status_led_apply_fn apply,
                                void *apply_context);
esp_err_t vazon_status_led_evaluate(vazon_status_led_t *module,
                                    const vazon_status_led_inputs_t *inputs,
                                    uint64_t now_ms);
const vazon_status_led_snapshot_t *vazon_status_led_get_snapshot(
    const vazon_status_led_t *module);

#ifdef __cplusplus
}
#endif
