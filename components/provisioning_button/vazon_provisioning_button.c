#include "vazon_provisioning_button.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "vazon_gpio_inputs.h"

#include <stdint.h>

static const char *TAG = "provisioning_button";

static uint64_t now_ms(void)
{
    return (uint64_t)esp_timer_get_time() / 1000ULL;
}

static esp_err_t button_is_pressed(bool *pressed)
{
    if (pressed == NULL) return ESP_ERR_INVALID_ARG;

    int raw_level = 1;
    const esp_err_t result =
        vazon_gpio_inputs_read_provisioning_button_raw(&raw_level);
    if (result != ESP_OK) return result;

    *pressed = raw_level == 0;
    return ESP_OK;
}

esp_err_t vazon_provisioning_button_check(
    const vazon_provisioning_button_config_t *config,
    bool *provisioning_requested)
{
    if (config == NULL || provisioning_requested == NULL ||
        config->check_window_ms == 0U || config->hold_ms == 0U ||
        config->poll_interval_ms == 0U) {
        return ESP_ERR_INVALID_ARG;
    }

    *provisioning_requested = false;
    const uint64_t check_deadline = now_ms() + config->check_window_ms;
    bool pressed = false;

    ESP_LOGI(TAG, "Provisioning button check window: %u ms",
             config->check_window_ms);
    do {
        esp_err_t result = button_is_pressed(&pressed);
        if (result != ESP_OK) return result;
        if (pressed) break;
        vTaskDelay(pdMS_TO_TICKS(config->poll_interval_ms));
    } while (now_ms() < check_deadline);

    if (!pressed) {
        ESP_LOGI(TAG, "Provisioning not requested");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Button pressed; hold for %u ms to enter provisioning",
             config->hold_ms);
    const uint64_t hold_deadline = now_ms() + config->hold_ms;
    while (now_ms() < hold_deadline) {
        esp_err_t result = button_is_pressed(&pressed);
        if (result != ESP_OK) return result;
        if (!pressed) {
            ESP_LOGI(TAG, "Button released; provisioning cancelled");
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(config->poll_interval_ms));
    }

    *provisioning_requested = true;
    ESP_LOGI(TAG, "Provisioning requested");
    return ESP_OK;
}
