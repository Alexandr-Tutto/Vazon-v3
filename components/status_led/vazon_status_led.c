#include "vazon_status_led.h"

#include <stddef.h>
#include <string.h>

#define STATUS_LED_SHORT_CYCLE_MS 2000ULL
#define STATUS_LED_SHORT_ON_MS     200ULL
#define STATUS_LED_EVEN_CYCLE_MS  1000ULL
#define STATUS_LED_EVEN_ON_MS      500ULL

static bool connection_is_offline(vazon_connection_state_t state)
{
    return state == VAZON_CONNECTION_DISCONNECTED ||
           state == VAZON_CONNECTION_CONNECTING;
}

static bool is_mqtt_only_error(const vazon_system_status_snapshot_t *status)
{
    return status->status == VAZON_SYSTEM_STATUS_ERROR &&
           status->affected_system == VAZON_SYSTEM_AFFECTED_CONNECTION &&
           status->status_reason != NULL &&
           strcmp(status->status_reason, "mqtt_offline") == 0;
}

static vazon_status_led_pattern_t select_pattern(
    const vazon_status_led_inputs_t *inputs)
{
    const vazon_system_status_snapshot_t *status = inputs->system_status;
    const bool mqtt_only_error = is_mqtt_only_error(status);

    if (inputs->provisioning_active) {
        return VAZON_STATUS_LED_PATTERN_PROVISIONING;
    }
    if (status->status == VAZON_SYSTEM_STATUS_ERROR && !mqtt_only_error) {
        return VAZON_STATUS_LED_PATTERN_SYSTEM_ERROR;
    }
    if (status->status == VAZON_SYSTEM_STATUS_WARNING &&
        status->affected_system != VAZON_SYSTEM_AFFECTED_CONNECTION) {
        return VAZON_STATUS_LED_PATTERN_LOCAL_WARNING;
    }
    if (mqtt_only_error || connection_is_offline(inputs->mqtt_state)) {
        return VAZON_STATUS_LED_PATTERN_MQTT_OFFLINE;
    }
    if (connection_is_offline(inputs->wifi_state)) {
        return VAZON_STATUS_LED_PATTERN_WIFI_OFFLINE;
    }
    if (status->status == VAZON_SYSTEM_STATUS_OK) {
        return VAZON_STATUS_LED_PATTERN_NORMAL;
    }
    return VAZON_STATUS_LED_PATTERN_UNKNOWN;
}

static void pattern_outputs(vazon_status_led_pattern_t pattern,
                            uint64_t now_ms,
                            bool *green_on,
                            bool *red_on)
{
    *green_on = false;
    *red_on = false;

    switch (pattern) {
    case VAZON_STATUS_LED_PATTERN_NORMAL:
        *green_on = now_ms % STATUS_LED_SHORT_CYCLE_MS < STATUS_LED_SHORT_ON_MS;
        break;
    case VAZON_STATUS_LED_PATTERN_PROVISIONING:
        *green_on = now_ms % STATUS_LED_EVEN_CYCLE_MS < STATUS_LED_EVEN_ON_MS;
        break;
    case VAZON_STATUS_LED_PATTERN_SYSTEM_ERROR:
        *red_on = true;
        break;
    case VAZON_STATUS_LED_PATTERN_LOCAL_WARNING:
        *green_on = now_ms % STATUS_LED_EVEN_CYCLE_MS < STATUS_LED_EVEN_ON_MS;
        *red_on = !*green_on;
        break;
    case VAZON_STATUS_LED_PATTERN_MQTT_OFFLINE:
        *red_on = now_ms % STATUS_LED_EVEN_CYCLE_MS < STATUS_LED_EVEN_ON_MS;
        break;
    case VAZON_STATUS_LED_PATTERN_WIFI_OFFLINE:
        *red_on = now_ms % STATUS_LED_SHORT_CYCLE_MS < STATUS_LED_SHORT_ON_MS;
        break;
    case VAZON_STATUS_LED_PATTERN_UNKNOWN:
    default:
        break;
    }
}

esp_err_t vazon_status_led_init(vazon_status_led_t *module,
                                vazon_status_led_apply_fn apply,
                                void *apply_context)
{
    if (module == NULL || apply == NULL) return ESP_ERR_INVALID_ARG;

    *module = (vazon_status_led_t){
        .snapshot = {
            .pattern = VAZON_STATUS_LED_PATTERN_UNKNOWN,
            .green_on = false,
            .red_on = false,
        },
        .apply = apply,
        .apply_context = apply_context,
        .initialized = true,
    };
    return ESP_OK;
}

esp_err_t vazon_status_led_evaluate(vazon_status_led_t *module,
                                    const vazon_status_led_inputs_t *inputs,
                                    uint64_t now_ms)
{
    if (module == NULL || !module->initialized || inputs == NULL ||
        inputs->system_status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const vazon_status_led_pattern_t pattern = select_pattern(inputs);
    bool green_on = false;
    bool red_on = false;
    pattern_outputs(pattern, now_ms, &green_on, &red_on);

    if (green_on != module->snapshot.green_on ||
        red_on != module->snapshot.red_on) {
        const esp_err_t result =
            module->apply(green_on, red_on, module->apply_context);
        if (result != ESP_OK) return result;
        module->snapshot.green_on = green_on;
        module->snapshot.red_on = red_on;
    }
    module->snapshot.pattern = pattern;
    return ESP_OK;
}

const vazon_status_led_snapshot_t *vazon_status_led_get_snapshot(
    const vazon_status_led_t *module)
{
    return module != NULL && module->initialized ? &module->snapshot : NULL;
}
