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

#define VAZON_FAN_POWER_UNKNOWN (-1)
#define VAZON_COMMAND_TARGET_FAN "fan"
#define VAZON_COMMAND_FAN_SET_MODE "set_mode"
#define VAZON_COMMAND_FAN_SET_RUNTIME "set_runtime"
#define VAZON_COMMAND_FAN_SET_AUTO_STRATEGY "set_auto_strategy"
#define VAZON_COMMAND_FAN_SET_POWER_LEVEL "set_power_level"
#define VAZON_COMMAND_FAN_MANUAL_RUN "manual_run"
#define VAZON_COMMAND_FAN_STOP "stop"
#define VAZON_COMMAND_FAN_SET_SETTINGS "set_settings"

typedef enum {
    VAZON_FAN_MODE_AUTO = 0,
    VAZON_FAN_MODE_MANUAL,
} vazon_fan_mode_t;

typedef enum {
    VAZON_FAN_RUNTIME_DAY = 0,
    VAZON_FAN_RUNTIME_ALWAYS,
} vazon_fan_runtime_t;

typedef enum {
    VAZON_FAN_AUTO_STRATEGY_DELTA = 0,
    VAZON_FAN_AUTO_STRATEGY_TIMER,
} vazon_fan_auto_strategy_t;

typedef enum {
    VAZON_FAN_OUTPUT_UNKNOWN = 0,
    VAZON_FAN_OUTPUT_OFF,
    VAZON_FAN_OUTPUT_ON,
} vazon_fan_output_t;

typedef enum {
    VAZON_FAN_AUTO_STATE_UNKNOWN = 0,
    VAZON_FAN_AUTO_STATE_OFF,
    VAZON_FAN_AUTO_STATE_RUNNING,
    VAZON_FAN_AUTO_STATE_PAUSE,
    VAZON_FAN_AUTO_STATE_BLOCKED,
    VAZON_FAN_AUTO_STATE_ALERT,
} vazon_fan_auto_state_t;

typedef enum {
    VAZON_FAN_STATUS_UNKNOWN = 0,
    VAZON_FAN_STATUS_OK,
    VAZON_FAN_STATUS_WARNING,
    VAZON_FAN_STATUS_ERROR,
} vazon_fan_status_t;

typedef enum {
    VAZON_FAN_STATUS_REASON_NONE = 0,
    VAZON_FAN_STATUS_REASON_DOOR_OPEN,
    VAZON_FAN_STATUS_REASON_DOOR_UNKNOWN,
    VAZON_FAN_STATUS_REASON_CLIMATE_INVALID,
    VAZON_FAN_STATUS_REASON_DAY_WINDOW_UNKNOWN,
    VAZON_FAN_STATUS_REASON_OUTPUT_FAILED,
} vazon_fan_status_reason_t;

typedef struct {
    vazon_fan_mode_t mode;
    vazon_fan_runtime_t runtime;
    vazon_fan_auto_strategy_t auto_strategy;
    uint32_t manual_duration_sec;
    float auto_delta_on_pct;
    float auto_delta_off_pct;
    uint32_t auto_timer_on_sec;
    uint32_t auto_timer_off_sec;
    uint8_t power_level_pct;
} vazon_fan_settings_t;

typedef struct {
    bool has_mode;
    vazon_fan_mode_t mode;
    bool has_runtime;
    vazon_fan_runtime_t runtime;
    bool has_auto_strategy;
    vazon_fan_auto_strategy_t auto_strategy;
    bool has_manual_duration_sec;
    uint32_t manual_duration_sec;
    bool has_auto_delta_on_pct;
    float auto_delta_on_pct;
    bool has_auto_delta_off_pct;
    float auto_delta_off_pct;
    bool has_auto_timer_on_sec;
    uint32_t auto_timer_on_sec;
    bool has_auto_timer_off_sec;
    uint32_t auto_timer_off_sec;
    bool has_power_level_pct;
    uint8_t power_level_pct;
} vazon_fan_settings_patch_t;

typedef struct {
    vazon_fan_mode_t mode;
} vazon_fan_set_mode_args_t;

typedef struct {
    vazon_fan_runtime_t runtime;
} vazon_fan_set_runtime_args_t;

typedef struct {
    vazon_fan_auto_strategy_t auto_strategy;
} vazon_fan_set_auto_strategy_args_t;

typedef struct {
    uint8_t power_level_pct;
} vazon_fan_set_power_level_args_t;

typedef struct {
    const vazon_global_context_t *global_context;
    vazon_door_state_t door_state;
    bool climate_delta_valid;
    bool climate_delta_stale;
    float rh_delta_pct;
    uint64_t now_ms;
} vazon_fan_inputs_t;

typedef struct {
    vazon_fan_output_t output;
    vazon_fan_output_t output_request;
    int16_t requested_power_pct;
    int16_t applied_power_pct;
    vazon_fan_auto_state_t auto_state;
    vazon_fan_status_t status;
    vazon_fan_status_reason_t status_reason;
    vazon_command_result_status_t last_command_result;
    vazon_tristate_t last_output_confirmed;
    vazon_fan_settings_t settings;
} vazon_fan_snapshot_t;

typedef enum {
    VAZON_FAN_PWM_PHASE_OFF = 0,
    VAZON_FAN_PWM_PHASE_BOOST,
    VAZON_FAN_PWM_PHASE_RAMP,
    VAZON_FAN_PWM_PHASE_STEADY,
} vazon_fan_pwm_phase_t;

typedef struct {
    vazon_fan_snapshot_t snapshot;
    bool initialized;
    bool has_last_evaluation;
    uint64_t last_evaluation_ms;
    bool delta_demand;
    bool manual_run_active;
    uint64_t manual_deadline_ms;
    bool timer_cycle_valid;
    uint64_t timer_cycle_started_ms;
    bool requested_on;
    bool output_failed;
    vazon_fan_pwm_phase_t pwm_phase;
    uint64_t pwm_phase_started_ms;
    uint8_t pwm_ramp_start_pct;
    uint8_t pwm_target_pct;
} vazon_fan_module_t;

esp_err_t vazon_fan_module_init(vazon_fan_module_t *module,
                                const vazon_fan_settings_t *settings);
esp_err_t vazon_fan_module_validate_settings(const vazon_fan_settings_t *settings);
esp_err_t vazon_fan_module_set_settings(vazon_fan_module_t *module,
                                        const vazon_fan_settings_patch_t *patch);
esp_err_t vazon_fan_module_manual_run(vazon_fan_module_t *module, uint64_t now_ms);
esp_err_t vazon_fan_module_stop(vazon_fan_module_t *module);
esp_err_t vazon_fan_module_evaluate(vazon_fan_module_t *module,
                                    const vazon_fan_inputs_t *inputs);
esp_err_t vazon_fan_module_report_output_result(vazon_fan_module_t *module,
                                                uint8_t applied_power_pct,
                                                esp_err_t driver_result);
esp_err_t vazon_fan_module_register_command_target(
    vazon_fan_module_t *module, vazon_command_router_t *router);
const vazon_fan_snapshot_t *vazon_fan_module_get_snapshot(
    const vazon_fan_module_t *module);

#ifdef __cplusplus
}
#endif
