#pragma once

#include "esp_err.h"
#include "vazon_command_router.h"
#include "vazon_runtime_core.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VAZON_COMMAND_TARGET_LIGHT "light"
#define VAZON_COMMAND_LIGHT_SET_MODE "set_mode"
#define VAZON_COMMAND_LIGHT_SET_MANUAL_STATE "set_manual_state"
#define VAZON_COMMAND_LIGHT_SET_SETTINGS "set_settings"

typedef enum {
    VAZON_LIGHT_MODE_AUTO = 0,
    VAZON_LIGHT_MODE_MANUAL,
} vazon_light_mode_t;

typedef enum {
    VAZON_LIGHT_OUTPUT_UNKNOWN = 0,
    VAZON_LIGHT_OUTPUT_OFF,
    VAZON_LIGHT_OUTPUT_ON,
} vazon_light_output_t;

typedef enum {
    VAZON_LIGHT_STATUS_UNKNOWN = 0,
    VAZON_LIGHT_STATUS_OK,
    VAZON_LIGHT_STATUS_WARNING,
    VAZON_LIGHT_STATUS_ERROR,
    VAZON_LIGHT_STATUS_INACTIVE,
} vazon_light_status_t;

typedef enum {
    VAZON_LIGHT_STATUS_REASON_NONE = 0,
    VAZON_LIGHT_STATUS_REASON_MAINTENANCE_SERVICE_LIGHT,
    VAZON_LIGHT_STATUS_REASON_DAY_WINDOW_UNKNOWN,
    VAZON_LIGHT_STATUS_REASON_OUTPUT_FAILED,
} vazon_light_status_reason_t;

typedef struct {
    vazon_light_mode_t mode;
    bool manual_state;
} vazon_light_settings_t;

typedef struct {
    vazon_light_output_t output;
    vazon_light_output_t output_request;
    vazon_light_status_t status;
    vazon_light_status_reason_t status_reason;
    vazon_command_result_status_t last_command_result;
    vazon_tristate_t last_output_confirmed;
    vazon_light_settings_t settings;
} vazon_light_snapshot_t;

typedef esp_err_t (*vazon_light_output_apply_fn)(bool on, void *context);

typedef struct {
    const vazon_global_context_t *global_context;
    vazon_light_output_apply_fn output_apply;
    void *output_context;
    vazon_light_snapshot_t snapshot;
    bool initialized;
} vazon_light_module_t;

typedef struct {
    vazon_light_mode_t mode;
} vazon_light_set_mode_args_t;

typedef struct {
    bool manual_state;
} vazon_light_set_manual_state_args_t;

typedef struct {
    bool has_mode;
    vazon_light_mode_t mode;
    bool has_manual_state;
    bool manual_state;
} vazon_light_settings_patch_t;

esp_err_t vazon_light_module_init(vazon_light_module_t *module,
                                  const vazon_global_context_t *global_context,
                                  vazon_light_output_apply_fn output_apply,
                                  void *output_context);
esp_err_t vazon_light_module_evaluate(vazon_light_module_t *module);
const vazon_light_snapshot_t *vazon_light_module_get_snapshot(
    const vazon_light_module_t *module);
esp_err_t vazon_light_module_register_command_target(vazon_light_module_t *module,
                                                      vazon_command_router_t *router);

#ifdef __cplusplus
}
#endif
