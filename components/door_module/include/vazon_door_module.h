#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VAZON_DOOR_STATE_UNKNOWN = 0,
    VAZON_DOOR_STATE_CLOSED,
    VAZON_DOOR_STATE_OPEN,
} vazon_door_state_t;

typedef enum {
    VAZON_DOOR_STATUS_UNKNOWN = 0,
    VAZON_DOOR_STATUS_OK,
    VAZON_DOOR_STATUS_WARNING,
    VAZON_DOOR_STATUS_ERROR,
    VAZON_DOOR_STATUS_INACTIVE,
} vazon_door_status_t;

typedef enum {
    VAZON_DOOR_STATUS_REASON_NONE = 0,
    VAZON_DOOR_STATUS_REASON_DOOR_UNSTABLE,
} vazon_door_status_reason_t;

typedef struct {
    uint32_t debounce_ms;
    uint32_t unstable_timeout_ms;
} vazon_door_config_t;

typedef struct {
    vazon_door_state_t state;
    vazon_door_status_t status;
    vazon_door_status_reason_t status_reason;
    uint64_t last_change_time_ms;
} vazon_door_snapshot_t;

typedef struct {
    vazon_door_config_t config;
    vazon_door_snapshot_t snapshot;
    uint64_t last_sample_time_ms;
    uint64_t candidate_since_ms;
    uint64_t transition_since_ms;
    int accepted_raw_level;
    int candidate_raw_level;
    bool has_last_sample;
    bool has_accepted_level;
    bool transition_active;
    bool initialized;
} vazon_door_module_t;

esp_err_t vazon_door_module_init(vazon_door_module_t *module,
                                 const vazon_door_config_t *config);
esp_err_t vazon_door_module_update_raw(vazon_door_module_t *module,
                                       int raw_level,
                                       uint64_t now_ms);
const vazon_door_snapshot_t *vazon_door_module_get_snapshot(
    const vazon_door_module_t *module);

#ifdef __cplusplus
}
#endif
