#include "vazon_mqtt_command_codec.h"

#include "cJSON.h"
#include "esp_timer.h"
#include "vazon_climate_module.h"
#include "vazon_fan_module.h"
#include "vazon_humidifier_module.h"
#include "vazon_light_module.h"
#include "vazon_pot_module.h"
#include "vazon_runtime_core.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define CODEC_CMD_ID_SIZE 64
#define CODEC_CMD_SIZE    64
#define CODEC_TARGET_SIZE 32

typedef union {
    vazon_day_window_patch_t day_window;
    vazon_maintenance_command_args_t maintenance;
    vazon_light_set_mode_args_t light_mode;
    vazon_light_set_manual_state_args_t light_manual;
    vazon_light_settings_patch_t light_settings;
    vazon_fan_set_mode_args_t fan_mode;
    vazon_fan_set_runtime_args_t fan_runtime;
    vazon_fan_set_auto_strategy_args_t fan_strategy;
    vazon_fan_set_power_level_args_t fan_power;
    vazon_fan_settings_patch_t fan_settings;
    vazon_humidifier_set_mode_args_t humidifier_mode;
    vazon_humidifier_set_runtime_args_t humidifier_runtime;
    vazon_humidifier_set_mist_power_args_t humidifier_power;
    vazon_humidifier_settings_patch_t humidifier_settings;
    vazon_climate_settings_patch_t climate_settings;
    vazon_pot_set_enabled_args_t pot_enabled;
    vazon_pot_settings_patch_t pot_settings;
    vazon_pot_calibration_args_t pot_calibration;
} decoded_args_t;

static bool safe_token(const char *text, size_t max_length)
{
    if (text == NULL) return false;
    const size_t length = strnlen(text, max_length);
    if (length == 0U || length >= max_length) return false;
    for (size_t index = 0; index < length; ++index) {
        const char value = text[index];
        if (!((value >= 'a' && value <= 'z') ||
              (value >= 'A' && value <= 'Z') ||
              (value >= '0' && value <= '9') || value == '-' || value == '_')) {
            return false;
        }
    }
    return true;
}

static bool target_valid(const char *target)
{
    if (target == NULL || target[0] == '\0') return false;
    const char *segment = target;
    while (*segment != '\0') {
        const char *slash = strchr(segment, '/');
        const size_t length = slash == NULL ? strlen(segment)
                                             : (size_t)(slash - segment);
        if (length == 0U || length >= CODEC_TARGET_SIZE) return false;
        char token[CODEC_TARGET_SIZE];
        memcpy(token, segment, length);
        token[length] = '\0';
        if (!safe_token(token, sizeof(token))) return false;
        if (slash == NULL) return true;
        segment = slash + 1;
    }
    return false;
}

static bool copy_json_string(const cJSON *object, const char *key,
                             char *destination, size_t destination_size)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    if (!cJSON_IsString(item) || item->valuestring == NULL) return false;
    const size_t length = strnlen(item->valuestring, destination_size);
    if (length == 0U || length >= destination_size) return false;
    memcpy(destination, item->valuestring, length + 1U);
    return true;
}

static bool read_bool(const cJSON *args, const char *key, bool *value)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(args, key);
    if (!cJSON_IsBool(item)) return false;
    *value = cJSON_IsTrue(item);
    return true;
}

static bool read_on_off(const cJSON *args, const char *key, bool *value)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(args, key);
    if (!cJSON_IsString(item) || item->valuestring == NULL) return false;
    if (strcmp(item->valuestring, "on") == 0) {
        *value = true;
        return true;
    }
    if (strcmp(item->valuestring, "off") == 0) {
        *value = false;
        return true;
    }
    return false;
}

static bool read_u32(const cJSON *args, const char *key, uint32_t *value)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(args, key);
    if (!cJSON_IsNumber(item) || !isfinite(item->valuedouble) ||
        item->valuedouble < 0.0 || item->valuedouble > 4294967295.0 ||
        floor(item->valuedouble) != item->valuedouble) {
        return false;
    }
    *value = (uint32_t)item->valuedouble;
    return true;
}

static bool read_u8(const cJSON *args, const char *key, uint8_t *value)
{
    uint32_t decoded = 0U;
    if (!read_u32(args, key, &decoded) || decoded > UINT8_MAX) return false;
    *value = (uint8_t)decoded;
    return true;
}

static bool read_float(const cJSON *args, const char *key, float *value)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(args, key);
    if (!cJSON_IsNumber(item) || !isfinite(item->valuedouble)) return false;
    *value = (float)item->valuedouble;
    return isfinite(*value);
}

static bool read_enum(const cJSON *args, const char *key,
                      const char *const *names, size_t name_count, int *value)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(args, key);
    if (!cJSON_IsString(item) || item->valuestring == NULL) return false;
    for (size_t index = 0; index < name_count; ++index) {
        if (strcmp(item->valuestring, names[index]) == 0) {
            *value = (int)index;
            return true;
        }
    }
    return false;
}

static bool args_exact(const cJSON *args, int expected_fields)
{
    return cJSON_IsObject(args) && cJSON_GetArraySize(args) == expected_fields;
}

static bool optional_bool(const cJSON *args, const char *key, bool *has,
                          bool *value, int *field_count)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(args, key);
    if (item == NULL) return true;
    if (!cJSON_IsBool(item)) return false;
    *has = true;
    *value = cJSON_IsTrue(item);
    ++*field_count;
    return true;
}

static bool optional_on_off(const cJSON *args, const char *key, bool *has,
                            bool *value, int *field_count)
{
    if (cJSON_GetObjectItemCaseSensitive(args, key) == NULL) return true;
    if (!read_on_off(args, key, value)) return false;
    *has = true;
    ++*field_count;
    return true;
}

static bool optional_u32(const cJSON *args, const char *key, bool *has,
                         uint32_t *value, int *field_count)
{
    if (cJSON_GetObjectItemCaseSensitive(args, key) == NULL) return true;
    if (!read_u32(args, key, value)) return false;
    *has = true;
    ++*field_count;
    return true;
}

static bool optional_float(const cJSON *args, const char *key, bool *has,
                           float *value, int *field_count)
{
    if (cJSON_GetObjectItemCaseSensitive(args, key) == NULL) return true;
    if (!read_float(args, key, value)) return false;
    *has = true;
    ++*field_count;
    return true;
}

static bool optional_enum(const cJSON *args, const char *key,
                          const char *const *names, size_t name_count,
                          bool *has, int *value, int *field_count)
{
    if (cJSON_GetObjectItemCaseSensitive(args, key) == NULL) return true;
    if (!read_enum(args, key, names, name_count, value)) return false;
    *has = true;
    ++*field_count;
    return true;
}

static esp_err_t decode_system(const char *cmd, const cJSON *args,
                               decoded_args_t *storage, const void **decoded)
{
    if (strcmp(cmd, VAZON_COMMAND_SYSTEM_SET_MAINTENANCE) == 0) {
        if (!args_exact(args, 1) ||
            !read_bool(args, "active", &storage->maintenance.active)) {
            return ESP_ERR_INVALID_ARG;
        }
        *decoded = &storage->maintenance;
        return ESP_OK;
    }
    if (strcmp(cmd, VAZON_COMMAND_SYSTEM_SET_DAY_WINDOW) != 0)
        return ESP_ERR_NOT_SUPPORTED;
    if (!cJSON_IsObject(args)) return ESP_ERR_INVALID_ARG;
    int count = 0;
    vazon_day_window_patch_t *patch = &storage->day_window;
    if (!optional_bool(args, "schedule_enabled", &patch->has_schedule_enabled,
                       &patch->schedule_enabled, &count)) return ESP_ERR_INVALID_ARG;
    const cJSON *time_on = cJSON_GetObjectItemCaseSensitive(args, "time_on");
    if (time_on != NULL) {
        if (!cJSON_IsString(time_on) || time_on->valuestring == NULL) return ESP_ERR_INVALID_ARG;
        patch->time_on = time_on->valuestring;
        ++count;
    }
    const cJSON *time_off = cJSON_GetObjectItemCaseSensitive(args, "time_off");
    if (time_off != NULL) {
        if (!cJSON_IsString(time_off) || time_off->valuestring == NULL) return ESP_ERR_INVALID_ARG;
        patch->time_off = time_off->valuestring;
        ++count;
    }
    if (count == 0 || count != cJSON_GetArraySize(args)) return ESP_ERR_INVALID_ARG;
    *decoded = patch;
    return ESP_OK;
}

static esp_err_t decode_light(const char *cmd, const cJSON *args,
                              decoded_args_t *storage, const void **decoded)
{
    static const char *const modes[] = {"auto", "manual"};
    int value = 0;
    if (strcmp(cmd, VAZON_COMMAND_LIGHT_SET_MODE) == 0) {
        if (!args_exact(args, 1) || !read_enum(args, "mode", modes, 2U, &value)) return ESP_ERR_INVALID_ARG;
        storage->light_mode.mode = (vazon_light_mode_t)value;
        *decoded = &storage->light_mode;
        return ESP_OK;
    }
    if (strcmp(cmd, VAZON_COMMAND_LIGHT_SET_MANUAL_STATE) == 0) {
        if (!args_exact(args, 1) ||
            !read_on_off(args, "manual_state",
                         &storage->light_manual.manual_state)) {
            return ESP_ERR_INVALID_ARG;
        }
        *decoded = &storage->light_manual;
        return ESP_OK;
    }
    if (strcmp(cmd, VAZON_COMMAND_LIGHT_SET_SETTINGS) != 0)
        return ESP_ERR_NOT_SUPPORTED;
    if (!cJSON_IsObject(args)) return ESP_ERR_INVALID_ARG;
    int count = 0;
    vazon_light_settings_patch_t *patch = &storage->light_settings;
    if (!optional_enum(args, "mode", modes, 2U, &patch->has_mode, &value, &count)) return ESP_ERR_INVALID_ARG;
    patch->mode = (vazon_light_mode_t)value;
    if (!optional_on_off(args, "manual_state", &patch->has_manual_state,
                         &patch->manual_state, &count)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (count == 0 || count != cJSON_GetArraySize(args)) return ESP_ERR_INVALID_ARG;
    *decoded = patch;
    return ESP_OK;
}

static esp_err_t decode_fan(const char *cmd, const cJSON *args,
                            decoded_args_t *storage, const void **decoded)
{
    static const char *const modes[] = {"auto", "manual"};
    static const char *const runtimes[] = {"day", "always"};
    static const char *const strategies[] = {"delta", "timer"};
    if (strcmp(cmd, VAZON_COMMAND_FAN_MANUAL_RUN) == 0 ||
        strcmp(cmd, VAZON_COMMAND_FAN_STOP) == 0) {
        return args_exact(args, 0) ? ESP_OK : ESP_ERR_INVALID_ARG;
    }
    int value = 0;
    if (strcmp(cmd, VAZON_COMMAND_FAN_SET_MODE) == 0) {
        if (!args_exact(args, 1) || !read_enum(args, "mode", modes, 2U, &value)) return ESP_ERR_INVALID_ARG;
        storage->fan_mode.mode = (vazon_fan_mode_t)value;
        *decoded = &storage->fan_mode;
        return ESP_OK;
    }
    if (strcmp(cmd, VAZON_COMMAND_FAN_SET_RUNTIME) == 0) {
        if (!args_exact(args, 1) || !read_enum(args, "runtime", runtimes, 2U, &value)) return ESP_ERR_INVALID_ARG;
        storage->fan_runtime.runtime = (vazon_fan_runtime_t)value;
        *decoded = &storage->fan_runtime;
        return ESP_OK;
    }
    if (strcmp(cmd, VAZON_COMMAND_FAN_SET_AUTO_STRATEGY) == 0) {
        if (!args_exact(args, 1) || !read_enum(args, "auto_strategy", strategies, 2U, &value)) return ESP_ERR_INVALID_ARG;
        storage->fan_strategy.auto_strategy = (vazon_fan_auto_strategy_t)value;
        *decoded = &storage->fan_strategy;
        return ESP_OK;
    }
    if (strcmp(cmd, VAZON_COMMAND_FAN_SET_POWER_LEVEL) == 0) {
        if (!args_exact(args, 1) || !read_u8(args, "power_level_pct", &storage->fan_power.power_level_pct)) return ESP_ERR_INVALID_ARG;
        *decoded = &storage->fan_power;
        return ESP_OK;
    }
    if (strcmp(cmd, VAZON_COMMAND_FAN_SET_SETTINGS) != 0)
        return ESP_ERR_NOT_SUPPORTED;
    if (!cJSON_IsObject(args)) return ESP_ERR_INVALID_ARG;
    int count = 0;
    vazon_fan_settings_patch_t *p = &storage->fan_settings;
    if (!optional_enum(args, "mode", modes, 2U, &p->has_mode, &value,
                       &count)) return ESP_ERR_INVALID_ARG;
    p->mode = (vazon_fan_mode_t)value;
    if (!optional_enum(args, "runtime", runtimes, 2U, &p->has_runtime,
                       &value, &count)) return ESP_ERR_INVALID_ARG;
    p->runtime = (vazon_fan_runtime_t)value;
    if (!optional_enum(args, "auto_strategy", strategies, 2U,
                       &p->has_auto_strategy, &value, &count)) {
        return ESP_ERR_INVALID_ARG;
    }
    p->auto_strategy = (vazon_fan_auto_strategy_t)value;
    if (!optional_u32(args, "manual_duration_sec", &p->has_manual_duration_sec, &p->manual_duration_sec, &count) ||
        !optional_float(args, "auto_delta_on_pct", &p->has_auto_delta_on_pct, &p->auto_delta_on_pct, &count) ||
        !optional_float(args, "auto_delta_off_pct", &p->has_auto_delta_off_pct, &p->auto_delta_off_pct, &count) ||
        !optional_u32(args, "auto_timer_on_sec", &p->has_auto_timer_on_sec, &p->auto_timer_on_sec, &count) ||
        !optional_u32(args, "auto_timer_off_sec", &p->has_auto_timer_off_sec, &p->auto_timer_off_sec, &count)) return ESP_ERR_INVALID_ARG;
    if (cJSON_GetObjectItemCaseSensitive(args, "power_level_pct") != NULL) {
        if (!read_u8(args, "power_level_pct", &p->power_level_pct)) return ESP_ERR_INVALID_ARG;
        p->has_power_level_pct = true;
        ++count;
    }
    if (count == 0 || count != cJSON_GetArraySize(args)) return ESP_ERR_INVALID_ARG;
    *decoded = p;
    return ESP_OK;
}

static esp_err_t decode_humidifier(const char *cmd, const cJSON *args,
                                   decoded_args_t *storage, const void **decoded)
{
    static const char *const modes[] = {"auto", "manual"};
    static const char *const runtimes[] = {"day", "always"};
    static const char *const powers[] = {"low", "medium", "high"};
    if (strcmp(cmd, VAZON_COMMAND_HUMIDIFIER_MANUAL_MIST) == 0 ||
        strcmp(cmd, VAZON_COMMAND_HUMIDIFIER_STOP) == 0) {
        return args_exact(args, 0) ? ESP_OK : ESP_ERR_INVALID_ARG;
    }
    int value = 0;
    if (strcmp(cmd, VAZON_COMMAND_HUMIDIFIER_SET_MODE) == 0) {
        if (!args_exact(args, 1) || !read_enum(args, "mode", modes, 2U, &value)) return ESP_ERR_INVALID_ARG;
        storage->humidifier_mode.mode = (vazon_humidifier_mode_t)value;
        *decoded = &storage->humidifier_mode;
        return ESP_OK;
    }
    if (strcmp(cmd, VAZON_COMMAND_HUMIDIFIER_SET_RUNTIME) == 0) {
        if (!args_exact(args, 1) || !read_enum(args, "runtime", runtimes, 2U, &value)) return ESP_ERR_INVALID_ARG;
        storage->humidifier_runtime.runtime = (vazon_humidifier_runtime_t)value;
        *decoded = &storage->humidifier_runtime;
        return ESP_OK;
    }
    if (strcmp(cmd, VAZON_COMMAND_HUMIDIFIER_SET_MIST_POWER_LEVEL) == 0) {
        if (!args_exact(args, 1) || !read_enum(args, "mist_power_level", powers, 3U, &value)) return ESP_ERR_INVALID_ARG;
        storage->humidifier_power.mist_power_level =
            (vazon_humidifier_mist_power_t)value;
        *decoded = &storage->humidifier_power;
        return ESP_OK;
    }
    if (strcmp(cmd, VAZON_COMMAND_HUMIDIFIER_SET_SETTINGS) != 0)
        return ESP_ERR_NOT_SUPPORTED;
    if (!cJSON_IsObject(args)) return ESP_ERR_INVALID_ARG;
    int count = 0;
    vazon_humidifier_settings_patch_t *p = &storage->humidifier_settings;
    if (!optional_enum(args, "mode", modes, 2U, &p->has_mode, &value,
                       &count)) return ESP_ERR_INVALID_ARG;
    p->mode = (vazon_humidifier_mode_t)value;
    if (!optional_enum(args, "runtime", runtimes, 2U, &p->has_runtime,
                       &value, &count)) return ESP_ERR_INVALID_ARG;
    p->runtime = (vazon_humidifier_runtime_t)value;
    if (!optional_float(args, "rh_start", &p->has_rh_start, &p->rh_start, &count) ||
        !optional_float(args, "rh_stop", &p->has_rh_stop, &p->rh_stop, &count) ||
        !optional_float(args, "rh_delta", &p->has_rh_delta, &p->rh_delta, &count) ||
        !optional_u32(args, "manual_mist_duration_sec", &p->has_manual_mist_duration_sec, &p->manual_mist_duration_sec, &count) ||
        !optional_u32(args, "post_fan_sec", &p->has_post_fan_sec, &p->post_fan_sec, &count)) return ESP_ERR_INVALID_ARG;
    if (cJSON_GetObjectItemCaseSensitive(args, "mist_power_level") != NULL) {
        if (!read_enum(args, "mist_power_level", powers, 3U, &value)) return ESP_ERR_INVALID_ARG;
        p->has_mist_power_level = true;
        p->mist_power_level = (vazon_humidifier_mist_power_t)value;
        ++count;
    }
    if (count == 0 || count != cJSON_GetArraySize(args)) return ESP_ERR_INVALID_ARG;
    *decoded = p;
    return ESP_OK;
}

static esp_err_t decode_climate(const char *cmd, const cJSON *args,
                                decoded_args_t *storage, const void **decoded)
{
    if (strcmp(cmd, VAZON_COMMAND_CLIMATE_SET_SETTINGS) != 0)
        return ESP_ERR_NOT_SUPPORTED;
    if (!cJSON_IsObject(args)) return ESP_ERR_INVALID_ARG;
    int count = 0;
    vazon_climate_settings_patch_t *p = &storage->climate_settings;
    if (!optional_float(args, "temperature_low_warn", &p->has_temperature_low_warn, &p->temperature_low_warn, &count) ||
        !optional_float(args, "temperature_high_warn", &p->has_temperature_high_warn, &p->temperature_high_warn, &count) ||
        !optional_float(args, "humidity_low_warn", &p->has_humidity_low_warn, &p->humidity_low_warn, &count) ||
        !optional_float(args, "humidity_high_warn", &p->has_humidity_high_warn, &p->humidity_high_warn, &count) ||
        !optional_u32(args, "sht31_stale_timeout_sec", &p->has_sht31_stale_timeout_sec, &p->sht31_stale_timeout_sec, &count) ||
        !optional_float(args, "temperature_delta_warn", &p->has_temperature_delta_warn, &p->temperature_delta_warn, &count) ||
        !optional_float(args, "temperature_delta_error", &p->has_temperature_delta_error, &p->temperature_delta_error, &count)) return ESP_ERR_INVALID_ARG;
    if (count == 0 || count != cJSON_GetArraySize(args)) return ESP_ERR_INVALID_ARG;
    *decoded = p;
    return ESP_OK;
}

static esp_err_t decode_pot(const char *cmd, const cJSON *args,
                            decoded_args_t *storage, const void **decoded)
{
    if (strcmp(cmd, VAZON_COMMAND_POT_SET_SOIL_MOISTURE_ENABLED) == 0 ||
        strcmp(cmd, VAZON_COMMAND_POT_SET_SOIL_TEMPERATURE_ENABLED) == 0) {
        if (!args_exact(args, 1) || !read_bool(args, "enabled", &storage->pot_enabled.enabled)) return ESP_ERR_INVALID_ARG;
        *decoded = &storage->pot_enabled;
        return ESP_OK;
    }
    if (strcmp(cmd, VAZON_COMMAND_POT_CALIBRATE_SOIL_MOISTURE) == 0) {
        static const char *const points[] = {"dry", "normal", "wet", "reset"};
        int value = 0;
        if (!args_exact(args, 1) || !read_enum(args, "point", points, 4U, &value)) return ESP_ERR_INVALID_ARG;
        storage->pot_calibration.point = (vazon_pot_calibration_point_t)value;
        *decoded = &storage->pot_calibration;
        return ESP_OK;
    }
    if (strcmp(cmd, VAZON_COMMAND_POT_SET_SETTINGS) != 0)
        return ESP_ERR_NOT_SUPPORTED;
    if (!cJSON_IsObject(args)) return ESP_ERR_INVALID_ARG;
    int count = 0;
    vazon_pot_settings_patch_t *p = &storage->pot_settings;
    if (!optional_bool(args, "soil_moisture_enabled", &p->has_soil_moisture_enabled, &p->soil_moisture_enabled, &count) ||
        !optional_bool(args, "soil_temperature_enabled", &p->has_soil_temperature_enabled, &p->soil_temperature_enabled, &count) ||
        !optional_u32(args, "moisture_stale_timeout_sec", &p->has_moisture_stale_timeout_sec, &p->moisture_stale_timeout_sec, &count) ||
        !optional_u32(args, "temperature_stale_timeout_sec", &p->has_temperature_stale_timeout_sec, &p->temperature_stale_timeout_sec, &count) ||
        !optional_float(args, "temperature_low_warn_c", &p->has_temperature_low_warn_c, &p->temperature_low_warn_c, &count) ||
        !optional_float(args, "temperature_high_warn_c", &p->has_temperature_high_warn_c, &p->temperature_high_warn_c, &count)) return ESP_ERR_INVALID_ARG;
    if (count == 0 || count != cJSON_GetArraySize(args)) return ESP_ERR_INVALID_ARG;
    *decoded = p;
    return ESP_OK;
}

static esp_err_t decode_args(const char *target, const char *cmd,
                             const cJSON *args, decoded_args_t *storage,
                             const void **decoded)
{
    memset(storage, 0, sizeof(*storage));
    *decoded = NULL;
    if (strcmp(target, VAZON_COMMAND_TARGET_SYSTEM) == 0) return decode_system(cmd, args, storage, decoded);
    if (strcmp(target, VAZON_COMMAND_TARGET_LIGHT) == 0) return decode_light(cmd, args, storage, decoded);
    if (strcmp(target, VAZON_COMMAND_TARGET_FAN) == 0) return decode_fan(cmd, args, storage, decoded);
    if (strcmp(target, VAZON_COMMAND_TARGET_HUMIDIFIER) == 0) return decode_humidifier(cmd, args, storage, decoded);
    if (strcmp(target, VAZON_COMMAND_TARGET_CLIMATE) == 0) return decode_climate(cmd, args, storage, decoded);
    if (strcmp(target, VAZON_COMMAND_TARGET_POT_0) == 0 || strcmp(target, VAZON_COMMAND_TARGET_POT_1) == 0) return decode_pot(cmd, args, storage, decoded);
    return ESP_ERR_NOT_FOUND;
}

static vazon_command_source_t decode_source(const char *source)
{
    if (strcmp(source, "ui") == 0) return VAZON_COMMAND_SOURCE_UI;
    if (strcmp(source, "service") == 0) return VAZON_COMMAND_SOURCE_SERVICE;
    if (strcmp(source, "test") == 0) return VAZON_COMMAND_SOURCE_TEST;
    if (strcmp(source, "unknown") == 0) return VAZON_COMMAND_SOURCE_UNKNOWN;
    return (vazon_command_source_t)-1;
}

static const char *error_reason(esp_err_t error)
{
    switch (error) {
    case ESP_ERR_INVALID_ARG: return "invalid_command";
    case ESP_ERR_NOT_FOUND: return "unknown_target";
    case ESP_ERR_NOT_SUPPORTED: return "unsupported_command";
    case ESP_ERR_INVALID_STATE: return "invalid_state";
    case ESP_ERR_NO_MEM: return "no_memory";
    default: return "execution_failed";
    }
}

static esp_err_t create_response(const vazon_mqtt_command_codec_t *codec,
                                 const char *cmd_id, const char *target,
                                 const char *cmd, vazon_command_result_t result,
                                 vazon_mqtt_command_response_t *response)
{
    const char *topic_class = "fail";
    const char *result_text = "failed";
    if (result.status == VAZON_COMMAND_RESULT_ACCEPTED) {
        topic_class = "ack"; result_text = "accepted";
    } else if (result.status == VAZON_COMMAND_RESULT_REJECTED) {
        topic_class = "reject"; result_text = "rejected";
    }
    const int topic_written = snprintf(response->topic, sizeof(response->topic),
                                       "vazon/v3/%s/%s/%s", codec->device_id,
                                       topic_class, cmd_id);
    if (topic_written < 0 || (size_t)topic_written >= sizeof(response->topic)) return ESP_ERR_INVALID_SIZE;
    const int64_t now_ms = esp_timer_get_time() / 1000;
    int payload_written = 0;
    if (result.status == VAZON_COMMAND_RESULT_ACCEPTED) {
        payload_written = snprintf(response->payload, sizeof(response->payload),
            "{\"cmd_id\":\"%s\",\"target\":\"%s\",\"cmd\":\"%s\",\"result\":\"%s\",\"reason\":null,\"ts\":%lld}",
            cmd_id, target, cmd, result_text, (long long)now_ms);
    } else {
        payload_written = snprintf(response->payload, sizeof(response->payload),
            "{\"cmd_id\":\"%s\",\"target\":\"%s\",\"cmd\":\"%s\",\"result\":\"%s\",\"reason\":\"%s\",\"ts\":%lld}",
            cmd_id, target, cmd, result_text, error_reason(result.error),
            (long long)now_ms);
    }
    if (payload_written < 0 || (size_t)payload_written >= sizeof(response->payload)) return ESP_ERR_INVALID_SIZE;
    response->available = true;
    return ESP_OK;
}

esp_err_t vazon_mqtt_command_codec_init(
    vazon_mqtt_command_codec_t *codec, const char *device_id,
    const vazon_command_router_t *router)
{
    if (codec == NULL || router == NULL || !safe_token(device_id, VAZON_MQTT_CODEC_DEVICE_ID_SIZE)) return ESP_ERR_INVALID_ARG;
    memset(codec, 0, sizeof(*codec));
    strcpy(codec->device_id, device_id);
    const int written = snprintf(codec->command_prefix, sizeof(codec->command_prefix),
                                 "vazon/v3/%s/cmd/", device_id);
    if (written < 0 || (size_t)written >= sizeof(codec->command_prefix)) return ESP_ERR_INVALID_SIZE;
    codec->router = router;
    codec->initialized = true;
    return ESP_OK;
}

esp_err_t vazon_mqtt_command_codec_handle(
    const vazon_mqtt_command_codec_t *codec, const char *topic,
    size_t topic_length, const char *payload, size_t payload_length,
    vazon_mqtt_command_response_t *response)
{
    if (codec == NULL || !codec->initialized || topic == NULL || payload == NULL || response == NULL) return ESP_ERR_INVALID_ARG;
    memset(response, 0, sizeof(*response));
    const size_t prefix_length = strlen(codec->command_prefix);
    if (topic_length <= prefix_length || topic_length >= VAZON_MQTT_CODEC_TOPIC_SIZE ||
        memcmp(topic, codec->command_prefix, prefix_length) != 0) return ESP_ERR_NOT_FOUND;
    char target[CODEC_TARGET_SIZE];
    const size_t target_length = topic_length - prefix_length;
    if (target_length >= sizeof(target)) return ESP_ERR_INVALID_SIZE;
    memcpy(target, topic + prefix_length, target_length); target[target_length] = '\0';
    if (!target_valid(target)) return ESP_ERR_INVALID_ARG;

    cJSON *root = cJSON_ParseWithLength(payload, payload_length);
    if (!cJSON_IsObject(root) || cJSON_GetArraySize(root) != 5) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }
    char cmd_id[CODEC_CMD_ID_SIZE];
    char cmd[CODEC_CMD_SIZE];
    char source_text[16];
    const cJSON *args = cJSON_GetObjectItemCaseSensitive(root, "args");
    const cJSON *timestamp = cJSON_GetObjectItemCaseSensitive(root, "ts");
    if (!copy_json_string(root, "cmd_id", cmd_id, sizeof(cmd_id)) ||
        !safe_token(cmd_id, sizeof(cmd_id)) ||
        !copy_json_string(root, "cmd", cmd, sizeof(cmd)) ||
        !safe_token(cmd, sizeof(cmd)) ||
        !copy_json_string(root, "source", source_text, sizeof(source_text)) ||
        !cJSON_IsObject(args) || !cJSON_IsNumber(timestamp) ||
        !isfinite(timestamp->valuedouble) ||
        floor(timestamp->valuedouble) != timestamp->valuedouble ||
        timestamp->valuedouble < (double)INT64_MIN ||
        timestamp->valuedouble >= (double)INT64_MAX) {
        cJSON_Delete(root); return ESP_ERR_INVALID_ARG;
    }
    const vazon_command_source_t source = decode_source(source_text);
    if ((int)source < 0) { cJSON_Delete(root); return ESP_ERR_INVALID_ARG; }

    decoded_args_t storage;
    const void *decoded = NULL;
    const esp_err_t decode_result = decode_args(target, cmd, args, &storage, &decoded);
    vazon_command_result_t result = {
        .status = VAZON_COMMAND_RESULT_REJECTED,
        .error = decode_result,
    };
    if (decode_result == ESP_OK) {
        const vazon_command_t command = {
            .cmd_id = cmd_id, .target = target, .cmd = cmd, .args = decoded,
            .source = source, .ts = (int64_t)timestamp->valuedouble,
        };
        result = vazon_command_router_route(codec->router, &command);
    }
    const esp_err_t response_result = create_response(codec, cmd_id, target, cmd,
                                                       result, response);
    cJSON_Delete(root);
    return response_result;
}
