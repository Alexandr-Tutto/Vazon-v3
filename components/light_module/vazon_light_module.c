#include "vazon_light_module.h"

#include <string.h>

static bool mode_valid(vazon_light_mode_t mode)
{
    return mode == VAZON_LIGHT_MODE_AUTO || mode == VAZON_LIGHT_MODE_MANUAL;
}

static vazon_light_output_t requested_output(const vazon_light_module_t *module,
                                              vazon_light_status_t *status,
                                              vazon_light_status_reason_t *reason)
{
    const vazon_global_context_t *global = module->global_context;
    *status = VAZON_LIGHT_STATUS_OK;
    *reason = VAZON_LIGHT_STATUS_REASON_NONE;

    if (global->maintenance.active) {
        *status = VAZON_LIGHT_STATUS_WARNING;
        *reason = VAZON_LIGHT_STATUS_REASON_MAINTENANCE_SERVICE_LIGHT;
        return VAZON_LIGHT_OUTPUT_ON;
    }

    if (module->snapshot.settings.mode == VAZON_LIGHT_MODE_MANUAL) {
        return module->snapshot.settings.manual_state ? VAZON_LIGHT_OUTPUT_ON
                                                      : VAZON_LIGHT_OUTPUT_OFF;
    }

    if (global->day_window.active == VAZON_TRISTATE_UNKNOWN) {
        *status = VAZON_LIGHT_STATUS_WARNING;
        *reason = VAZON_LIGHT_STATUS_REASON_DAY_WINDOW_UNKNOWN;
        return VAZON_LIGHT_OUTPUT_OFF;
    }

    return global->day_window.active == VAZON_TRISTATE_TRUE ? VAZON_LIGHT_OUTPUT_ON
                                                            : VAZON_LIGHT_OUTPUT_OFF;
}

esp_err_t vazon_light_module_init(vazon_light_module_t *module,
                                  const vazon_global_context_t *global_context,
                                  vazon_light_output_apply_fn output_apply,
                                  void *output_context)
{
    if (module == NULL || global_context == NULL || output_apply == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(module, 0, sizeof(*module));
    module->global_context = global_context;
    module->output_apply = output_apply;
    module->output_context = output_context;
    module->snapshot.output = VAZON_LIGHT_OUTPUT_UNKNOWN;
    module->snapshot.output_request = VAZON_LIGHT_OUTPUT_OFF;
    module->snapshot.status = VAZON_LIGHT_STATUS_UNKNOWN;
    module->snapshot.status_reason = VAZON_LIGHT_STATUS_REASON_NONE;
    module->snapshot.last_command_result = VAZON_COMMAND_RESULT_UNKNOWN;
    module->snapshot.last_output_confirmed = VAZON_TRISTATE_UNKNOWN;
    module->snapshot.settings.mode = VAZON_LIGHT_MODE_AUTO;
    module->snapshot.settings.manual_state = false;
    module->initialized = true;
    return ESP_OK;
}

esp_err_t vazon_light_module_evaluate(vazon_light_module_t *module)
{
    if (module == NULL || !module->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    vazon_light_status_t next_status = VAZON_LIGHT_STATUS_OK;
    vazon_light_status_reason_t next_reason = VAZON_LIGHT_STATUS_REASON_NONE;
    const vazon_light_output_t request =
        requested_output(module, &next_status, &next_reason);
    module->snapshot.output_request = request;

    if (module->snapshot.output != request) {
        const esp_err_t apply_result = module->output_apply(
            request == VAZON_LIGHT_OUTPUT_ON, module->output_context);
        if (apply_result != ESP_OK) {
            module->snapshot.status = VAZON_LIGHT_STATUS_ERROR;
            module->snapshot.status_reason = VAZON_LIGHT_STATUS_REASON_OUTPUT_FAILED;
            module->snapshot.last_command_result = VAZON_COMMAND_RESULT_FAILED;
            return apply_result;
        }

        module->snapshot.output = request;
        module->snapshot.last_command_result = VAZON_COMMAND_RESULT_ACCEPTED;
    }

    module->snapshot.status = next_status;
    module->snapshot.status_reason = next_reason;
    module->snapshot.last_output_confirmed = VAZON_TRISTATE_UNKNOWN;
    return ESP_OK;
}

const vazon_light_snapshot_t *vazon_light_module_get_snapshot(
    const vazon_light_module_t *module)
{
    return module == NULL || !module->initialized ? NULL : &module->snapshot;
}

static vazon_command_result_t command_result(vazon_command_result_status_t status,
                                             esp_err_t error)
{
    const vazon_command_result_t result = {
        .status = status,
        .error = error,
    };
    return result;
}

static vazon_command_result_t evaluate_command(vazon_light_module_t *module)
{
    const esp_err_t result = vazon_light_module_evaluate(module);
    return command_result(result == ESP_OK ? VAZON_COMMAND_RESULT_ACCEPTED
                                          : VAZON_COMMAND_RESULT_FAILED,
                          result);
}

static vazon_command_result_t light_command_handler(const vazon_command_t *command,
                                                     void *context)
{
    vazon_light_module_t *module = (vazon_light_module_t *)context;
    if (command->args == NULL) {
        return command_result(VAZON_COMMAND_RESULT_REJECTED, ESP_ERR_INVALID_ARG);
    }

    if (strcmp(command->cmd, VAZON_COMMAND_LIGHT_SET_MODE) == 0) {
        const vazon_light_set_mode_args_t *args = command->args;
        if (!mode_valid(args->mode)) {
            return command_result(VAZON_COMMAND_RESULT_REJECTED, ESP_ERR_INVALID_ARG);
        }
        module->snapshot.settings.mode = args->mode;
        return evaluate_command(module);
    }

    if (strcmp(command->cmd, VAZON_COMMAND_LIGHT_SET_MANUAL_STATE) == 0) {
        const vazon_light_set_manual_state_args_t *args = command->args;
        module->snapshot.settings.manual_state = args->manual_state;
        return evaluate_command(module);
    }

    if (strcmp(command->cmd, VAZON_COMMAND_LIGHT_SET_SETTINGS) == 0) {
        const vazon_light_settings_patch_t *args = command->args;
        if (args->has_mode && !mode_valid(args->mode)) {
            return command_result(VAZON_COMMAND_RESULT_REJECTED, ESP_ERR_INVALID_ARG);
        }

        vazon_light_settings_t settings = module->snapshot.settings;
        if (args->has_mode) settings.mode = args->mode;
        if (args->has_manual_state) settings.manual_state = args->manual_state;
        module->snapshot.settings = settings;
        return evaluate_command(module);
    }

    return command_result(VAZON_COMMAND_RESULT_REJECTED, ESP_ERR_NOT_SUPPORTED);
}

esp_err_t vazon_light_module_register_command_target(vazon_light_module_t *module,
                                                      vazon_command_router_t *router)
{
    if (module == NULL || !module->initialized || router == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return vazon_command_router_register(router, VAZON_COMMAND_TARGET_LIGHT,
                                         light_command_handler, module);
}
