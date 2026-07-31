#include "vazon_i2c_service.h"

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "vazon_board_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const char *TAG = "i2c_service";

#define SHT31_ADDRESS_0              0x44U
#define SHT31_ADDRESS_1              0x45U
#define SHT31_TRANSACTION_TIMEOUT_MS 300
#define SHT31_MEASUREMENT_DELAY_MS   50U
#define SHT31_PROBE_RETRY_COUNT      3U
#define SHT31_PROBE_RETRY_DELAY_MS   10U
#define SHT31_READ_RETRY_COUNT       3U
#define SHT31_RESET_DELAY_MS         5U

static i2c_master_bus_handle_t climate_bus;
static i2c_master_dev_handle_t sht31_devices[2];
static bool sht31_present[2];

static int sht31_index(uint8_t address)
{
    if (address == SHT31_ADDRESS_0) return 0;
    if (address == SHT31_ADDRESS_1) return 1;
    return -1;
}

static uint8_t sht31_crc(const uint8_t *data, size_t length)
{
    uint8_t crc = 0xFFU;
    for (size_t index = 0; index < length; ++index) {
        crc ^= data[index];
        for (unsigned int bit = 0; bit < 8U; ++bit) {
            crc = (crc & 0x80U) != 0U ? (uint8_t)((crc << 1U) ^ 0x31U)
                                      : (uint8_t)(crc << 1U);
        }
    }
    return crc;
}

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
    if (climate_bus != NULL) return ESP_ERR_INVALID_STATE;
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &climate_bus), TAG,
                        "Failed to initialize I2C bus");

    const uint8_t addresses[2] = {SHT31_ADDRESS_0, SHT31_ADDRESS_1};
    const uint8_t reset_command[2] = {0x30U, 0xA2U};
    unsigned int devices_found = 0;
    ESP_LOGI(TAG, "Probing SHT31 addresses at %u Hz: SDA=GPIO%d SCL=GPIO%d",
             VAZON_CLIMATE_I2C_FREQ_HZ,
             VAZON_GPIO_CLIMATE_I2C_SDA,
             VAZON_GPIO_CLIMATE_I2C_SCL);
    for (size_t index = 0; index < 2U; ++index) {
        const uint8_t address = addresses[index];
        const i2c_device_config_t device_config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = address,
            .scl_speed_hz = VAZON_CLIMATE_I2C_FREQ_HZ,
        };
        ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(
                                climate_bus, &device_config, &sht31_devices[index]),
                            TAG, "Failed to bind SHT31 device");

        esp_err_t probe_result = ESP_FAIL;
        for (unsigned int attempt = 0; attempt < SHT31_PROBE_RETRY_COUNT; ++attempt) {
            probe_result = i2c_master_transmit(sht31_devices[index],
                                               reset_command,
                                               sizeof(reset_command),
                                               SHT31_TRANSACTION_TIMEOUT_MS);
            if (probe_result == ESP_OK) break;
            if (attempt + 1U < SHT31_PROBE_RETRY_COUNT) {
                vTaskDelay(pdMS_TO_TICKS(SHT31_PROBE_RETRY_DELAY_MS));
            }
        }

        if (probe_result == ESP_OK) {
            ESP_LOGI(TAG, "SHT31 ACK at 0x%02X", address);
            ++devices_found;
            sht31_present[index] = true;
        } else {
            ESP_LOGW(TAG, "No SHT31 response at 0x%02X after %u attempts: %s",
                     address, SHT31_PROBE_RETRY_COUNT,
                     esp_err_to_name(probe_result));
        }
    }
    ESP_LOGI(TAG, "SHT31 probe complete: %u/2 device(s) found", devices_found);
    return ESP_OK;
}

esp_err_t vazon_i2c_service_sht31_is_present(uint8_t address, bool *present)
{
    const int index = sht31_index(address);
    if (index < 0 || present == NULL) return ESP_ERR_INVALID_ARG;
    if (climate_bus == NULL) return ESP_ERR_INVALID_STATE;
    *present = sht31_present[index];
    return ESP_OK;
}

esp_err_t vazon_i2c_service_read_sht31(uint8_t address,
                                      float *temperature_c,
                                      float *humidity_pct)
{
    const int index = sht31_index(address);
    if (index < 0 || temperature_c == NULL || humidity_pct == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (climate_bus == NULL || sht31_devices[index] == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!sht31_present[index]) return ESP_ERR_NOT_FOUND;

    const uint8_t measurement_command[2] = {0x24U, 0x00U};
    const uint8_t reset_command[2] = {0x30U, 0xA2U};
    uint8_t response[6] = {0};
    esp_err_t result = ESP_FAIL;

    for (unsigned int attempt = 0; attempt < SHT31_READ_RETRY_COUNT; ++attempt) {
        result = i2c_master_transmit(sht31_devices[index],
                                     measurement_command,
                                     sizeof(measurement_command),
                                     SHT31_TRANSACTION_TIMEOUT_MS);
        if (result == ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(SHT31_MEASUREMENT_DELAY_MS));
            result = i2c_master_receive(sht31_devices[index], response,
                                        sizeof(response),
                                        SHT31_TRANSACTION_TIMEOUT_MS);
        }
        if (result == ESP_OK &&
            (sht31_crc(&response[0], 2U) != response[2] ||
             sht31_crc(&response[3], 2U) != response[5])) {
            result = ESP_ERR_INVALID_CRC;
        }
        if (result == ESP_OK) break;

        if (attempt + 1U < SHT31_READ_RETRY_COUNT) {
            (void)i2c_master_transmit(sht31_devices[index], reset_command,
                                      sizeof(reset_command),
                                      SHT31_TRANSACTION_TIMEOUT_MS);
            vTaskDelay(pdMS_TO_TICKS(SHT31_RESET_DELAY_MS));
        }
    }

    if (result != ESP_OK) {
        ESP_LOGW(TAG, "SHT31 0x%02X read failed after %u attempts: %s",
                 address, SHT31_READ_RETRY_COUNT, esp_err_to_name(result));
        return result;
    }

    const uint16_t raw_temperature =
        (uint16_t)(((uint16_t)response[0] << 8U) | response[1]);
    const uint16_t raw_humidity =
        (uint16_t)(((uint16_t)response[3] << 8U) | response[4]);
    *temperature_c = -45.0f + 175.0f * (float)raw_temperature / 65535.0f;
    *humidity_pct = 100.0f * (float)raw_humidity / 65535.0f;
    return ESP_OK;
}
