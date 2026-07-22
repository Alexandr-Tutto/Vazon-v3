#pragma once

#include "esp_err.h"
#include "vazon_command_router.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VAZON_CLIMATE_SHT31_0_ADDRESS 0x44U
#define VAZON_CLIMATE_SHT31_1_ADDRESS 0x45U
#define VAZON_COMMAND_TARGET_CLIMATE "climate"
#define VAZON_COMMAND_CLIMATE_SET_SETTINGS "set_settings"

typedef enum {
    VAZON_CLIMATE_SENSOR_UNKNOWN = 0,
    VAZON_CLIMATE_SENSOR_OK,
    VAZON_CLIMATE_SENSOR_WARNING,
    VAZON_CLIMATE_SENSOR_ERROR,
} vazon_climate_sensor_status_t;

typedef enum {
    VAZON_CLIMATE_STATUS_UNKNOWN = 0,
    VAZON_CLIMATE_STATUS_OK,
    VAZON_CLIMATE_STATUS_WARNING,
    VAZON_CLIMATE_STATUS_ERROR,
    VAZON_CLIMATE_STATUS_INACTIVE,
} vazon_climate_status_t;

typedef enum {
    VAZON_CLIMATE_REASON_NONE = 0,
    VAZON_CLIMATE_REASON_SHT31_0X44_MISSING,
    VAZON_CLIMATE_REASON_SHT31_0X45_MISSING,
    VAZON_CLIMATE_REASON_SHT31_STALE,
    VAZON_CLIMATE_REASON_INVALID_VALUE,
    VAZON_CLIMATE_REASON_TEMPERATURE_OUT_OF_RANGE,
    VAZON_CLIMATE_REASON_HUMIDITY_OUT_OF_RANGE,
    VAZON_CLIMATE_REASON_TEMPERATURE_DELTA_HIGH,
    VAZON_CLIMATE_REASON_TEMPERATURE_DELTA_CRITICAL,
} vazon_climate_status_reason_t;

typedef struct {
    float temperature_low_warn;
    float temperature_high_warn;
    float humidity_low_warn;
    float humidity_high_warn;
    uint32_t sht31_stale_timeout_sec;
    float temperature_delta_warn;
    float temperature_delta_error;
} vazon_climate_settings_t;

typedef struct {
    bool has_temperature_low_warn;
    float temperature_low_warn;
    bool has_temperature_high_warn;
    float temperature_high_warn;
    bool has_humidity_low_warn;
    float humidity_low_warn;
    bool has_humidity_high_warn;
    float humidity_high_warn;
    bool has_sht31_stale_timeout_sec;
    uint32_t sht31_stale_timeout_sec;
    bool has_temperature_delta_warn;
    float temperature_delta_warn;
    bool has_temperature_delta_error;
    float temperature_delta_error;
} vazon_climate_settings_patch_t;

typedef struct {
    float temperature_c;
    float humidity_pct;
    vazon_climate_sensor_status_t status;
    bool present;
    bool has_valid_sample;
    uint64_t last_valid_sample_ms;
} vazon_climate_sensor_snapshot_t;

typedef struct {
    vazon_climate_sensor_snapshot_t sensor_0x44;
    vazon_climate_sensor_snapshot_t sensor_0x45;
    float rh_delta_pct;
    float temperature_delta_c;
    vazon_climate_status_t status;
    vazon_climate_status_reason_t status_reason;
    bool data_valid;
    bool data_stale;
    vazon_climate_settings_t settings;
} vazon_climate_snapshot_t;

typedef struct {
    bool present_known;
    bool invalid_value;
    bool first_attempt_recorded;
    uint64_t first_attempt_ms;
} vazon_climate_sensor_runtime_t;

typedef struct {
    vazon_climate_snapshot_t snapshot;
    vazon_climate_sensor_runtime_t sensor_runtime[2];
    bool initialized;
    uint64_t last_evaluation_ms;
} vazon_climate_module_t;

esp_err_t vazon_climate_module_init(vazon_climate_module_t *module,
                                    const vazon_climate_settings_t *settings);
esp_err_t vazon_climate_module_record_sensor(vazon_climate_module_t *module,
                                             uint8_t address,
                                             bool present,
                                             esp_err_t read_result,
                                             float temperature_c,
                                             float humidity_pct,
                                             uint64_t now_ms);
esp_err_t vazon_climate_module_evaluate(vazon_climate_module_t *module,
                                        uint64_t now_ms);
esp_err_t vazon_climate_module_set_settings(
    vazon_climate_module_t *module,
    const vazon_climate_settings_patch_t *patch);
esp_err_t vazon_climate_module_register_command_target(
    vazon_climate_module_t *module, vazon_command_router_t *router);
const vazon_climate_snapshot_t *vazon_climate_module_get_snapshot(
    const vazon_climate_module_t *module);

#ifdef __cplusplus
}
#endif
