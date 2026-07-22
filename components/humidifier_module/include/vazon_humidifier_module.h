#pragma once

#include "esp_err.h"
#include "vazon_command_router.h"
#include "vazon_door_module.h"
#include "vazon_runtime_core.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VAZON_COMMAND_TARGET_HUMIDIFIER "humidifier"
#define VAZON_COMMAND_HUMIDIFIER_SET_MODE "set_mode"
#define VAZON_COMMAND_HUMIDIFIER_SET_RUNTIME "set_runtime"
#define VAZON_COMMAND_HUMIDIFIER_MANUAL_MIST "manual_mist"
#define VAZON_COMMAND_HUMIDIFIER_STOP "stop"
#define VAZON_COMMAND_HUMIDIFIER_SET_MIST_POWER_LEVEL \
    "set_mist_power_level"
#define VAZON_COMMAND_HUMIDIFIER_SET_SETTINGS "set_settings"

typedef enum {
    VAZON_HUMIDIFIER_MODE_AUTO = 0,
    VAZON_HUMIDIFIER_MODE_MANUAL,
} vazon_humidifier_mode_t;

typedef enum {
    VAZON_HUMIDIFIER_RUNTIME_DAY = 0,
    VAZON_HUMIDIFIER_RUNTIME_ALWAYS,
} vazon_humidifier_runtime_t;

typedef enum {
    VAZON_HUMIDIFIER_MIST_POWER_LOW = 0,
    VAZON_HUMIDIFIER_MIST_POWER_MEDIUM,
    VAZON_HUMIDIFIER_MIST_POWER_HIGH,
} vazon_humidifier_mist_power_t;

typedef enum {
    VAZON_HUMIDIFIER_WATER_UNKNOWN = 0,
    VAZON_HUMIDIFIER_WATER_EMPTY,
    VAZON_HUMIDIFIER_WATER_PRESENT,
} vazon_humidifier_water_status_t;

typedef enum {
    VAZON_HUMIDIFIER_OUTPUT_UNKNOWN = 0,
    VAZON_HUMIDIFIER_OUTPUT_OFF,
    VAZON_HUMIDIFIER_OUTPUT_ON,
} vazon_humidifier_output_t;

typedef enum {
    VAZON_HUMIDIFIER_FAN_UNKNOWN = 0,
    VAZON_HUMIDIFIER_FAN_OFF,
    VAZON_HUMIDIFIER_FAN_ON,
    VAZON_HUMIDIFIER_FAN_POST_RUN,
} vazon_humidifier_fan_output_t;

typedef enum {
    VAZON_HUMIDIFIER_STATUS_UNKNOWN = 0,
    VAZON_HUMIDIFIER_STATUS_OK,
    VAZON_HUMIDIFIER_STATUS_WARNING,
    VAZON_HUMIDIFIER_STATUS_ERROR,
    VAZON_HUMIDIFIER_STATUS_INACTIVE,
} vazon_humidifier_status_t;

typedef enum {
    VAZON_HUMIDIFIER_STATUS_REASON_NONE = 0,
    VAZON_HUMIDIFIER_STATUS_REASON_DOOR_OPEN,
    VAZON_HUMIDIFIER_STATUS_REASON_DOOR_UNKNOWN,
    VAZON_HUMIDIFIER_STATUS_REASON_NO_WATER,
    VAZON_HUMIDIFIER_STATUS_REASON_WATER_UNKNOWN,
    VAZON_HUMIDIFIER_STATUS_REASON_DAY_WINDOW_UNKNOWN,
    VAZON_HUMIDIFIER_STATUS_REASON_CLIMATE_INVALID,
    VAZON_HUMIDIFIER_STATUS_REASON_OUTPUT_FAILED,
} vazon_humidifier_status_reason_t;

typedef struct {
    vazon_humidifier_mode_t mode;
    vazon_humidifier_runtime_t runtime;
    float rh_start;
    float rh_stop;
    float rh_delta;
    vazon_humidifier_mist_power_t mist_power_level;
    uint32_t manual_mist_duration_sec;
    uint32_t post_fan_sec;
} vazon_humidifier_settings_t;

typedef struct {
    bool has_mode;
    vazon_humidifier_mode_t mode;
    bool has_runtime;
    vazon_humidifier_runtime_t runtime;
    bool has_rh_start;
    float rh_start;
    bool has_rh_stop;
    float rh_stop;
    bool has_rh_delta;
    float rh_delta;
    bool has_mist_power_level;
    vazon_humidifier_mist_power_t mist_power_level;
    bool has_manual_mist_duration_sec;
    uint32_t manual_mist_duration_sec;
    bool has_post_fan_sec;
    uint32_t post_fan_sec;
} vazon_humidifier_settings_patch_t;

typedef struct {
    vazon_humidifier_mode_t mode;
} vazon_humidifier_set_mode_args_t;

typedef struct {
    vazon_humidifier_runtime_t runtime;
} vazon_humidifier_set_runtime_args_t;

typedef struct {
    vazon_humidifier_mist_power_t mist_power_level;
} vazon_humidifier_set_mist_power_args_t;

typedef struct {
    uint32_t water_debounce_ms;
    uint32_t water_unstable_timeout_ms;
} vazon_humidifier_config_t;

typedef struct {
    const vazon_global_context_t *global_context;
    vazon_door_state_t door_state;
    bool water_raw_valid;
    int water_raw_level;
    bool climate_valid;
    bool climate_stale;
    float zone_0_rh_pct;
    float zone_1_rh_pct;
    uint64_t now_ms;
} vazon_humidifier_inputs_t;

typedef struct {
    vazon_humidifier_water_status_t water_status;
    vazon_humidifier_output_t mist_output;
    vazon_humidifier_output_t mist_output_request;
    vazon_humidifier_fan_output_t fan_output;
    vazon_humidifier_fan_output_t fan_output_request;
    vazon_humidifier_status_t status;
    vazon_humidifier_status_reason_t status_reason;
    vazon_command_result_status_t last_command_result;
    vazon_tristate_t last_mist_output_confirmed;
    vazon_tristate_t last_fan_output_confirmed;
    vazon_humidifier_settings_t settings;
} vazon_humidifier_snapshot_t;

typedef struct {
    vazon_humidifier_config_t config;
    vazon_humidifier_snapshot_t snapshot;
    bool initialized;
    bool has_last_evaluation;
    uint64_t last_evaluation_ms;
    bool water_has_accepted_level;
    int water_accepted_level;
    bool water_transition_active;
    int water_candidate_level;
    uint64_t water_candidate_since_ms;
    uint64_t water_transition_since_ms;
    bool water_unstable;
    bool automatic_demand;
    bool manual_mist_active;
    uint64_t manual_mist_deadline_ms;
    bool mist_requested_on;
    bool post_fan_active;
    uint64_t post_fan_deadline_ms;
    bool mist_output_failed;
    bool fan_output_failed;
} vazon_humidifier_module_t;

esp_err_t vazon_humidifier_module_init(
    vazon_humidifier_module_t *module,
    const vazon_humidifier_config_t *config,
    const vazon_humidifier_settings_t *settings);
esp_err_t vazon_humidifier_module_evaluate(
    vazon_humidifier_module_t *module,
    const vazon_humidifier_inputs_t *inputs);
esp_err_t vazon_humidifier_module_manual_mist(
    vazon_humidifier_module_t *module, uint64_t now_ms);
esp_err_t vazon_humidifier_module_stop(vazon_humidifier_module_t *module);
esp_err_t vazon_humidifier_module_set_settings(
    vazon_humidifier_module_t *module,
    const vazon_humidifier_settings_patch_t *patch);
esp_err_t vazon_humidifier_module_report_mist_output(
    vazon_humidifier_module_t *module, bool on, esp_err_t driver_result);
esp_err_t vazon_humidifier_module_report_fan_output(
    vazon_humidifier_module_t *module, bool on, esp_err_t driver_result);
esp_err_t vazon_humidifier_module_register_command_target(
    vazon_humidifier_module_t *module, vazon_command_router_t *router);
const vazon_humidifier_snapshot_t *vazon_humidifier_module_get_snapshot(
    const vazon_humidifier_module_t *module);

#ifdef __cplusplus
}
#endif
