#include "vazon_onewire_service.h"

#include "ds18b20.h"
#include "esp_check.h"
#include "esp_log.h"
#include "onewire_bus.h"
#include "onewire_device.h"
#include "vazon_board_config.h"

#include <inttypes.h>
#include <stddef.h>

static const char *TAG = "onewire_service";
static onewire_bus_handle_t pot_temperature_bus;
static ds18b20_device_handle_t pot_temperature_sensors[2];
static size_t pot_temperature_sensor_count;

esp_err_t vazon_onewire_service_scan_pot_temperature_bus(void)
{
    const onewire_bus_config_t bus_config = {
        .bus_gpio_num = VAZON_GPIO_POT_TEMP_ONEWIRE,
        .flags = {.en_pull_up = true},
    };
    const onewire_bus_rmt_config_t rmt_config = {.max_rx_bytes = 10};
    if (pot_temperature_bus != NULL) return ESP_ERR_INVALID_STATE;
    ESP_RETURN_ON_ERROR(onewire_new_bus_rmt(&bus_config, &rmt_config,
                                            &pot_temperature_bus), TAG,
                        "Failed to initialize OneWire bus on GPIO%d", VAZON_GPIO_POT_TEMP_ONEWIRE);

    onewire_device_iter_handle_t iterator = NULL;
    esp_err_t result = onewire_new_device_iter(pot_temperature_bus, &iterator);
    if (result != ESP_OK) {
        onewire_bus_del(pot_temperature_bus);
        pot_temperature_bus = NULL;
        return result;
    }

    unsigned int count = 0;
    onewire_device_t device;
    ESP_LOGI(TAG, "Scanning OneWire bus on GPIO%d", VAZON_GPIO_POT_TEMP_ONEWIRE);
    while (onewire_device_iter_get_next(iterator, &device) == ESP_OK) {
        ds18b20_config_t config = {};
        ds18b20_device_handle_t sensor = NULL;
        if (ds18b20_new_device_from_enumeration(&device, &config, &sensor) == ESP_OK) {
            onewire_device_address_t address = 0;
            (void)ds18b20_get_device_address(sensor, &address);
            if (count < 2U) {
                pot_temperature_sensors[count] = sensor;
                ESP_LOGI(TAG, "DS18B20[%u] address=%016" PRIx64, count, address);
                ++count;
            } else {
                ESP_LOGW(TAG, "Extra DS18B20 ignored address=%016" PRIx64, address);
                ds18b20_del_device(sensor);
            }
        } else {
            ESP_LOGI(TAG, "Non-DS18B20 OneWire device address=%016" PRIx64, device.address);
        }
    }
    onewire_del_device_iter(iterator);
    pot_temperature_sensor_count = count;
    ESP_LOGI(TAG, "OneWire scan complete: %u DS18B20 device(s) found", count);
    return ESP_OK;
}

esp_err_t vazon_onewire_service_sensor_present(uint8_t pot_id, bool *present)
{
    if (pot_id > 1U || present == NULL) return ESP_ERR_INVALID_ARG;
    if (pot_temperature_bus == NULL) return ESP_ERR_INVALID_STATE;
    *present = pot_id < pot_temperature_sensor_count &&
               pot_temperature_sensors[pot_id] != NULL;
    return ESP_OK;
}

esp_err_t vazon_onewire_service_read_pot_temperatures(
    float temperatures_c[2], esp_err_t read_results[2])
{
    if (temperatures_c == NULL || read_results == NULL) return ESP_ERR_INVALID_ARG;
    if (pot_temperature_bus == NULL) return ESP_ERR_INVALID_STATE;

    for (size_t index = 0; index < 2U; ++index) {
        temperatures_c[index] = 0.0f;
        read_results[index] = index < pot_temperature_sensor_count
                                  ? ESP_ERR_INVALID_STATE
                                  : ESP_ERR_NOT_FOUND;
    }
    if (pot_temperature_sensor_count == 0U) return ESP_OK;

    const esp_err_t conversion_result =
        ds18b20_trigger_temperature_conversion_for_all(pot_temperature_bus);
    if (conversion_result != ESP_OK) {
        for (size_t index = 0; index < pot_temperature_sensor_count; ++index) {
            read_results[index] = conversion_result;
        }
        return ESP_OK;
    }

    for (size_t index = 0; index < pot_temperature_sensor_count; ++index) {
        read_results[index] = ds18b20_get_temperature(
            pot_temperature_sensors[index], &temperatures_c[index]);
    }
    return ESP_OK;
}
