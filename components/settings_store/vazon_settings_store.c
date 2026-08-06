#include "vazon_settings_store.h"

#include "esp_crc.h"
#include "esp_log.h"
#include "nvs.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SETTINGS_SCHEMA_VERSION 1U

typedef struct {
    uint16_t schema_version;
    uint16_t payload_size;
    vazon_persisted_settings_t payload;
    uint32_t crc32;
} settings_blob_t;

static const char *TAG = "settings_store";
static const char *SETTINGS_NAMESPACE = "vazon_settings";
static const char *SETTINGS_KEY = "runtime_v1";

static uint32_t blob_crc(const settings_blob_t *blob)
{
    return esp_crc32_le(0U, (const uint8_t *)blob,
                        offsetof(settings_blob_t, crc32));
}

esp_err_t vazon_settings_store_load(vazon_persisted_settings_t *settings)
{
    if (settings == NULL) return ESP_ERR_INVALID_ARG;
    memset(settings, 0, sizeof(*settings));

    nvs_handle_t handle;
    esp_err_t result = nvs_open(SETTINGS_NAMESPACE, NVS_READONLY, &handle);
    if (result != ESP_OK) return result;

    settings_blob_t blob = {0};
    size_t size = sizeof(blob);
    result = nvs_get_blob(handle, SETTINGS_KEY, &blob, &size);
    nvs_close(handle);
    if (result != ESP_OK) return result;
    if (size != sizeof(blob) ||
        blob.schema_version != SETTINGS_SCHEMA_VERSION ||
        blob.payload_size != (uint16_t)sizeof(blob.payload)) {
        return ESP_ERR_INVALID_VERSION;
    }
    if (blob.crc32 != blob_crc(&blob)) return ESP_ERR_INVALID_CRC;

    *settings = blob.payload;
    ESP_LOGI(TAG, "Runtime settings loaded schema=%u",
             (unsigned int)blob.schema_version);
    return ESP_OK;
}

esp_err_t vazon_settings_store_save(
    const vazon_persisted_settings_t *settings)
{
    if (settings == NULL) return ESP_ERR_INVALID_ARG;
    settings_blob_t blob = {
        .schema_version = SETTINGS_SCHEMA_VERSION,
        .payload_size = (uint16_t)sizeof(blob.payload),
        .payload = *settings,
    };
    blob.crc32 = blob_crc(&blob);

    nvs_handle_t handle;
    esp_err_t result = nvs_open(SETTINGS_NAMESPACE, NVS_READWRITE, &handle);
    if (result != ESP_OK) return result;
    result = nvs_set_blob(handle, SETTINGS_KEY, &blob, sizeof(blob));
    if (result == ESP_OK) result = nvs_commit(handle);
    nvs_close(handle);
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Runtime settings saved schema=%u",
                 (unsigned int)blob.schema_version);
    }
    return result;
}

static bool pot_settings_equal(const vazon_pot_settings_t *left,
                               const vazon_pot_settings_t *right)
{
    return left->soil_moisture_enabled == right->soil_moisture_enabled &&
           left->soil_temperature_enabled == right->soil_temperature_enabled &&
           left->moisture_stale_timeout_sec == right->moisture_stale_timeout_sec &&
           left->temperature_stale_timeout_sec == right->temperature_stale_timeout_sec &&
           left->temperature_low_warn_c == right->temperature_low_warn_c &&
           left->temperature_high_warn_c == right->temperature_high_warn_c;
}

static bool calibration_equal(
    const vazon_pot_moisture_calibration_t *left,
    const vazon_pot_moisture_calibration_t *right)
{
    return left->dry_valid == right->dry_valid &&
           left->normal_valid == right->normal_valid &&
           left->wet_valid == right->wet_valid &&
           left->dry_calibration_mv == right->dry_calibration_mv &&
           left->normal_calibration_mv == right->normal_calibration_mv &&
           left->wet_calibration_mv == right->wet_calibration_mv;
}

bool vazon_settings_store_equal(
    const vazon_persisted_settings_t *left,
    const vazon_persisted_settings_t *right)
{
    if (left == NULL || right == NULL) return false;
    if (left->schedule_enabled != right->schedule_enabled ||
        memcmp(left->time_on, right->time_on, sizeof(left->time_on)) != 0 ||
        memcmp(left->time_off, right->time_off, sizeof(left->time_off)) != 0) {
        return false;
    }

    const vazon_climate_settings_t *lc = &left->climate;
    const vazon_climate_settings_t *rc = &right->climate;
    if (lc->temperature_low_warn != rc->temperature_low_warn ||
        lc->temperature_high_warn != rc->temperature_high_warn ||
        lc->humidity_low_warn != rc->humidity_low_warn ||
        lc->humidity_high_warn != rc->humidity_high_warn ||
        lc->sht31_stale_timeout_sec != rc->sht31_stale_timeout_sec ||
        lc->temperature_delta_warn != rc->temperature_delta_warn ||
        lc->temperature_delta_error != rc->temperature_delta_error) return false;

    if (left->light.mode != right->light.mode ||
        left->light.manual_state != right->light.manual_state) return false;

    const vazon_fan_settings_t *lf = &left->fan;
    const vazon_fan_settings_t *rf = &right->fan;
    if (lf->mode != rf->mode || lf->runtime != rf->runtime ||
        lf->auto_strategy != rf->auto_strategy ||
        lf->manual_duration_sec != rf->manual_duration_sec ||
        lf->auto_delta_on_pct != rf->auto_delta_on_pct ||
        lf->auto_delta_off_pct != rf->auto_delta_off_pct ||
        lf->auto_timer_on_sec != rf->auto_timer_on_sec ||
        lf->auto_timer_off_sec != rf->auto_timer_off_sec ||
        lf->power_level_pct != rf->power_level_pct) return false;

    const vazon_humidifier_settings_t *lh = &left->humidifier;
    const vazon_humidifier_settings_t *rh = &right->humidifier;
    if (lh->mode != rh->mode || lh->runtime != rh->runtime ||
        lh->rh_start != rh->rh_start || lh->rh_stop != rh->rh_stop ||
        lh->rh_delta != rh->rh_delta ||
        lh->mist_power_level != rh->mist_power_level ||
        lh->manual_mist_duration_sec != rh->manual_mist_duration_sec ||
        lh->post_fan_sec != rh->post_fan_sec) return false;

    for (size_t index = 0; index < 2U; ++index) {
        if (!pot_settings_equal(&left->pots[index], &right->pots[index]) ||
            !calibration_equal(&left->pot_calibrations[index],
                               &right->pot_calibrations[index])) return false;
    }
    return true;
}
