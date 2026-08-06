#include "vazon_app_core.h"

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "vazon_adc_service.h"
#include "vazon_board_config.h"
#include "vazon_climate_module.h"
#include "vazon_command_router.h"
#include "vazon_connection_config.h"
#include "vazon_door_module.h"
#include "vazon_fan_module.h"
#include "vazon_gpio_inputs.h"
#include "vazon_gpio_outputs.h"
#include "vazon_humidifier_module.h"
#include "vazon_i2c_service.h"
#include "vazon_light_module.h"
#include "vazon_mqtt_command_codec.h"
#include "vazon_mqtt_command_queue.h"
#include "vazon_mqtt_service.h"
#include "vazon_mqtt_state_publisher.h"
#include "vazon_onewire_service.h"
#include "vazon_ota_service.h"
#include "vazon_pot_module.h"
#include "vazon_provisioning_button.h"
#include "vazon_provisioning_service.h"
#include "vazon_runtime_core.h"
#include "vazon_settings_store.h"
#include "vazon_status_led.h"
#include "vazon_system_status.h"
#include "vazon_wifi_service.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *TAG = "app_core";

#define DOOR_POLL_INTERVAL_MS      20U
#define DOOR_DEBOUNCE_MS           100U
#define DOOR_UNSTABLE_TIMEOUT_MS   1000U
#define CLIMATE_READ_INTERVAL_MS   5000U
#define POT_READ_INTERVAL_MS       10000U
#define MQTT_COMMAND_QUEUE_DEPTH   8U
#define MQTT_COMMAND_TASK_STACK    6144U
#define MQTT_STATE_TASK_STACK      8192U
#define MQTT_STATE_POLL_MS         1000U
#define DAY_WINDOW_TASK_STACK      3072U
#define DAY_WINDOW_POLL_MS         1000U
#define SETTINGS_TASK_STACK        3072U
#define SETTINGS_POLL_MS           1000U
#define CLOCK_VALID_YEAR           2020
#define PROVISIONING_CHECK_MS       5000U
#define PROVISIONING_HOLD_MS        5000U
#define PROVISIONING_POLL_MS        50U
#define PROVISIONING_LED_POLL_MS    50U

static vazon_command_router_t command_router;
static vazon_runtime_core_t runtime_core;
static vazon_climate_module_t climate_module;
static vazon_door_module_t door_module;
static vazon_light_module_t light_module;
static vazon_fan_module_t fan_module;
static vazon_humidifier_module_t humidifier_module;
static vazon_pot_module_t pot_modules[2];
static vazon_system_status_t system_status;
static vazon_status_led_t status_led;
static vazon_mqtt_service_t mqtt_service;
static vazon_wifi_service_t wifi_service;
static vazon_mqtt_command_codec_t mqtt_command_codec;
static vazon_mqtt_command_queue_t mqtt_command_queue;
static vazon_mqtt_state_publisher_t mqtt_state_publisher;
static vazon_ota_service_t ota_service;
static SemaphoreHandle_t runtime_mutex;
static uint64_t next_climate_read_ms;
static bool sntp_started;
static uint8_t humidifier_mist_applied_duty_pct;
static char ota_status_topic[VAZON_MQTT_TOPIC_SIZE];
static vazon_persisted_settings_t last_saved_settings;
static bool last_saved_settings_valid;

static esp_err_t lock_runtime(void *context);
static void unlock_runtime(void *context);

static void capture_persisted_settings(vazon_persisted_settings_t *settings)
{
    memset(settings, 0, sizeof(*settings));
    const vazon_global_context_t *global =
        vazon_runtime_core_get_context(&runtime_core);
    settings->schedule_enabled = global->day_window.schedule_enabled;
    memcpy(settings->time_on, global->day_window.time_on,
           sizeof(settings->time_on));
    memcpy(settings->time_off, global->day_window.time_off,
           sizeof(settings->time_off));
    settings->climate =
        vazon_climate_module_get_snapshot(&climate_module)->settings;
    settings->light = vazon_light_module_get_snapshot(&light_module)->settings;
    settings->fan = vazon_fan_module_get_snapshot(&fan_module)->settings;
    settings->humidifier =
        vazon_humidifier_module_get_snapshot(&humidifier_module)->settings;
    for (size_t index = 0; index < 2U; ++index) {
        const vazon_pot_snapshot_t *pot =
            vazon_pot_module_get_snapshot(&pot_modules[index]);
        settings->pots[index] = pot->settings;
        settings->pot_calibrations[index] = pot->calibration;
    }
}

static void settings_runtime_task(void *context)
{
    (void)context;
    while (true) {
        vazon_persisted_settings_t current;
        if (lock_runtime(runtime_mutex) == ESP_OK) {
            capture_persisted_settings(&current);
            unlock_runtime(runtime_mutex);
            if (!last_saved_settings_valid ||
                !vazon_settings_store_equal(&current, &last_saved_settings)) {
                const esp_err_t result = vazon_settings_store_save(&current);
                if (result == ESP_OK) {
                    last_saved_settings = current;
                    last_saved_settings_valid = true;
                } else {
                    ESP_LOGW(TAG, "Runtime settings save failed: %s",
                             esp_err_to_name(result));
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SETTINGS_POLL_MS));
    }
}

static const char *day_window_active_text(vazon_tristate_t active)
{
    return active == VAZON_TRISTATE_TRUE ? "true" :
           active == VAZON_TRISTATE_FALSE ? "false" : "unknown";
}

static void start_sntp_if_needed(void)
{
    if (sntp_started) return;

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    esp_sntp_set_sync_interval(3600UL * 1000UL);
    esp_sntp_init();
    sntp_started = true;
    ESP_LOGI(TAG, "SNTP started server=pool.ntp.org timezone=Europe/Kyiv");
}

static void day_window_runtime_task(void *context)
{
    (void)context;
    bool first_report = true;
    bool previous_clock_valid = false;
    vazon_tristate_t previous_active = VAZON_TRISTATE_UNKNOWN;

    while (true) {
        const time_t now = time(NULL);
        struct tm local_time = {0};
        const bool clock_valid =
            localtime_r(&now, &local_time) != NULL &&
            local_time.tm_year + 1900 >= CLOCK_VALID_YEAR;

        ESP_ERROR_CHECK(lock_runtime(runtime_mutex));
        const esp_err_t result = vazon_runtime_core_evaluate_day_window(
            &runtime_core, clock_valid,
            clock_valid ? (uint8_t)local_time.tm_hour : 0U,
            clock_valid ? (uint8_t)local_time.tm_min : 0U);
        const vazon_day_window_t day_window =
            vazon_runtime_core_get_context(&runtime_core)->day_window;
        unlock_runtime(runtime_mutex);

        if (result != ESP_OK) {
            ESP_LOGE(TAG, "Day window evaluation failed: %s",
                     esp_err_to_name(result));
        } else if (first_report || clock_valid != previous_clock_valid ||
                   day_window.active != previous_active) {
            if (clock_valid) {
                ESP_LOGI(TAG,
                         "Clock local=%04d-%02d-%02d %02d:%02d:%02d day_window=%s schedule=%s-%s",
                         local_time.tm_year + 1900, local_time.tm_mon + 1,
                         local_time.tm_mday, local_time.tm_hour,
                         local_time.tm_min, local_time.tm_sec,
                         day_window_active_text(day_window.active),
                         day_window.time_on, day_window.time_off);
            } else {
                ESP_LOGW(TAG, "Clock invalid day_window=%s",
                         day_window_active_text(day_window.active));
            }
        }

        first_report = false;
        previous_clock_valid = clock_valid;
        previous_active = day_window.active;
        vTaskDelay(pdMS_TO_TICKS(DAY_WINDOW_POLL_MS));
    }
}

static vazon_connection_state_t mqtt_runtime_state(
    vazon_mqtt_connection_state_t state)
{
    switch (state) {
    case VAZON_MQTT_CONNECTION_CONNECTED:
        return VAZON_CONNECTION_CONNECTED;
    case VAZON_MQTT_CONNECTION_CONNECTING:
        return VAZON_CONNECTION_CONNECTING;
    case VAZON_MQTT_CONNECTION_DISCONNECTED:
    default:
        return VAZON_CONNECTION_DISCONNECTED;
    }
}

static vazon_connection_state_t wifi_runtime_state(
    vazon_wifi_connection_state_t state)
{
    switch (state) {
    case VAZON_WIFI_CONNECTION_CONNECTED:
        return VAZON_CONNECTION_CONNECTED;
    case VAZON_WIFI_CONNECTION_CONNECTING:
        return VAZON_CONNECTION_CONNECTING;
    case VAZON_WIFI_CONNECTION_DISCONNECTED:
    default:
        return VAZON_CONNECTION_DISCONNECTED;
    }
}

static esp_err_t lock_runtime(void *context)
{
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)context;
    if (mutex == NULL) return ESP_ERR_INVALID_ARG;
    return xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE ? ESP_OK
                                                          : ESP_ERR_TIMEOUT;
}

static void unlock_runtime(void *context)
{
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)context;
    if (mutex != NULL) xSemaphoreGive(mutex);
}

static esp_err_t publish_mqtt_message(
    const char *topic, const char *payload, int qos, bool retain,
    void *context)
{
    return vazon_mqtt_service_publish((vazon_mqtt_service_t *)context,
                                      topic, payload, qos, retain);
}

static void ota_status_changed(const char *state, const char *version,
                               esp_err_t error, void *context)
{
    (void)context;
    char payload[256];
    const int written = snprintf(
        payload, sizeof(payload),
        "{\"state\":\"%s\",\"version\":%s%s%s,\"error\":%s%s%s,\"ts\":%lld}",
        state,
        version == NULL ? "null" : "\"",
        version == NULL ? "" : version,
        version == NULL ? "" : "\"",
        error == ESP_OK ? "null" : "\"",
        error == ESP_OK ? "" : esp_err_to_name(error),
        error == ESP_OK ? "" : "\"",
        (long long)(esp_timer_get_time() / 1000));
    if (written < 0 || (size_t)written >= sizeof(payload)) {
        ESP_LOGE(TAG, "OTA status payload overflow");
        return;
    }
    const esp_err_t result = vazon_mqtt_service_publish(
        &mqtt_service, ota_status_topic, payload, 1, false);
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "OTA status publish failed: %s",
                 esp_err_to_name(result));
    }
}

static void mqtt_connection_changed(vazon_mqtt_connection_state_t state,
                                    void *context)
{
    (void)context;
    if (lock_runtime(runtime_mutex) != ESP_OK) return;
    const vazon_global_context_t *global_context =
        vazon_runtime_core_get_context(&runtime_core);
    const vazon_connection_state_t wifi_state =
        global_context->connection.wifi_state;
    ESP_ERROR_CHECK_WITHOUT_ABORT(vazon_runtime_core_set_connection(
        &runtime_core, wifi_state, mqtt_runtime_state(state)));
    unlock_runtime(runtime_mutex);
    ESP_LOGI(TAG, "MQTT state=%s",
             state == VAZON_MQTT_CONNECTION_CONNECTED ? "connected" :
             state == VAZON_MQTT_CONNECTION_CONNECTING ? "connecting" :
                                                        "disconnected");
    if (state == VAZON_MQTT_CONNECTION_CONNECTED) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(
            vazon_ota_service_report_ready(&ota_service));
    }
}

static void mqtt_message_received(const char *topic, size_t topic_length,
                                  const char *payload, size_t payload_length,
                                  void *context)
{
    (void)context;
    vazon_mqtt_command_queue_on_message(topic, topic_length, payload,
                                        payload_length,
                                        &mqtt_command_queue);
}

static void wifi_connection_changed(vazon_wifi_connection_state_t state,
                                    void *context)
{
    (void)context;
    if (lock_runtime(runtime_mutex) != ESP_OK) return;
    const vazon_global_context_t *global_context =
        vazon_runtime_core_get_context(&runtime_core);
    const vazon_connection_state_t mqtt_state =
        global_context->connection.mqtt_state;
    ESP_ERROR_CHECK_WITHOUT_ABORT(vazon_runtime_core_set_connection(
        &runtime_core, wifi_runtime_state(state), mqtt_state));
    unlock_runtime(runtime_mutex);

    ESP_LOGI(TAG, "Wi-Fi state=%s",
             state == VAZON_WIFI_CONNECTION_CONNECTED ? "connected" :
             state == VAZON_WIFI_CONNECTION_CONNECTING ? "connecting" :
                                                        "disconnected");
    if (state == VAZON_WIFI_CONNECTION_CONNECTED &&
        mqtt_service.initialized && !mqtt_service.started) {
        start_sntp_if_needed();
        const esp_err_t result = vazon_mqtt_service_start(&mqtt_service);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "MQTT start failed: %s", esp_err_to_name(result));
            mqtt_connection_changed(VAZON_MQTT_CONNECTION_DISCONNECTED, NULL);
        }
    }
}

static esp_err_t build_device_id(char *device_id, size_t device_id_size)
{
    if (device_id == NULL || device_id_size == 0U) return ESP_ERR_INVALID_ARG;
    uint8_t mac[6];
    ESP_RETURN_ON_ERROR(esp_read_mac(mac, ESP_MAC_WIFI_STA), TAG,
                        "Failed to read STA MAC");
    const int written = snprintf(device_id, device_id_size,
                                 "Vazon_%02u_%02x%02x%02x",
                                 (unsigned int)VAZON_DEVICE_SERIAL_NUMBER,
                                 mac[3], mac[4], mac[5]);
    return written < 0 || (size_t)written >= device_id_size
               ? ESP_ERR_INVALID_SIZE
               : ESP_OK;
}

static const char *climate_status_text(vazon_climate_status_t status)
{
    switch (status) {
    case VAZON_CLIMATE_STATUS_OK:
        return "ok";
    case VAZON_CLIMATE_STATUS_WARNING:
        return "warning";
    case VAZON_CLIMATE_STATUS_ERROR:
        return "error";
    case VAZON_CLIMATE_STATUS_INACTIVE:
        return "inactive";
    case VAZON_CLIMATE_STATUS_UNKNOWN:
    default:
        return "unknown";
    }
}

static const char *climate_reason_text(vazon_climate_status_reason_t reason)
{
    switch (reason) {
    case VAZON_CLIMATE_REASON_SHT31_0X44_MISSING:
        return "sht31_0x44_missing";
    case VAZON_CLIMATE_REASON_SHT31_0X45_MISSING:
        return "sht31_0x45_missing";
    case VAZON_CLIMATE_REASON_SHT31_STALE:
        return "sht31_stale";
    case VAZON_CLIMATE_REASON_INVALID_VALUE:
        return "invalid_value";
    case VAZON_CLIMATE_REASON_TEMPERATURE_OUT_OF_RANGE:
        return "temperature_out_of_range";
    case VAZON_CLIMATE_REASON_HUMIDITY_OUT_OF_RANGE:
        return "humidity_out_of_range";
    case VAZON_CLIMATE_REASON_TEMPERATURE_DELTA_HIGH:
        return "temperature_delta_high";
    case VAZON_CLIMATE_REASON_TEMPERATURE_DELTA_CRITICAL:
        return "temperature_delta_critical";
    case VAZON_CLIMATE_REASON_NONE:
    default:
        return "none";
    }
}

static void read_climate_and_log(uint64_t now_ms)
{
    if (now_ms < next_climate_read_ms) return;
    next_climate_read_ms = now_ms + CLIMATE_READ_INTERVAL_MS;

    const uint8_t addresses[2] = {VAZON_CLIMATE_SHT31_0_ADDRESS,
                                  VAZON_CLIMATE_SHT31_1_ADDRESS};
    for (size_t index = 0; index < 2U; ++index) {
        bool present = false;
        esp_err_t result =
            vazon_i2c_service_sht31_is_present(addresses[index], &present);
        float temperature_c = 0.0f;
        float humidity_pct = 0.0f;
        if (result == ESP_OK && present) {
            result = vazon_i2c_service_read_sht31(
                addresses[index], &temperature_c, &humidity_pct);
        } else if (result == ESP_OK) {
            result = ESP_ERR_NOT_FOUND;
        }
        ESP_ERROR_CHECK(vazon_climate_module_record_sensor(
            &climate_module, addresses[index], present, result,
            temperature_c, humidity_pct, now_ms));
        if (result != ESP_OK) {
            ESP_LOGW(TAG,
                     "SENSOR_FAIL climate address=0x%02X temperature=unknown humidity=unknown result=%s",
                     addresses[index], esp_err_to_name(result));
        } else if (result == ESP_OK) {
            ESP_LOGI(TAG,
                     "SENSOR_OK climate address=0x%02X temperature=%.1fC humidity=%.1f%% result=ok",
                     addresses[index], temperature_c, humidity_pct);
        }
    }

    ESP_ERROR_CHECK(vazon_climate_module_evaluate(&climate_module, now_ms));
    const vazon_climate_snapshot_t *snapshot =
        vazon_climate_module_get_snapshot(&climate_module);
    if (snapshot->data_valid) {
        ESP_LOGI(TAG,
                 "Climate 0x44=%.1fC/%.1f%% 0x45=%.1fC/%.1f%% dT=%.1fC dRH=%.1f%% status=%s reason=%s",
                 snapshot->sensor_0x44.temperature_c,
                 snapshot->sensor_0x44.humidity_pct,
                 snapshot->sensor_0x45.temperature_c,
                 snapshot->sensor_0x45.humidity_pct,
                 snapshot->temperature_delta_c,
                 snapshot->rh_delta_pct,
                 climate_status_text(snapshot->status),
                 climate_reason_text(snapshot->status_reason));
    } else {
        ESP_LOGW(TAG, "Climate data invalid status=%s reason=%s",
                 climate_status_text(snapshot->status),
                 climate_reason_text(snapshot->status_reason));
    }
}

static const char *pot_sensor_status_text(vazon_pot_sensor_status_t status)
{
    switch (status) {
    case VAZON_POT_SENSOR_OK:
        return "ok";
    case VAZON_POT_SENSOR_WARNING:
        return "warning";
    case VAZON_POT_SENSOR_ERROR:
        return "error";
    case VAZON_POT_SENSOR_INACTIVE:
        return "inactive";
    case VAZON_POT_SENSOR_UNKNOWN:
    default:
        return "unknown";
    }
}

static const char *pot_reason_text(vazon_pot_status_reason_t reason)
{
    switch (reason) {
    case VAZON_POT_REASON_CALIBRATION_INVALID:
        return "calibration_invalid";
    case VAZON_POT_REASON_MOISTURE_READ_FAILED:
        return "moisture_read_failed";
    case VAZON_POT_REASON_MOISTURE_STALE:
        return "moisture_stale";
    case VAZON_POT_REASON_TEMPERATURE_MISSING:
        return "temperature_missing";
    case VAZON_POT_REASON_TEMPERATURE_READ_FAILED:
        return "temperature_read_failed";
    case VAZON_POT_REASON_TEMPERATURE_STALE:
        return "temperature_stale";
    case VAZON_POT_REASON_TEMPERATURE_INVALID:
        return "temperature_invalid";
    case VAZON_POT_REASON_TEMPERATURE_OUT_OF_RANGE:
        return "temperature_out_of_range";
    case VAZON_POT_REASON_NONE:
    default:
        return "none";
    }
}

static vazon_system_module_status_t pot_system_status(vazon_pot_status_t status)
{
    switch (status) {
    case VAZON_POT_STATUS_OK:
        return VAZON_SYSTEM_MODULE_STATUS_OK;
    case VAZON_POT_STATUS_WARNING:
        return VAZON_SYSTEM_MODULE_STATUS_WARNING;
    case VAZON_POT_STATUS_ERROR:
        return VAZON_SYSTEM_MODULE_STATUS_ERROR;
    case VAZON_POT_STATUS_INACTIVE:
        return VAZON_SYSTEM_MODULE_STATUS_INACTIVE;
    case VAZON_POT_STATUS_UNKNOWN:
    default:
        return VAZON_SYSTEM_MODULE_STATUS_UNKNOWN;
    }
}

static void pot_runtime_task(void *context)
{
    (void)context;
    while (true) {
        float temperatures_c[2] = {0.0f, 0.0f};
        esp_err_t temperature_results[2] = {ESP_ERR_NOT_FOUND,
                                            ESP_ERR_NOT_FOUND};
        const esp_err_t batch_result =
            vazon_onewire_service_read_pot_temperatures(
                temperatures_c, temperature_results);
        if (batch_result != ESP_OK) {
            ESP_LOGE(TAG, "Pot temperature batch read failed: %s",
                     esp_err_to_name(batch_result));
        }

        const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000);
        for (uint8_t pot_id = 0; pot_id < 2U; ++pot_id) {
            int raw_value = 0;
            int millivolts = 0;
            const esp_err_t moisture_result =
                vazon_adc_service_read_millivolts_average(
                    pot_id, 32U, &raw_value, &millivolts);
            bool temperature_present = false;
            const esp_err_t presence_result =
                vazon_onewire_service_sensor_present(
                    pot_id, &temperature_present);
            if (presence_result != ESP_OK) temperature_present = false;
            const esp_err_t temperature_result =
                batch_result == ESP_OK ? temperature_results[pot_id]
                                       : batch_result;

            ESP_ERROR_CHECK(lock_runtime(runtime_mutex));
            ESP_ERROR_CHECK(vazon_pot_module_record_moisture(
                &pot_modules[pot_id], moisture_result, raw_value,
                millivolts, now_ms));
            ESP_ERROR_CHECK(vazon_pot_module_record_temperature(
                &pot_modules[pot_id], temperature_present,
                temperature_result, temperatures_c[pot_id], now_ms));
            ESP_ERROR_CHECK(vazon_pot_module_evaluate(
                &pot_modules[pot_id], now_ms));

            const vazon_pot_snapshot_t *snapshot =
                vazon_pot_module_get_snapshot(&pot_modules[pot_id]);
            const bool sensor_missing =
                snapshot->status_reason == VAZON_POT_REASON_MOISTURE_READ_FAILED ||
                snapshot->status_reason == VAZON_POT_REASON_MOISTURE_STALE ||
                snapshot->status_reason == VAZON_POT_REASON_TEMPERATURE_MISSING ||
                snapshot->status_reason == VAZON_POT_REASON_TEMPERATURE_READ_FAILED ||
                snapshot->status_reason == VAZON_POT_REASON_TEMPERATURE_STALE;
            ESP_ERROR_CHECK(vazon_system_status_update_module(
                &system_status,
                pot_id == 0U ? VAZON_SYSTEM_MODULE_POT_0
                             : VAZON_SYSTEM_MODULE_POT_1,
                pot_system_status(snapshot->status),
                pot_reason_text(snapshot->status_reason), sensor_missing,
                false, now_ms));
            if (moisture_result == ESP_OK &&
                snapshot->soil_moisture.value_pct == VAZON_POT_VALUE_UNKNOWN) {
                ESP_LOGI(TAG,
                         "SENSOR_OK pot[%u].soil_moisture GPIO%d raw=%d mv=%d value=unknown status=%s reason=%s result=ok",
                         (unsigned int)pot_id,
                         pot_id == 0U ? VAZON_GPIO_POT0_SOIL_MOISTURE
                                      : VAZON_GPIO_POT1_SOIL_MOISTURE,
                         snapshot->soil_moisture.raw_adc_value,
                         snapshot->soil_moisture.raw_mv,
                         pot_sensor_status_text(snapshot->soil_moisture.status),
                         "calibration_invalid");
            } else if (moisture_result == ESP_OK) {
                ESP_LOGI(TAG,
                         "SENSOR_OK pot[%u].soil_moisture GPIO%d raw=%d mv=%d value=%d%% status=%s reason=%s result=ok",
                         (unsigned int)pot_id,
                         pot_id == 0U ? VAZON_GPIO_POT0_SOIL_MOISTURE
                                      : VAZON_GPIO_POT1_SOIL_MOISTURE,
                         snapshot->soil_moisture.raw_adc_value,
                         snapshot->soil_moisture.raw_mv,
                         snapshot->soil_moisture.value_pct,
                         pot_sensor_status_text(snapshot->soil_moisture.status),
                         "none");
            } else {
                ESP_LOGW(TAG,
                         "SENSOR_FAIL pot[%u].soil_moisture GPIO%d raw=%d mv=unavailable status=%s reason=%s result=%s",
                         (unsigned int)pot_id,
                         pot_id == 0U ? VAZON_GPIO_POT0_SOIL_MOISTURE
                                      : VAZON_GPIO_POT1_SOIL_MOISTURE,
                         snapshot->soil_moisture.raw_adc_value,
                         pot_sensor_status_text(snapshot->soil_moisture.status),
                         "moisture_read_failed",
                         esp_err_to_name(moisture_result));
            }
            if (temperature_result == ESP_OK && temperature_present) {
                ESP_LOGI(TAG,
                         "SENSOR_OK pot[%u].soil_temperature GPIO%d OneWire[%u] temperature=%.2fC status=%s result=ok",
                         (unsigned int)pot_id, VAZON_GPIO_POT_TEMP_ONEWIRE,
                         (unsigned int)pot_id,
                         snapshot->soil_temperature.temperature_c,
                         pot_sensor_status_text(snapshot->soil_temperature.status));
            } else {
                ESP_LOGW(TAG,
                         "SENSOR_FAIL pot[%u].soil_temperature GPIO%d OneWire[%u] temperature=unknown status=%s reason=%s result=%s",
                         (unsigned int)pot_id, VAZON_GPIO_POT_TEMP_ONEWIRE,
                         (unsigned int)pot_id,
                         pot_sensor_status_text(snapshot->soil_temperature.status),
                         temperature_present ? "temperature_read_failed"
                                             : "temperature_missing",
                         esp_err_to_name(temperature_result));
            }
            unlock_runtime(runtime_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(POT_READ_INTERVAL_MS));
    }
}

static esp_err_t apply_light_output(bool on, void *context)
{
    (void)context;
    return vazon_gpio_outputs_set(VAZON_GPIO_OUTPUT_LIGHT, on);
}

static const char *light_output_text(vazon_light_output_t output)
{
    return output == VAZON_LIGHT_OUTPUT_ON ? "on" :
           output == VAZON_LIGHT_OUTPUT_OFF ? "off" : "unknown";
}

static const char *light_status_reason_text(vazon_light_status_reason_t reason)
{
    switch (reason) {
    case VAZON_LIGHT_STATUS_REASON_MAINTENANCE_SERVICE_LIGHT:
        return "maintenance_service_light";
    case VAZON_LIGHT_STATUS_REASON_DAY_WINDOW_UNKNOWN:
        return "day_window_unknown";
    case VAZON_LIGHT_STATUS_REASON_OUTPUT_FAILED:
        return "output_failed";
    case VAZON_LIGHT_STATUS_REASON_NONE:
    default:
        return "none";
    }
}

static const char *maintenance_reason_text(vazon_maintenance_reason_t reason)
{
    switch (reason) {
    case VAZON_MAINTENANCE_REASON_MANUAL:
        return "manual";
    case VAZON_MAINTENANCE_REASON_EXTERNAL:
        return "external";
    case VAZON_MAINTENANCE_REASON_DOOR:
        return "door";
    case VAZON_MAINTENANCE_REASON_UNKNOWN:
    default:
        return "unknown";
    }
}

static void set_door_service_and_log(bool required)
{
    const vazon_maintenance_t previous =
        vazon_runtime_core_get_context(&runtime_core)->maintenance;
    ESP_ERROR_CHECK(vazon_runtime_core_set_door_service_required(&runtime_core,
                                                                  required));
    const vazon_maintenance_t current =
        vazon_runtime_core_get_context(&runtime_core)->maintenance;

    if (current.active != previous.active || current.reason != previous.reason) {
        ESP_LOGI(TAG, "Maintenance active=%s reason=%s",
                 current.active ? "true" : "false",
                 maintenance_reason_text(current.reason));
    }
}

static void evaluate_light_and_log(void)
{
    const vazon_light_snapshot_t previous =
        *vazon_light_module_get_snapshot(&light_module);
    const esp_err_t result = vazon_light_module_evaluate(&light_module);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Light evaluation failed: %s", esp_err_to_name(result));
        return;
    }

    const vazon_light_snapshot_t *current =
        vazon_light_module_get_snapshot(&light_module);
    if (current->output != previous.output || current->status != previous.status ||
        current->status_reason != previous.status_reason) {
        ESP_LOGI(TAG, "Light output=%s status=%s reason=%s",
                 light_output_text(current->output),
                 current->status == VAZON_LIGHT_STATUS_OK ? "ok" : "warning",
                 light_status_reason_text(current->status_reason));
    }
}

static const char *fan_status_reason_text(vazon_fan_status_reason_t reason)
{
    switch (reason) {
    case VAZON_FAN_STATUS_REASON_DOOR_OPEN:
        return "door_open";
    case VAZON_FAN_STATUS_REASON_DOOR_UNKNOWN:
        return "door_unknown";
    case VAZON_FAN_STATUS_REASON_CLIMATE_INVALID:
        return "climate_invalid";
    case VAZON_FAN_STATUS_REASON_DAY_WINDOW_UNKNOWN:
        return "day_window_unknown";
    case VAZON_FAN_STATUS_REASON_OUTPUT_FAILED:
        return "output_failed";
    case VAZON_FAN_STATUS_REASON_NONE:
    default:
        return "none";
    }
}

static const char *fan_status_text(vazon_fan_status_t status)
{
    switch (status) {
    case VAZON_FAN_STATUS_OK:
        return "ok";
    case VAZON_FAN_STATUS_WARNING:
        return "warning";
    case VAZON_FAN_STATUS_ERROR:
        return "error";
    case VAZON_FAN_STATUS_UNKNOWN:
    default:
        return "unknown";
    }
}

static void evaluate_fan_binary_and_log(vazon_door_state_t door_state,
                                        uint64_t now_ms)
{
    const vazon_climate_snapshot_t *climate =
        vazon_climate_module_get_snapshot(&climate_module);
    const vazon_fan_snapshot_t previous = *vazon_fan_module_get_snapshot(&fan_module);
    const vazon_fan_inputs_t inputs = {
        .global_context = vazon_runtime_core_get_context(&runtime_core),
        .door_state = door_state,
        .climate_delta_valid = climate->data_valid,
        .climate_delta_stale = climate->data_stale,
        .rh_delta_pct = climate->rh_delta_pct,
        .now_ms = now_ms,
    };
    const esp_err_t evaluate_result = vazon_fan_module_evaluate(&fan_module, &inputs);
    if (evaluate_result != ESP_OK) {
        ESP_LOGE(TAG, "Fan evaluation failed: %s", esp_err_to_name(evaluate_result));
        return;
    }

    const vazon_fan_snapshot_t *current = vazon_fan_module_get_snapshot(&fan_module);
    const bool request_on = current->output_request == VAZON_FAN_OUTPUT_ON;
    const bool output_matches =
        (request_on && current->output == VAZON_FAN_OUTPUT_ON) ||
        (!request_on && current->output == VAZON_FAN_OUTPUT_OFF);
    if (!output_matches) {
        const esp_err_t driver_result =
            vazon_gpio_outputs_set(VAZON_GPIO_OUTPUT_MAIN_FAN, request_on);
        const esp_err_t report_result = vazon_fan_module_report_output_result(
            &fan_module, request_on ? 100U : 0U, driver_result);
        if (report_result != ESP_OK) {
            ESP_LOGE(TAG, "Fan binary output failed: %s", esp_err_to_name(report_result));
        }
        current = vazon_fan_module_get_snapshot(&fan_module);
    }

    if (current->output != previous.output || current->auto_state != previous.auto_state ||
        current->status != previous.status ||
        current->status_reason != previous.status_reason) {
        ESP_LOGI(TAG, "Fan output=%s applied=%d%% requested=%d%% status=%s reason=%s",
                 current->output == VAZON_FAN_OUTPUT_ON ? "on" : "off",
                 current->applied_power_pct,
                 current->requested_power_pct,
                 fan_status_text(current->status),
                 fan_status_reason_text(current->status_reason));
    }
}

static const char *humidifier_status_reason_text(
    vazon_humidifier_status_reason_t reason)
{
    switch (reason) {
    case VAZON_HUMIDIFIER_STATUS_REASON_DOOR_OPEN:
        return "door_open";
    case VAZON_HUMIDIFIER_STATUS_REASON_DOOR_UNKNOWN:
        return "door_unknown";
    case VAZON_HUMIDIFIER_STATUS_REASON_NO_WATER:
        return "no_water";
    case VAZON_HUMIDIFIER_STATUS_REASON_WATER_UNKNOWN:
        return "water_unknown";
    case VAZON_HUMIDIFIER_STATUS_REASON_DAY_WINDOW_UNKNOWN:
        return "day_window_unknown";
    case VAZON_HUMIDIFIER_STATUS_REASON_CLIMATE_INVALID:
        return "climate_invalid";
    case VAZON_HUMIDIFIER_STATUS_REASON_OUTPUT_FAILED:
        return "output_failed";
    case VAZON_HUMIDIFIER_STATUS_REASON_NONE:
    default:
        return "none";
    }
}

static const char *humidifier_status_text(vazon_humidifier_status_t status)
{
    switch (status) {
    case VAZON_HUMIDIFIER_STATUS_OK:
        return "ok";
    case VAZON_HUMIDIFIER_STATUS_WARNING:
        return "warning";
    case VAZON_HUMIDIFIER_STATUS_ERROR:
        return "error";
    case VAZON_HUMIDIFIER_STATUS_INACTIVE:
        return "inactive";
    case VAZON_HUMIDIFIER_STATUS_UNKNOWN:
    default:
        return "unknown";
    }
}

static const char *humidifier_water_text(vazon_humidifier_water_status_t status)
{
    switch (status) {
    case VAZON_HUMIDIFIER_WATER_EMPTY:
        return "empty";
    case VAZON_HUMIDIFIER_WATER_PRESENT:
        return "present";
    case VAZON_HUMIDIFIER_WATER_UNKNOWN:
    default:
        return "unknown";
    }
}

static const char *humidifier_fan_text(vazon_humidifier_fan_output_t output)
{
    switch (output) {
    case VAZON_HUMIDIFIER_FAN_OFF:
        return "off";
    case VAZON_HUMIDIFIER_FAN_ON:
        return "on";
    case VAZON_HUMIDIFIER_FAN_POST_RUN:
        return "post_run";
    case VAZON_HUMIDIFIER_FAN_UNKNOWN:
    default:
        return "unknown";
    }
}

static void evaluate_humidifier_and_log(vazon_door_state_t door_state,
                                        uint64_t now_ms)
{
    const vazon_climate_snapshot_t *climate =
        vazon_climate_module_get_snapshot(&climate_module);
    int water_raw = 0;
    const esp_err_t water_result =
        vazon_gpio_inputs_read_humidifier_water_raw(&water_raw);
    const vazon_humidifier_snapshot_t previous =
        *vazon_humidifier_module_get_snapshot(&humidifier_module);
    const vazon_humidifier_inputs_t inputs = {
        .global_context = vazon_runtime_core_get_context(&runtime_core),
        .door_state = door_state,
        .water_raw_valid = water_result == ESP_OK,
        .water_raw_level = water_raw,
        .climate_valid = climate->data_valid,
        .climate_stale = climate->data_stale,
        .zone_0_rh_pct = climate->sensor_0x44.humidity_pct,
        .zone_1_rh_pct = climate->sensor_0x45.humidity_pct,
        .now_ms = now_ms,
    };
    const esp_err_t evaluate_result =
        vazon_humidifier_module_evaluate(&humidifier_module, &inputs);
    if (evaluate_result != ESP_OK) {
        ESP_LOGE(TAG, "Humidifier evaluation failed: %s",
                 esp_err_to_name(evaluate_result));
        return;
    }

    const vazon_humidifier_snapshot_t *current =
        vazon_humidifier_module_get_snapshot(&humidifier_module);
    const bool fan_request_on =
        current->fan_output_request != VAZON_HUMIDIFIER_FAN_OFF;
    const bool fan_is_on = current->fan_output == VAZON_HUMIDIFIER_FAN_ON ||
                           current->fan_output == VAZON_HUMIDIFIER_FAN_POST_RUN;
    if (fan_request_on && !fan_is_on) {
        const esp_err_t result =
            vazon_gpio_outputs_set(VAZON_GPIO_OUTPUT_HUMIDIFIER_FAN, true);
        const esp_err_t report_result =
            vazon_humidifier_module_report_fan_output(&humidifier_module,
                                                       true, result);
        if (report_result != ESP_OK) {
            ESP_LOGE(TAG, "Humidifier fan output failed: %s",
                     esp_err_to_name(report_result));
        }
    } else if (fan_request_on &&
               current->fan_output != current->fan_output_request) {
        const esp_err_t report_result =
            vazon_humidifier_module_report_fan_output(&humidifier_module,
                                                       true, ESP_OK);
        if (report_result != ESP_OK) {
            ESP_LOGE(TAG, "Humidifier fan state update failed: %s",
                     esp_err_to_name(report_result));
        }
    }

    current = vazon_humidifier_module_get_snapshot(&humidifier_module);
    const bool mist_request_on =
        current->mist_output_request == VAZON_HUMIDIFIER_OUTPUT_ON;
    const bool mist_is_on = current->mist_output == VAZON_HUMIDIFIER_OUTPUT_ON;
    const bool mist_state_known =
        current->mist_output != VAZON_HUMIDIFIER_OUTPUT_UNKNOWN;
    uint8_t mist_duty_pct = 0U;
    if (mist_request_on) {
        switch (current->settings.mist_power_level) {
        case VAZON_HUMIDIFIER_MIST_POWER_LOW:
            mist_duty_pct = VAZON_HUMIDIFIER_MIST_PWM_LOW_DUTY_PCT;
            break;
        case VAZON_HUMIDIFIER_MIST_POWER_HIGH:
            mist_duty_pct = VAZON_HUMIDIFIER_MIST_PWM_HIGH_DUTY_PCT;
            break;
        case VAZON_HUMIDIFIER_MIST_POWER_MEDIUM:
        default:
            mist_duty_pct = VAZON_HUMIDIFIER_MIST_PWM_MEDIUM_DUTY_PCT;
            break;
        }
    }
    if (!mist_state_known || mist_request_on != mist_is_on ||
        mist_duty_pct != humidifier_mist_applied_duty_pct) {
        const esp_err_t result =
            vazon_gpio_outputs_set_humidifier_mist_duty(mist_duty_pct);
        if (result == ESP_OK) {
            humidifier_mist_applied_duty_pct = mist_duty_pct;
            ESP_LOGI(TAG, "Humidifier mist PWM GPIO%d frequency=%uHz duty=%u%%",
                     VAZON_GPIO_HUMIDIFIER_MIST,
                     VAZON_HUMIDIFIER_MIST_PWM_FREQUENCY_HZ,
                     mist_duty_pct);
        }
        const esp_err_t report_result = vazon_humidifier_module_report_mist_output(
            &humidifier_module, mist_request_on, result);
        if (report_result != ESP_OK) {
            ESP_LOGE(TAG, "Humidifier mist output failed: %s",
                     esp_err_to_name(report_result));
        }
    }

    current = vazon_humidifier_module_get_snapshot(&humidifier_module);
    const bool fan_still_requested =
        current->fan_output_request != VAZON_HUMIDIFIER_FAN_OFF;
    const bool fan_still_on = current->fan_output == VAZON_HUMIDIFIER_FAN_ON ||
                              current->fan_output == VAZON_HUMIDIFIER_FAN_POST_RUN;
    if (!fan_still_requested && fan_still_on) {
        const esp_err_t result =
            vazon_gpio_outputs_set(VAZON_GPIO_OUTPUT_HUMIDIFIER_FAN, false);
        const esp_err_t report_result =
            vazon_humidifier_module_report_fan_output(&humidifier_module,
                                                       false, result);
        if (report_result != ESP_OK) {
            ESP_LOGE(TAG, "Humidifier fan output failed: %s",
                     esp_err_to_name(report_result));
        }
    }

    current = vazon_humidifier_module_get_snapshot(&humidifier_module);
    if (current->water_status != previous.water_status) {
        ESP_LOGI(TAG,
                 "%s humidifier.water GPIO%d raw=%d state=%s result=%s",
                 water_result == ESP_OK ? "SENSOR_OK" : "SENSOR_FAIL",
                 VAZON_GPIO_HUMIDIFIER_WATER, water_raw,
                 humidifier_water_text(current->water_status),
                 water_result == ESP_OK ? "ok" : esp_err_to_name(water_result));
    }
    if (current->water_status != previous.water_status ||
        current->mist_output != previous.mist_output ||
        current->fan_output != previous.fan_output ||
        current->status != previous.status ||
        current->status_reason != previous.status_reason) {
        ESP_LOGI(TAG,
                 "Humidifier mist=%s local_fan=%s status=%s reason=%s",
                 current->mist_output == VAZON_HUMIDIFIER_OUTPUT_ON ? "on" : "off",
                 humidifier_fan_text(current->fan_output),
                 humidifier_status_text(current->status),
                 humidifier_status_reason_text(current->status_reason));
    }
}

static const char *door_state_text(vazon_door_state_t state)
{
    switch (state) {
    case VAZON_DOOR_STATE_CLOSED:
        return "closed";
    case VAZON_DOOR_STATE_OPEN:
        return "open";
    case VAZON_DOOR_STATE_UNKNOWN:
    default:
        return "unknown";
    }
}

static void log_door_snapshot(const vazon_door_snapshot_t *snapshot,
                              int raw_level)
{
    if (snapshot->status == VAZON_DOOR_STATUS_WARNING) {
        ESP_LOGW(TAG,
                 "SENSOR_WARN door GPIO%d raw=%d state=%s status=warning reason=door_unstable result=unstable",
                 VAZON_GPIO_DOOR_REED, raw_level,
                 door_state_text(snapshot->state));
        return;
    }

    ESP_LOGI(TAG, "SENSOR_OK door GPIO%d raw=%d state=%s status=%s result=ok",
             VAZON_GPIO_DOOR_REED, raw_level, door_state_text(snapshot->state),
             snapshot->status == VAZON_DOOR_STATUS_OK ? "ok" : "unknown");
}

static vazon_system_module_status_t climate_system_status(
    vazon_climate_status_t status)
{
    switch (status) {
    case VAZON_CLIMATE_STATUS_OK:
        return VAZON_SYSTEM_MODULE_STATUS_OK;
    case VAZON_CLIMATE_STATUS_WARNING:
        return VAZON_SYSTEM_MODULE_STATUS_WARNING;
    case VAZON_CLIMATE_STATUS_ERROR:
        return VAZON_SYSTEM_MODULE_STATUS_ERROR;
    case VAZON_CLIMATE_STATUS_INACTIVE:
        return VAZON_SYSTEM_MODULE_STATUS_INACTIVE;
    case VAZON_CLIMATE_STATUS_UNKNOWN:
    default:
        return VAZON_SYSTEM_MODULE_STATUS_UNKNOWN;
    }
}

static vazon_system_module_status_t light_system_status(vazon_light_status_t status)
{
    switch (status) {
    case VAZON_LIGHT_STATUS_OK:
        return VAZON_SYSTEM_MODULE_STATUS_OK;
    case VAZON_LIGHT_STATUS_WARNING:
        return VAZON_SYSTEM_MODULE_STATUS_WARNING;
    case VAZON_LIGHT_STATUS_ERROR:
        return VAZON_SYSTEM_MODULE_STATUS_ERROR;
    case VAZON_LIGHT_STATUS_INACTIVE:
        return VAZON_SYSTEM_MODULE_STATUS_INACTIVE;
    case VAZON_LIGHT_STATUS_UNKNOWN:
    default:
        return VAZON_SYSTEM_MODULE_STATUS_UNKNOWN;
    }
}

static vazon_system_module_status_t fan_system_status(vazon_fan_status_t status)
{
    switch (status) {
    case VAZON_FAN_STATUS_OK:
        return VAZON_SYSTEM_MODULE_STATUS_OK;
    case VAZON_FAN_STATUS_WARNING:
        return VAZON_SYSTEM_MODULE_STATUS_WARNING;
    case VAZON_FAN_STATUS_ERROR:
        return VAZON_SYSTEM_MODULE_STATUS_ERROR;
    case VAZON_FAN_STATUS_UNKNOWN:
    default:
        return VAZON_SYSTEM_MODULE_STATUS_UNKNOWN;
    }
}

static vazon_system_module_status_t humidifier_system_status(
    vazon_humidifier_status_t status)
{
    switch (status) {
    case VAZON_HUMIDIFIER_STATUS_OK:
        return VAZON_SYSTEM_MODULE_STATUS_OK;
    case VAZON_HUMIDIFIER_STATUS_WARNING:
        return VAZON_SYSTEM_MODULE_STATUS_WARNING;
    case VAZON_HUMIDIFIER_STATUS_ERROR:
        return VAZON_SYSTEM_MODULE_STATUS_ERROR;
    case VAZON_HUMIDIFIER_STATUS_INACTIVE:
        return VAZON_SYSTEM_MODULE_STATUS_INACTIVE;
    case VAZON_HUMIDIFIER_STATUS_UNKNOWN:
    default:
        return VAZON_SYSTEM_MODULE_STATUS_UNKNOWN;
    }
}

static vazon_system_module_status_t door_system_status(vazon_door_status_t status)
{
    switch (status) {
    case VAZON_DOOR_STATUS_OK:
        return VAZON_SYSTEM_MODULE_STATUS_OK;
    case VAZON_DOOR_STATUS_WARNING:
        return VAZON_SYSTEM_MODULE_STATUS_WARNING;
    case VAZON_DOOR_STATUS_ERROR:
        return VAZON_SYSTEM_MODULE_STATUS_ERROR;
    case VAZON_DOOR_STATUS_INACTIVE:
        return VAZON_SYSTEM_MODULE_STATUS_INACTIVE;
    case VAZON_DOOR_STATUS_UNKNOWN:
    default:
        return VAZON_SYSTEM_MODULE_STATUS_UNKNOWN;
    }
}

static const char *system_status_text(vazon_system_status_value_t status)
{
    switch (status) {
    case VAZON_SYSTEM_STATUS_OK:
        return "ok";
    case VAZON_SYSTEM_STATUS_WARNING:
        return "warning";
    case VAZON_SYSTEM_STATUS_ERROR:
        return "error";
    case VAZON_SYSTEM_STATUS_UNKNOWN:
    default:
        return "unknown";
    }
}

static const char *affected_system_text(vazon_system_affected_t affected)
{
    switch (affected) {
    case VAZON_SYSTEM_AFFECTED_CLIMATE:
        return "climate";
    case VAZON_SYSTEM_AFFECTED_POT:
        return "pot";
    case VAZON_SYSTEM_AFFECTED_LIGHT:
        return "light";
    case VAZON_SYSTEM_AFFECTED_FAN:
        return "fan";
    case VAZON_SYSTEM_AFFECTED_HUMIDIFIER:
        return "humidifier";
    case VAZON_SYSTEM_AFFECTED_DOOR:
        return "door";
    case VAZON_SYSTEM_AFFECTED_CONNECTION:
        return "connection";
    case VAZON_SYSTEM_AFFECTED_SYSTEM:
        return "system";
    case VAZON_SYSTEM_AFFECTED_NONE:
    default:
        return "none";
    }
}

static esp_err_t apply_status_led(bool green_on, bool red_on, void *context)
{
    (void)context;
    return vazon_gpio_outputs_set_status_led(green_on, red_on);
}

static const char *status_led_pattern_text(vazon_status_led_pattern_t pattern)
{
    switch (pattern) {
    case VAZON_STATUS_LED_PATTERN_NORMAL:
        return "normal";
    case VAZON_STATUS_LED_PATTERN_PROVISIONING:
        return "provisioning";
    case VAZON_STATUS_LED_PATTERN_SYSTEM_ERROR:
        return "system_error";
    case VAZON_STATUS_LED_PATTERN_LOCAL_WARNING:
        return "local_warning";
    case VAZON_STATUS_LED_PATTERN_MQTT_OFFLINE:
        return "mqtt_offline";
    case VAZON_STATUS_LED_PATTERN_WIFI_OFFLINE:
        return "wifi_offline";
    case VAZON_STATUS_LED_PATTERN_UNKNOWN:
    default:
        return "unknown";
    }
}

static void update_system_status_and_log(vazon_door_state_t door_state,
                                         bool door_fault,
                                         uint64_t now_ms)
{
    vazon_system_status_snapshot_t previous;
    ESP_ERROR_CHECK(vazon_system_status_get_snapshot(&system_status, &previous));

    const vazon_climate_snapshot_t *climate =
        vazon_climate_module_get_snapshot(&climate_module);
    const bool climate_missing =
        climate->status_reason == VAZON_CLIMATE_REASON_SHT31_0X44_MISSING ||
        climate->status_reason == VAZON_CLIMATE_REASON_SHT31_0X45_MISSING ||
        climate->status_reason == VAZON_CLIMATE_REASON_SHT31_STALE;
    ESP_ERROR_CHECK(vazon_system_status_update_module(
        &system_status, VAZON_SYSTEM_MODULE_CLIMATE,
        climate_system_status(climate->status),
        climate_reason_text(climate->status_reason), climate_missing, false,
        now_ms));

    const vazon_light_snapshot_t *light =
        vazon_light_module_get_snapshot(&light_module);
    ESP_ERROR_CHECK(vazon_system_status_update_module(
        &system_status, VAZON_SYSTEM_MODULE_LIGHT,
        light_system_status(light->status),
        light_status_reason_text(light->status_reason), false,
        light->status_reason == VAZON_LIGHT_STATUS_REASON_OUTPUT_FAILED,
        now_ms));

    const vazon_fan_snapshot_t *fan = vazon_fan_module_get_snapshot(&fan_module);
    ESP_ERROR_CHECK(vazon_system_status_update_module(
        &system_status, VAZON_SYSTEM_MODULE_FAN,
        fan_system_status(fan->status), fan_status_reason_text(fan->status_reason),
        false, fan->status_reason == VAZON_FAN_STATUS_REASON_OUTPUT_FAILED,
        now_ms));

    const vazon_humidifier_snapshot_t *humidifier =
        vazon_humidifier_module_get_snapshot(&humidifier_module);
    ESP_ERROR_CHECK(vazon_system_status_update_module(
        &system_status, VAZON_SYSTEM_MODULE_HUMIDIFIER,
        humidifier_system_status(humidifier->status),
        humidifier_status_reason_text(humidifier->status_reason),
        humidifier->status_reason == VAZON_HUMIDIFIER_STATUS_REASON_WATER_UNKNOWN,
        humidifier->status_reason == VAZON_HUMIDIFIER_STATUS_REASON_OUTPUT_FAILED,
        now_ms));

    const vazon_door_snapshot_t *door =
        vazon_door_module_get_snapshot(&door_module);
    ESP_ERROR_CHECK(vazon_system_status_update_module(
        &system_status, VAZON_SYSTEM_MODULE_DOOR,
        door_fault ? VAZON_SYSTEM_MODULE_STATUS_ERROR
                   : door_system_status(door->status),
        door_fault ? "door_unknown"
                   : (door->status_reason == VAZON_DOOR_STATUS_REASON_DOOR_UNSTABLE
                          ? "door_unknown"
                          : "none"),
        door_fault, false, now_ms));

    ESP_ERROR_CHECK(vazon_system_status_evaluate(
        &system_status, vazon_runtime_core_get_context(&runtime_core),
        door_state == VAZON_DOOR_STATE_OPEN, now_ms));
    vazon_system_status_snapshot_t current;
    ESP_ERROR_CHECK(vazon_system_status_get_snapshot(&system_status, &current));
    ESP_ERROR_CHECK(vazon_runtime_core_update_interlocks(
        &runtime_core, current.sensor_missing, current.actuator_failed));

    const vazon_global_context_t *global_context =
        vazon_runtime_core_get_context(&runtime_core);
    const vazon_status_led_inputs_t led_inputs = {
        .provisioning_active = false,
        .maintenance_door_active = global_context->maintenance.door_active,
        .system_status = &current,
        .wifi_state = global_context->connection.wifi_state,
        .mqtt_state = global_context->connection.mqtt_state,
    };
    const vazon_status_led_pattern_t previous_led_pattern =
        vazon_status_led_get_snapshot(&status_led)->pattern;
    const esp_err_t led_result =
        vazon_status_led_evaluate(&status_led, &led_inputs, now_ms);
    if (led_result != ESP_OK) {
        ESP_LOGE(TAG, "Status LED update failed: %s",
                 esp_err_to_name(led_result));
    } else {
        const vazon_status_led_pattern_t current_led_pattern =
            vazon_status_led_get_snapshot(&status_led)->pattern;
        if (current_led_pattern != previous_led_pattern) {
            ESP_LOGI(TAG, "Status LED pattern=%s",
                     status_led_pattern_text(current_led_pattern));
        }
    }

    if (current.status != previous.status ||
        current.status_reason != previous.status_reason ||
        current.affected_system != previous.affected_system) {
        ESP_LOGI(TAG, "System status=%s reason=%s affected=%s primary=%s",
                 system_status_text(current.status), current.status_reason,
                 affected_system_text(current.affected_system),
                 affected_system_text(current.primary_fault));
    }
}

static void door_runtime_task(void *context)
{
    vazon_door_module_t *module = (vazon_door_module_t *)context;

    while (true) {
        const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000);
        ESP_ERROR_CHECK(lock_runtime(runtime_mutex));
        read_climate_and_log(now_ms);
        int raw_level = 0;
        const esp_err_t read_result = vazon_gpio_inputs_read_door_raw(&raw_level);
        if (read_result != ESP_OK) {
            ESP_LOGE(TAG, "Door input read failed: %s", esp_err_to_name(read_result));
            set_door_service_and_log(true);
            evaluate_light_and_log();
            evaluate_fan_binary_and_log(VAZON_DOOR_STATE_UNKNOWN, now_ms);
            evaluate_humidifier_and_log(VAZON_DOOR_STATE_UNKNOWN, now_ms);
            update_system_status_and_log(VAZON_DOOR_STATE_UNKNOWN, true, now_ms);
            unlock_runtime(runtime_mutex);
            vTaskDelay(pdMS_TO_TICKS(DOOR_POLL_INTERVAL_MS));
            continue;
        }

        const vazon_door_snapshot_t previous = *vazon_door_module_get_snapshot(module);
        const esp_err_t update_result =
            vazon_door_module_update_raw(module, raw_level, now_ms);
        if (update_result != ESP_OK) {
            ESP_LOGE(TAG, "Door state update failed: %s", esp_err_to_name(update_result));
            set_door_service_and_log(true);
            evaluate_light_and_log();
            evaluate_fan_binary_and_log(VAZON_DOOR_STATE_UNKNOWN, now_ms);
            evaluate_humidifier_and_log(VAZON_DOOR_STATE_UNKNOWN, now_ms);
            update_system_status_and_log(VAZON_DOOR_STATE_UNKNOWN, true, now_ms);
            unlock_runtime(runtime_mutex);
            vTaskDelay(pdMS_TO_TICKS(DOOR_POLL_INTERVAL_MS));
            continue;
        }

        const vazon_door_snapshot_t *current = vazon_door_module_get_snapshot(module);
        if (current->state != previous.state || current->status != previous.status ||
            current->status_reason != previous.status_reason) {
            log_door_snapshot(current, raw_level);
        }

        const bool door_service_required =
            current->state == VAZON_DOOR_STATE_OPEN ||
            (current->state == VAZON_DOOR_STATE_UNKNOWN &&
             current->status != VAZON_DOOR_STATUS_UNKNOWN);
        set_door_service_and_log(door_service_required);
        evaluate_light_and_log();
        evaluate_fan_binary_and_log(current->state, now_ms);
        evaluate_humidifier_and_log(current->state, now_ms);
        update_system_status_and_log(current->state, false, now_ms);

        unlock_runtime(runtime_mutex);
        vTaskDelay(pdMS_TO_TICKS(DOOR_POLL_INTERVAL_MS));
    }
}

static void mqtt_command_runtime_task(void *context)
{
    vazon_mqtt_command_queue_t *queue =
        (vazon_mqtt_command_queue_t *)context;
    while (true) {
        const esp_err_t result = vazon_mqtt_command_queue_process_next(
            queue, portMAX_DELAY);
        if (result != ESP_OK) {
            ESP_LOGW(TAG, "MQTT command processing failed: %s",
                     esp_err_to_name(result));
        }
    }
}

static void mqtt_state_runtime_task(void *context)
{
    vazon_mqtt_state_publisher_t *publisher =
        (vazon_mqtt_state_publisher_t *)context;
    bool was_connected = false;

    while (true) {
        vazon_mqtt_state_bundle_t bundle = {0};
        bool connected = false;
        if (lock_runtime(runtime_mutex) == ESP_OK) {
            const vazon_global_context_t *global_context =
                vazon_runtime_core_get_context(&runtime_core);
            connected = global_context->connection.mqtt_state ==
                        VAZON_CONNECTION_CONNECTED;
            if (connected) {
                bundle.global_context = *global_context;
                ESP_ERROR_CHECK_WITHOUT_ABORT(vazon_system_status_get_snapshot(
                    &system_status, &bundle.system_status));
                bundle.climate = *vazon_climate_module_get_snapshot(&climate_module);
                bundle.door = *vazon_door_module_get_snapshot(&door_module);
                bundle.light = *vazon_light_module_get_snapshot(&light_module);
                bundle.fan = *vazon_fan_module_get_snapshot(&fan_module);
                bundle.humidifier =
                    *vazon_humidifier_module_get_snapshot(&humidifier_module);
                bundle.pots[0] = *vazon_pot_module_get_snapshot(&pot_modules[0]);
                bundle.pots[1] = *vazon_pot_module_get_snapshot(&pot_modules[1]);
            }
            unlock_runtime(runtime_mutex);
        }

        if (connected) {
            const esp_err_t result = vazon_mqtt_state_publisher_publish(
                publisher, &bundle, !was_connected);
            if (result != ESP_OK) {
                ESP_LOGW(TAG, "MQTT state publish failed: %s",
                         esp_err_to_name(result));
            } else if (!was_connected) {
                ESP_LOGI(TAG, "MQTT retained runtime snapshot published");
            }
        }
        was_connected = connected;
        vTaskDelay(pdMS_TO_TICKS(MQTT_STATE_POLL_MS));
    }
}

static void provisioning_led_task(void *context)
{
    vazon_status_led_t *module = (vazon_status_led_t *)context;
    const vazon_system_status_snapshot_t inactive_system = {
        .status = VAZON_SYSTEM_STATUS_UNKNOWN,
        .status_reason = "provisioning",
        .primary_fault = VAZON_SYSTEM_AFFECTED_NONE,
        .affected_system = VAZON_SYSTEM_AFFECTED_NONE,
        .sensor_missing = false,
        .actuator_failed = false,
    };
    const vazon_status_led_inputs_t inputs = {
        .provisioning_active = true,
        .maintenance_door_active = false,
        .system_status = &inactive_system,
        .wifi_state = VAZON_CONNECTION_DISCONNECTED,
        .mqtt_state = VAZON_CONNECTION_DISCONNECTED,
    };

    while (true) {
        const uint64_t current_ms =
            (uint64_t)esp_timer_get_time() / 1000ULL;
        const esp_err_t result =
            vazon_status_led_evaluate(module, &inputs, current_ms);
        if (result != ESP_OK) {
            ESP_LOGW(TAG, "Provisioning LED update failed: %s",
                     esp_err_to_name(result));
        }
        vTaskDelay(pdMS_TO_TICKS(PROVISIONING_LED_POLL_MS));
    }
}

void vazon_app_core_init(void)
{
    ESP_ERROR_CHECK(vazon_gpio_outputs_init_safe_off());
    ESP_ERROR_CHECK(vazon_gpio_outputs_init_humidifier_mist_pwm());
    ESP_ERROR_CHECK(vazon_gpio_outputs_init_status_led_off());
    ESP_ERROR_CHECK(vazon_i2c_service_scan_climate_bus());

    esp_err_t nvs_result = nvs_flash_init();
    if (nvs_result == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS requires recovery erase");
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_result);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(vazon_gpio_inputs_init());
    ESP_ERROR_CHECK(vazon_status_led_init(&status_led, apply_status_led, NULL));
    TaskHandle_t provisioning_led_task_handle = NULL;
    ESP_ERROR_CHECK(xTaskCreate(provisioning_led_task,
                                "provisioning_led", 2048, &status_led, 4,
                                &provisioning_led_task_handle) == pdPASS
                        ? ESP_OK
                        : ESP_ERR_NO_MEM);
    const vazon_provisioning_button_config_t provisioning_button_config = {
        .check_window_ms = PROVISIONING_CHECK_MS,
        .hold_ms = PROVISIONING_HOLD_MS,
        .poll_interval_ms = PROVISIONING_POLL_MS,
    };
    bool provisioning_requested = false;
    ESP_ERROR_CHECK(vazon_provisioning_button_check(
        &provisioning_button_config, &provisioning_requested));
    if (provisioning_requested) {
        ESP_LOGW(TAG, "Boot mode=provisioning; normal runtime disabled");
        ESP_ERROR_CHECK(vazon_connection_config_clear());
        ESP_ERROR_CHECK(vazon_provisioning_service_start());
        return;
    }

    vTaskDelete(provisioning_led_task_handle);
    ESP_ERROR_CHECK(vazon_gpio_outputs_set_status_led(false, false));
    ESP_LOGI(TAG, "Boot mode=normal");
    if (setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1) != 0) {
        ESP_LOGE(TAG, "Failed to configure Europe/Kyiv timezone");
    }
    tzset();
    ESP_ERROR_CHECK(vazon_gpio_outputs_run_status_led_test());
    ESP_ERROR_CHECK(vazon_gpio_inputs_run_smoke_test());
    ESP_ERROR_CHECK(vazon_onewire_service_scan_pot_temperature_bus());
    ESP_ERROR_CHECK(vazon_adc_service_init());
    ESP_ERROR_CHECK(vazon_adc_service_read_raw_soil_moisture());

    runtime_mutex = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(runtime_mutex != NULL ? ESP_OK : ESP_ERR_NO_MEM);

    vazon_persisted_settings_t stored_settings;
    const esp_err_t settings_load_result =
        vazon_settings_store_load(&stored_settings);
    const bool settings_loaded = settings_load_result == ESP_OK;
    if (settings_load_result == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No stored runtime settings; defaults will be saved");
    } else if (!settings_loaded) {
        ESP_LOGW(TAG, "Runtime settings unavailable: %s",
                 esp_err_to_name(settings_load_result));
    }

    vazon_command_router_init(&command_router);
    vazon_runtime_core_init(&runtime_core);
    if (settings_loaded) {
        char time_on[VAZON_RUNTIME_TIME_TEXT_SIZE];
        char time_off[VAZON_RUNTIME_TIME_TEXT_SIZE];
        memcpy(time_on, stored_settings.time_on, sizeof(time_on));
        memcpy(time_off, stored_settings.time_off, sizeof(time_off));
        time_on[sizeof(time_on) - 1U] = '\0';
        time_off[sizeof(time_off) - 1U] = '\0';
        const vazon_day_window_patch_t day_window_patch = {
            .has_schedule_enabled = true,
            .schedule_enabled = stored_settings.schedule_enabled,
            .time_on = time_on,
            .time_off = time_off,
        };
        const esp_err_t result = vazon_runtime_core_set_day_window(
            &runtime_core, &day_window_patch);
        if (result != ESP_OK) {
            ESP_LOGW(TAG, "Stored day-window invalid; defaults restored");
        }
    }
    ESP_ERROR_CHECK(vazon_runtime_core_register_command_target(&runtime_core,
                                                                &command_router));
    const vazon_system_status_config_t system_status_config = {
        .mqtt_warning_grace_sec = 60U,
    };
    ESP_ERROR_CHECK(vazon_system_status_init(&system_status,
                                              &system_status_config));
    ESP_ERROR_CHECK(vazon_status_led_init(&status_led, apply_status_led, NULL));

    const vazon_climate_settings_t climate_defaults = {
        .temperature_low_warn = 18.0f,
        .temperature_high_warn = 35.0f,
        .humidity_low_warn = 20.0f,
        .humidity_high_warn = 95.0f,
        .sht31_stale_timeout_sec = 300U,
        .temperature_delta_warn = 5.0f,
        .temperature_delta_error = 10.0f,
    };
    esp_err_t module_init_result = vazon_climate_module_init(
        &climate_module,
        settings_loaded ? &stored_settings.climate : &climate_defaults);
    if (module_init_result != ESP_OK && settings_loaded) {
        ESP_LOGW(TAG, "Stored climate settings invalid; defaults restored");
        module_init_result = vazon_climate_module_init(&climate_module,
                                                        &climate_defaults);
    }
    ESP_ERROR_CHECK(module_init_result);
    ESP_ERROR_CHECK(vazon_climate_module_register_command_target(
        &climate_module, &command_router));

    const vazon_light_settings_t light_defaults = {
        .mode = VAZON_LIGHT_MODE_AUTO,
        .manual_state = false,
    };
    module_init_result = vazon_light_module_init(
        &light_module,
        settings_loaded ? &stored_settings.light : &light_defaults,
        vazon_runtime_core_get_context(&runtime_core), apply_light_output, NULL);
    if (module_init_result != ESP_OK && settings_loaded) {
        ESP_LOGW(TAG, "Stored light settings invalid; defaults restored");
        module_init_result = vazon_light_module_init(
            &light_module, &light_defaults,
            vazon_runtime_core_get_context(&runtime_core), apply_light_output,
            NULL);
    }
    ESP_ERROR_CHECK(module_init_result);
    ESP_ERROR_CHECK(vazon_light_module_register_command_target(&light_module,
                                                                &command_router));

    const vazon_fan_settings_t fan_defaults = {
        .mode = VAZON_FAN_MODE_AUTO,
        .runtime = VAZON_FAN_RUNTIME_DAY,
        .auto_strategy = VAZON_FAN_AUTO_STRATEGY_DELTA,
        .manual_duration_sec = 600U,
        .auto_delta_on_pct = 10.0f,
        .auto_delta_off_pct = 5.0f,
        .auto_timer_on_sec = 60U,
        .auto_timer_off_sec = 300U,
        .power_level_pct = 80U,
    };
    module_init_result = vazon_fan_module_init(
        &fan_module, settings_loaded ? &stored_settings.fan : &fan_defaults);
    if (module_init_result != ESP_OK && settings_loaded) {
        ESP_LOGW(TAG, "Stored fan settings invalid; defaults restored");
        module_init_result = vazon_fan_module_init(&fan_module, &fan_defaults);
    }
    ESP_ERROR_CHECK(module_init_result);
    ESP_ERROR_CHECK(vazon_fan_module_register_command_target(
        &fan_module, &command_router));

    const vazon_humidifier_config_t humidifier_config = {
        .water_debounce_ms = 100U,
        .water_unstable_timeout_ms = 1000U,
    };
    const vazon_humidifier_settings_t humidifier_defaults = {
        .mode = VAZON_HUMIDIFIER_MODE_AUTO,
        .runtime = VAZON_HUMIDIFIER_RUNTIME_DAY,
        .rh_start = 55.0f,
        .rh_stop = 75.0f,
        .rh_delta = 10.0f,
        .mist_power_level = VAZON_HUMIDIFIER_MIST_POWER_MEDIUM,
        .manual_mist_duration_sec = 180U,
        .post_fan_sec = 20U,
    };
    module_init_result = vazon_humidifier_module_init(
        &humidifier_module, &humidifier_config,
        settings_loaded ? &stored_settings.humidifier : &humidifier_defaults);
    if (module_init_result != ESP_OK && settings_loaded) {
        ESP_LOGW(TAG, "Stored humidifier settings invalid; defaults restored");
        module_init_result = vazon_humidifier_module_init(
            &humidifier_module, &humidifier_config, &humidifier_defaults);
    }
    ESP_ERROR_CHECK(module_init_result);
    ESP_ERROR_CHECK(vazon_humidifier_module_register_command_target(
        &humidifier_module, &command_router));

    const vazon_pot_settings_t pot_defaults = {
        .soil_moisture_enabled = true,
        .soil_temperature_enabled = true,
        .moisture_stale_timeout_sec = 300U,
        .temperature_stale_timeout_sec = 300U,
        .temperature_low_warn_c = 5.0f,
        .temperature_high_warn_c = 40.0f,
    };
    const vazon_pot_moisture_calibration_t calibration_defaults = {0};
    for (uint8_t pot_id = 0; pot_id < 2U; ++pot_id) {
        module_init_result = vazon_pot_module_init(
            &pot_modules[pot_id], pot_id,
            settings_loaded ? &stored_settings.pots[pot_id] : &pot_defaults,
            settings_loaded ? &stored_settings.pot_calibrations[pot_id]
                            : &calibration_defaults);
        if (module_init_result != ESP_OK && settings_loaded) {
            ESP_LOGW(TAG, "Stored pot[%u] settings invalid; defaults restored",
                     (unsigned int)pot_id);
            module_init_result = vazon_pot_module_init(
                &pot_modules[pot_id], pot_id, &pot_defaults,
                &calibration_defaults);
        }
        ESP_ERROR_CHECK(module_init_result);
        ESP_ERROR_CHECK(vazon_pot_module_register_command_target(
            &pot_modules[pot_id], &command_router));
    }

    vazon_persisted_settings_t active_settings;
    capture_persisted_settings(&active_settings);
    if (!settings_loaded ||
        !vazon_settings_store_equal(&active_settings, &stored_settings)) {
        const esp_err_t result = vazon_settings_store_save(&active_settings);
        if (result == ESP_OK) {
            last_saved_settings = active_settings;
            last_saved_settings_valid = true;
        } else {
            ESP_LOGW(TAG, "Initial runtime settings save failed: %s",
                     esp_err_to_name(result));
        }
    } else {
        last_saved_settings = active_settings;
        last_saved_settings_valid = true;
    }

    char device_id[VAZON_MQTT_CODEC_DEVICE_ID_SIZE];
    ESP_ERROR_CHECK(build_device_id(device_id, sizeof(device_id)));
    const int ota_topic_written = snprintf(
        ota_status_topic, sizeof(ota_status_topic),
        "vazon/v3/%s/service/ota/status", device_id);
    ESP_ERROR_CHECK(ota_topic_written > 0 &&
                            (size_t)ota_topic_written < sizeof(ota_status_topic)
                        ? ESP_OK
                        : ESP_ERR_INVALID_SIZE);
    ESP_ERROR_CHECK(vazon_ota_service_init(
        &ota_service, ota_status_changed, NULL));
    ESP_ERROR_CHECK(vazon_ota_service_register_command_target(
        &ota_service, &command_router));
    ESP_ERROR_CHECK(vazon_mqtt_command_codec_init(
        &mqtt_command_codec, device_id, &command_router));
    ESP_ERROR_CHECK(vazon_mqtt_state_publisher_init(
        &mqtt_state_publisher, device_id, publish_mqtt_message,
        &mqtt_service));
    const vazon_mqtt_command_queue_config_t mqtt_queue_config = {
        .depth = MQTT_COMMAND_QUEUE_DEPTH,
        .lock_runtime = lock_runtime,
        .unlock_runtime = unlock_runtime,
        .runtime_lock_context = runtime_mutex,
        .publish = publish_mqtt_message,
        .publish_context = &mqtt_service,
    };
    ESP_ERROR_CHECK(vazon_mqtt_command_queue_init(
        &mqtt_command_queue, &mqtt_queue_config, &mqtt_command_codec));
    ESP_LOGI(TAG, "MQTT command runtime ready device_id=%s queue_depth=%u",
             device_id, (unsigned int)MQTT_COMMAND_QUEUE_DEPTH);

    bool network_configured = false;
    vazon_connection_config_t connection_config;
    const esp_err_t config_result =
        vazon_connection_config_load(&connection_config);
    if (config_result == ESP_OK) {
        const vazon_mqtt_service_config_t mqtt_config = {
            .device_id = device_id,
            .host = connection_config.mqtt_host,
            .port = connection_config.mqtt_port,
            .username = connection_config.mqtt_username,
            .password = connection_config.mqtt_password,
        };
        const vazon_mqtt_service_callbacks_t mqtt_callbacks = {
            .on_connection = mqtt_connection_changed,
            .on_message = mqtt_message_received,
            .context = NULL,
        };
        ESP_ERROR_CHECK(vazon_mqtt_service_init(
            &mqtt_service, &mqtt_config, &mqtt_callbacks));

        const vazon_wifi_service_config_t wifi_config = {
            .ssid = connection_config.ssid,
            .password = connection_config.wifi_password,
            .on_connection = wifi_connection_changed,
            .callback_context = NULL,
        };
        ESP_ERROR_CHECK(vazon_wifi_service_init(&wifi_service, &wifi_config));
        ESP_ERROR_CHECK(vazon_runtime_core_set_connection(
            &runtime_core, VAZON_CONNECTION_DISCONNECTED,
            VAZON_CONNECTION_DISCONNECTED));
        network_configured = true;
    } else {
        ESP_LOGW(TAG,
                 "No valid connection config; local runtime continues offline (%s)",
                 esp_err_to_name(config_result));
        ESP_ERROR_CHECK(vazon_runtime_core_set_connection(
            &runtime_core, VAZON_CONNECTION_DISCONNECTED,
            VAZON_CONNECTION_DISCONNECTED));
    }
    memset(&connection_config, 0, sizeof(connection_config));

    const vazon_door_config_t door_config = {
        .debounce_ms = DOOR_DEBOUNCE_MS,
        .unstable_timeout_ms = DOOR_UNSTABLE_TIMEOUT_MS,
    };
    ESP_ERROR_CHECK(vazon_door_module_init(&door_module, &door_config));
    ESP_ERROR_CHECK(xTaskCreate(door_runtime_task, "door_runtime", 4096, &door_module, 5,
                                NULL) == pdPASS
                        ? ESP_OK
                        : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(xTaskCreate(pot_runtime_task, "pot_runtime", 4096, NULL, 4,
                                NULL) == pdPASS
                        ? ESP_OK
                        : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(xTaskCreate(mqtt_command_runtime_task,
                                "mqtt_cmd_runtime",
                                MQTT_COMMAND_TASK_STACK,
                                &mqtt_command_queue, 4, NULL) == pdPASS
                                ? ESP_OK
                                : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(xTaskCreate(mqtt_state_runtime_task,
                                "mqtt_state_runtime",
                                MQTT_STATE_TASK_STACK,
                                &mqtt_state_publisher, 4, NULL) == pdPASS
                        ? ESP_OK
                        : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(xTaskCreate(day_window_runtime_task,
                                "day_window_runtime",
                                DAY_WINDOW_TASK_STACK,
                                NULL, 4, NULL) == pdPASS
                        ? ESP_OK
                        : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(xTaskCreate(settings_runtime_task,
                                "settings_runtime",
                                SETTINGS_TASK_STACK,
                                NULL, 3, NULL) == pdPASS
                        ? ESP_OK
                        : ESP_ERR_NO_MEM);
    if (network_configured) {
        ESP_ERROR_CHECK(vazon_wifi_service_start(&wifi_service));
    }
}
