#include "vazon_app_core.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "vazon_adc_service.h"
#include "vazon_gpio_inputs.h"
#include "vazon_gpio_outputs.h"
#include "vazon_i2c_service.h"
#include "vazon_onewire_service.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "app_core";

static void bringup_uart_task(void *context)
{
    char line[48];
    ESP_LOGI(TAG, "Bring-up commands: set <4|13|19|26> <0|1>, alloff");

    while (true) {
        if (fgets(line, sizeof(line), stdin) == NULL) {
            clearerr(stdin);
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        int gpio = -1;
        int level = -1;
        if (sscanf(line, "set %d %d", &gpio, &level) == 2) {
            const esp_err_t result = vazon_gpio_outputs_set_raw((gpio_num_t)gpio, level);
            if (result == ESP_OK) {
                ESP_LOGI(TAG, "Bring-up GPIO%d=%d (%s)", gpio, level, level ? "ON" : "OFF");
            } else {
                ESP_LOGW(TAG, "Rejected; allowed GPIOs: 4, 13, 19, 26; levels: 0, 1");
            }
        } else if (strncmp(line, "alloff", 6) == 0) {
            ESP_ERROR_CHECK(vazon_gpio_outputs_set_all_off());
            ESP_LOGI(TAG, "All bring-up outputs OFF");
        } else {
            ESP_LOGW(TAG, "Unknown command; use: set <4|13|19|26> <0|1>, alloff");
        }
    }
}

void vazon_app_core_init(void)
{
    ESP_ERROR_CHECK(vazon_gpio_outputs_init_safe_off());
    ESP_ERROR_CHECK(vazon_gpio_outputs_run_status_led_test());
    ESP_ERROR_CHECK(vazon_gpio_inputs_run_smoke_test());
    ESP_ERROR_CHECK(vazon_i2c_service_scan_climate_bus());
    ESP_ERROR_CHECK(vazon_onewire_service_scan_pot_temperature_bus());
    ESP_ERROR_CHECK(vazon_adc_service_read_raw_soil_moisture());
    ESP_ERROR_CHECK(xTaskCreate(bringup_uart_task, "bringup_uart", 3072, NULL, 5, NULL) == pdPASS
                        ? ESP_OK
                        : ESP_ERR_NO_MEM);
}
