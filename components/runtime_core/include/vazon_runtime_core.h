#pragma once

#include "esp_err.h"
#include "vazon_command_router.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VAZON_RUNTIME_TIME_TEXT_SIZE 6
#define VAZON_COMMAND_TARGET_SYSTEM "system"
#define VAZON_COMMAND_SYSTEM_SET_DAY_WINDOW "set_day_window"
#define VAZON_COMMAND_SYSTEM_SET_MAINTENANCE "set_maintenance"

typedef enum {
    VAZON_TRISTATE_UNKNOWN = -1,
    VAZON_TRISTATE_FALSE = 0,
    VAZON_TRISTATE_TRUE = 1,
} vazon_tristate_t;

typedef enum {
    VAZON_MAINTENANCE_REASON_UNKNOWN = 0,
    VAZON_MAINTENANCE_REASON_MANUAL,
    VAZON_MAINTENANCE_REASON_EXTERNAL,
    VAZON_MAINTENANCE_REASON_DOOR,
} vazon_maintenance_reason_t;

typedef enum {
    VAZON_CONNECTION_UNKNOWN = 0,
    VAZON_CONNECTION_DISCONNECTED,
    VAZON_CONNECTION_CONNECTING,
    VAZON_CONNECTION_CONNECTED,
} vazon_connection_state_t;

typedef struct {
    bool active;
    bool manual_active;
    bool external_active;
    bool door_active;
    vazon_maintenance_reason_t reason;
} vazon_maintenance_t;

typedef struct {
    vazon_tristate_t active;
    bool schedule_enabled;
    char time_on[VAZON_RUNTIME_TIME_TEXT_SIZE];
    char time_off[VAZON_RUNTIME_TIME_TEXT_SIZE];
} vazon_day_window_t;

typedef struct {
    vazon_connection_state_t wifi_state;
    vazon_connection_state_t mqtt_state;
} vazon_connection_t;

typedef struct {
    bool maintenance_active;
    bool sensor_missing;
    bool actuator_failed;
} vazon_global_interlocks_t;

typedef struct {
    vazon_maintenance_t maintenance;
    vazon_day_window_t day_window;
    vazon_connection_t connection;
    vazon_global_interlocks_t global_interlocks;
} vazon_global_context_t;

typedef struct {
    vazon_global_context_t context;
    bool manual_maintenance_requested;
    bool external_maintenance_requested;
    bool door_service_required;
} vazon_runtime_core_t;

typedef struct {
    bool has_schedule_enabled;
    bool schedule_enabled;
    const char *time_on;
    const char *time_off;
} vazon_day_window_patch_t;

typedef struct {
    bool active;
} vazon_maintenance_command_args_t;

void vazon_runtime_core_init(vazon_runtime_core_t *core);
const vazon_global_context_t *vazon_runtime_core_get_context(const vazon_runtime_core_t *core);

esp_err_t vazon_runtime_core_set_maintenance(vazon_runtime_core_t *core,
                                             bool active,
                                             vazon_maintenance_reason_t reason);
esp_err_t vazon_runtime_core_set_door_service_required(vazon_runtime_core_t *core,
                                                       bool required);
esp_err_t vazon_runtime_core_set_day_window(vazon_runtime_core_t *core,
                                            const vazon_day_window_patch_t *patch);
esp_err_t vazon_runtime_core_evaluate_day_window(vazon_runtime_core_t *core,
                                                 bool clock_valid,
                                                 uint8_t hour,
                                                 uint8_t minute);
esp_err_t vazon_runtime_core_set_connection(vazon_runtime_core_t *core,
                                            vazon_connection_state_t wifi_state,
                                            vazon_connection_state_t mqtt_state);
esp_err_t vazon_runtime_core_update_interlocks(vazon_runtime_core_t *core,
                                               bool sensor_missing,
                                               bool actuator_failed);
esp_err_t vazon_runtime_core_register_command_target(vazon_runtime_core_t *core,
                                                     vazon_command_router_t *router);

#ifdef __cplusplus
}
#endif
