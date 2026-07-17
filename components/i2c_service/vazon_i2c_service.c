#include "vazon_i2c_service.h"

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "vazon_board_config.h"

static const char *TAG = "i2c_service";

esp_err_t vazon_i2c_service_scan_climate_bus(void)
{
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = VAZON_CLIMATE_I2C_PORT,
        .sda_io_num = VAZON_GPIO_CLIMATE_I2C_SDA,
        .scl_io_num = VAZON_GPIO_CLIMATE_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus = NULL;
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &bus), TAG, "Failed to initialize I2C bus");

    unsigned int devices_found = 0;
    ESP_LOGI(TAG, "Scanning I2C bus: SDA=GPIO%d SCL=GPIO%d",
             VAZON_GPIO_CLIMATE_I2C_SDA, VAZON_GPIO_CLIMATE_I2C_SCL);
    for (uint8_t address = 0x08; address <= 0x77; ++address) {
        const esp_err_t probe_result = i2c_master_probe(bus, address, 50);
        if (probe_result == ESP_OK) {
            ESP_LOGI(TAG, "I2C device found at 0x%02X", address);
            ++devices_found;
        } else if (probe_result != ESP_ERR_NOT_FOUND && probe_result != ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "I2C probe 0x%02X failed: %s", address, esp_err_to_name(probe_result));
        }
    }
    ESP_LOGI(TAG, "I2C scan complete: %u device(s) found", devices_found);
    ESP_RETURN_ON_ERROR(i2c_del_master_bus(bus), TAG, "Failed to release I2C bus");
    return ESP_OK;
}
