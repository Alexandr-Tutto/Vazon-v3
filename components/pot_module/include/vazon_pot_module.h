#pragma once

#include "esp_err.h"
#include "vazon_command_router.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VAZON_POT_VALUE_UNKNOWN (-1)
#define VAZON_COMMAND_TARGET_POT_0 "pot/0"
#define VAZON_COMMAND_TARGET_POT_1 "pot/1"
#define VAZON_COMMAND_POT_SET_SOIL_MOISTURE_ENABLED \
    "set_soil_moisture_enabled"
#define VAZON_COMMAND_POT_SET_SOIL_TEMPERATURE_ENABLED \
    "set_soil_temperature_enabled"
#define VAZON_COMMAND_POT_SET_SETTINGS "set_settings"
#define VAZON_COMMAND_POT_CALIBRATE_SOIL_MOISTURE \
    "calibrate_soil_moisture"

typedef enum {
    VAZON_POT_SENSOR_UNKNOWN = 0,
    VAZON_POT_SENSOR_OK,
    VAZON_POT_SENSOR_WARNING,
    VAZON_POT_SENSOR_ERROR,
    VAZON_POT_SENSOR_INACTIVE,
} vazon_pot_sensor_status_t;

typedef enum {
    VAZON_POT_MOISTURE_CLASS_UNKNOWN = 0,
    VAZON_POT_MOISTURE_CLASS_DRY,
    VAZON_POT_MOISTURE_CLASS_NORMAL,
    VAZON_POT_MOISTURE_CLASS_WET,
    VAZON_POT_MOISTURE_CLASS_OVERWET,
} vazon_pot_moisture_class_t;

typedef enum {
    VAZON_POT_STATUS_UNKNOWN = 0,
    VAZON_POT_STATUS_OK,
    VAZON_POT_STATUS_WARNING,
    VAZON_POT_STATUS_ERROR,
    VAZON_POT_STATUS_INACTIVE,
} vazon_pot_status_t;

typedef enum {
    VAZON_POT_REASON_NONE = 0,
    VAZON_POT_REASON_CALIBRATION_INVALID,
    VAZON_POT_REASON_MOISTURE_READ_FAILED,
    VAZON_POT_REASON_MOISTURE_STALE,
    VAZON_POT_REASON_TEMPERATURE_MISSING,
    VAZON_POT_REASON_TEMPERATURE_READ_FAILED,
    VAZON_POT_REASON_TEMPERATURE_STALE,
    VAZON_POT_REASON_TEMPERATURE_INVALID,
    VAZON_POT_REASON_TEMPERATURE_OUT_OF_RANGE,
} vazon_pot_status_reason_t;

typedef enum {
    VAZON_POT_CALIBRATION_DRY = 0,
    VAZON_POT_CALIBRATION_NORMAL,
    VAZON_POT_CALIBRATION_WET,
    VAZON_POT_CALIBRATION_RESET,
} vazon_pot_calibration_point_t;

typedef struct {
    vazon_pot_calibration_point_t point;
} vazon_pot_calibration_args_t;

typedef struct {
    bool soil_moisture_enabled;
    bool soil_temperature_enabled;
    uint32_t moisture_stale_timeout_sec;
    uint32_t temperature_stale_timeout_sec;
    float temperature_low_warn_c;
    float temperature_high_warn_c;
} vazon_pot_settings_t;

typedef struct {
    bool enabled;
} vazon_pot_set_enabled_args_t;

typedef struct {
    bool has_soil_moisture_enabled;
    bool soil_moisture_enabled;
    bool has_soil_temperature_enabled;
    bool soil_temperature_enabled;
    bool has_moisture_stale_timeout_sec;
    uint32_t moisture_stale_timeout_sec;
    bool has_temperature_stale_timeout_sec;
    uint32_t temperature_stale_timeout_sec;
    bool has_temperature_low_warn_c;
    float temperature_low_warn_c;
    bool has_temperature_high_warn_c;
    float temperature_high_warn_c;
} vazon_pot_settings_patch_t;

typedef struct {
    bool dry_valid;
    bool normal_valid;
    bool wet_valid;
    int dry_calibration_mv;
    int normal_calibration_mv;
    int wet_calibration_mv;
} vazon_pot_moisture_calibration_t;

typedef struct {
    int raw_adc_value;
    int raw_mv;
    int16_t value_pct;
    vazon_pot_moisture_class_t moisture_class;
    vazon_pot_sensor_status_t status;
} vazon_pot_moisture_snapshot_t;

typedef struct {
    float temperature_c;
    vazon_pot_sensor_status_t status;
    bool present;
} vazon_pot_temperature_snapshot_t;

typedef struct {
    uint8_t pot_id;
    vazon_pot_moisture_snapshot_t soil_moisture;
    vazon_pot_temperature_snapshot_t soil_temperature;
    vazon_pot_status_t status;
    vazon_pot_status_reason_t status_reason;
    vazon_pot_settings_t settings;
    vazon_pot_moisture_calibration_t calibration;
} vazon_pot_snapshot_t;

typedef struct {
    vazon_pot_snapshot_t snapshot;
    bool initialized;
    bool moisture_attempted;
    bool moisture_last_read_failed;
    bool moisture_has_valid_read;
    uint64_t moisture_first_attempt_ms;
    uint64_t moisture_last_valid_ms;
    bool temperature_attempted;
    bool temperature_last_read_failed;
    bool temperature_invalid;
    bool temperature_has_valid_read;
    uint64_t temperature_first_attempt_ms;
    uint64_t temperature_last_valid_ms;
    uint64_t last_evaluation_ms;
} vazon_pot_module_t;

esp_err_t vazon_pot_module_init(vazon_pot_module_t *module,
                                uint8_t pot_id,
                                const vazon_pot_settings_t *settings);
esp_err_t vazon_pot_module_record_moisture(vazon_pot_module_t *module,
                                           esp_err_t read_result,
                                           int raw_adc_value,
                                           int raw_mv,
                                           uint64_t now_ms);
esp_err_t vazon_pot_module_record_temperature(vazon_pot_module_t *module,
                                              bool present,
                                              esp_err_t read_result,
                                              float temperature_c,
                                              uint64_t now_ms);
esp_err_t vazon_pot_module_evaluate(vazon_pot_module_t *module,
                                    uint64_t now_ms);
esp_err_t vazon_pot_module_calibrate_moisture(
    vazon_pot_module_t *module, vazon_pot_calibration_point_t point);
esp_err_t vazon_pot_module_set_settings(
    vazon_pot_module_t *module, const vazon_pot_settings_patch_t *patch);
esp_err_t vazon_pot_module_register_command_target(
    vazon_pot_module_t *module, vazon_command_router_t *router);
const vazon_pot_snapshot_t *vazon_pot_module_get_snapshot(
    const vazon_pot_module_t *module);

#ifdef __cplusplus
}
#endif
