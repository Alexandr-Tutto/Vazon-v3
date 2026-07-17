#include "vazon_onewire_service.h"

#include "ds18b20.h"
#include "esp_check.h"
#include "esp_log.h"
#include "onewire_bus.h"
#include "onewire_device.h"
#include "vazon_board_config.h"

#include <inttypes.h>

static const char *TAG = "onewire_service";

esp_err_t vazon_onewire_service_scan_pot_temperature_bus(void)
{
    const onewire_bus_config_t bus_config = {
        .bus_gpio_num = VAZON_GPIO_POT_TEMP_ONEWIRE,
        .flags = {.en_pull_up = true},
    };
    const onewire_bus_rmt_config_t rmt_config = {.max_rx_bytes = 10};
    onewire_bus_handle_t bus = NULL;
    ESP_RETURN_ON_ERROR(onewire_new_bus_rmt(&bus_config, &rmt_config, &bus), TAG,
                        "Failed to initialize OneWire bus on GPIO%d", VAZON_GPIO_POT_TEMP_ONEWIRE);

    onewire_device_iter_handle_t iterator = NULL;
    esp_err_t result = onewire_new_device_iter(bus, &iterator);
    if (result != ESP_OK) {
        onewire_bus_del(bus);
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
            ESP_LOGI(TAG, "DS18B20[%u] address=%016" PRIx64, count++, address);
            ds18b20_del_device(sensor);
        } else {
            ESP_LOGI(TAG, "Non-DS18B20 OneWire device address=%016" PRIx64, device.address);
        }
    }
    onewire_del_device_iter(iterator);
    onewire_bus_del(bus);
    ESP_LOGI(TAG, "OneWire scan complete: %u DS18B20 device(s) found", count);
    return ESP_OK;
}
