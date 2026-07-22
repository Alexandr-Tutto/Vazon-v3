#include "vazon_door_module.h"

#include "vazon_gpio_levels.h"

#include <string.h>

static vazon_door_state_t state_from_raw_level(int raw_level)
{
    return raw_level == VAZON_DOOR_CLOSED_LEVEL ? VAZON_DOOR_STATE_CLOSED
                                                : VAZON_DOOR_STATE_OPEN;
}

static void accept_candidate(vazon_door_module_t *module, uint64_t now_ms)
{
    const vazon_door_state_t next_state =
        state_from_raw_level(module->candidate_raw_level);

    if (module->snapshot.state != next_state) {
        module->snapshot.last_change_time_ms = now_ms;
    }

    module->accepted_raw_level = module->candidate_raw_level;
    module->has_accepted_level = true;
    module->transition_active = false;
    module->snapshot.state = next_state;
    module->snapshot.status = VAZON_DOOR_STATUS_OK;
    module->snapshot.status_reason = VAZON_DOOR_STATUS_REASON_NONE;
}

esp_err_t vazon_door_module_init(vazon_door_module_t *module,
                                 const vazon_door_config_t *config)
{
    if (module == NULL || config == NULL || config->debounce_ms == 0U ||
        config->unstable_timeout_ms < config->debounce_ms) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(module, 0, sizeof(*module));
    module->config = *config;
    module->snapshot.state = VAZON_DOOR_STATE_UNKNOWN;
    module->snapshot.status = VAZON_DOOR_STATUS_UNKNOWN;
    module->snapshot.status_reason = VAZON_DOOR_STATUS_REASON_NONE;
    module->initialized = true;
    return ESP_OK;
}

esp_err_t vazon_door_module_update_raw(vazon_door_module_t *module,
                                       int raw_level,
                                       uint64_t now_ms)
{
    if (module == NULL || !module->initialized ||
        (raw_level != VAZON_DOOR_CLOSED_LEVEL &&
         raw_level != VAZON_DOOR_OPEN_LEVEL)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (module->has_last_sample && now_ms < module->last_sample_time_ms) {
        return ESP_ERR_INVALID_ARG;
    }

    module->last_sample_time_ms = now_ms;
    module->has_last_sample = true;

    if (!module->transition_active) {
        if (module->has_accepted_level && raw_level == module->accepted_raw_level) {
            return ESP_OK;
        }

        module->transition_active = true;
        module->transition_since_ms = now_ms;
        module->candidate_since_ms = now_ms;
        module->candidate_raw_level = raw_level;
    } else if (raw_level != module->candidate_raw_level) {
        module->candidate_raw_level = raw_level;
        module->candidate_since_ms = now_ms;
    }

    if (now_ms - module->candidate_since_ms >= module->config.debounce_ms) {
        accept_candidate(module, now_ms);
        return ESP_OK;
    }

    if (now_ms - module->transition_since_ms >= module->config.unstable_timeout_ms) {
        if (module->snapshot.state != VAZON_DOOR_STATE_UNKNOWN) {
            module->snapshot.last_change_time_ms = now_ms;
        }
        module->snapshot.state = VAZON_DOOR_STATE_UNKNOWN;
        module->snapshot.status = VAZON_DOOR_STATUS_WARNING;
        module->snapshot.status_reason = VAZON_DOOR_STATUS_REASON_DOOR_UNSTABLE;
    }

    return ESP_OK;
}

const vazon_door_snapshot_t *vazon_door_module_get_snapshot(
    const vazon_door_module_t *module)
{
    return module == NULL || !module->initialized ? NULL : &module->snapshot;
}
