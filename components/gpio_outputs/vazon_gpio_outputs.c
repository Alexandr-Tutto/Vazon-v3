#include "vazon_gpio_outputs.h"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "vazon_board_config.h"
#include "vazon_gpio_levels.h"

static const char *TAG = "gpio_outputs";

esp_err_t vazon_gpio_outputs_init_safe_off(void)
{
    const gpio_num_t actuator_gpios[] = {
        VAZON_GPIO_LIGHT,
        VAZON_GPIO_MAIN_FAN,
        VAZON_GPIO_HUMIDIFIER_FAN,
        VAZON_GPIO_HUMIDIFIER_MIST,
    };

    uint64_t actuator_pin_mask = 0;
    for (size_t i = 0; i < sizeof(actuator_gpios) / sizeof(actuator_gpios[0]); ++i) {
        actuator_pin_mask |= (1ULL << actuator_gpios[i]);
        ESP_RETURN_ON_ERROR(
            gpio_set_level(actuator_gpios[i], VAZON_OUTPUT_ACTIVE_HIGH_OFF_LEVEL),
            TAG,
            "Failed to preset GPIO%d to OFF",
            actuator_gpios[i]);
    }

    const gpio_config_t config = {
        .pin_bit_mask = actuator_pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&config), TAG, "Failed to configure actuator outputs");
    ESP_LOGI(TAG, "Actuator outputs initialized safe OFF");
    return ESP_OK;
}

esp_err_t vazon_gpio_outputs_run_status_led_test(void)
{
    const gpio_config_t config = {
        .pin_bit_mask = (1ULL << VAZON_GPIO_STATUS_LED_GREEN) |
                        (1ULL << VAZON_GPIO_STATUS_LED_RED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_RETURN_ON_ERROR(gpio_set_level(VAZON_GPIO_STATUS_LED_GREEN, VAZON_GPIO_LEVEL_HIGH), TAG,
                        "Failed to preset green status LED OFF");
    ESP_RETURN_ON_ERROR(gpio_set_level(VAZON_GPIO_STATUS_LED_RED, VAZON_GPIO_LEVEL_HIGH), TAG,
                        "Failed to preset red status LED OFF");
    ESP_RETURN_ON_ERROR(gpio_config(&config), TAG, "Failed to configure status LEDs");

    ESP_LOGI(TAG, "Status LED test: green");
    ESP_RETURN_ON_ERROR(gpio_set_level(VAZON_GPIO_STATUS_LED_GREEN, VAZON_GPIO_LEVEL_LOW), TAG,
                        "Failed to turn green status LED ON");
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_RETURN_ON_ERROR(gpio_set_level(VAZON_GPIO_STATUS_LED_GREEN, VAZON_GPIO_LEVEL_HIGH), TAG,
                        "Failed to turn green status LED OFF");

    ESP_LOGI(TAG, "Status LED test: red");
    ESP_RETURN_ON_ERROR(gpio_set_level(VAZON_GPIO_STATUS_LED_RED, VAZON_GPIO_LEVEL_LOW), TAG,
                        "Failed to turn red status LED ON");
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_RETURN_ON_ERROR(gpio_set_level(VAZON_GPIO_STATUS_LED_RED, VAZON_GPIO_LEVEL_HIGH), TAG,
                        "Failed to turn red status LED OFF");

    ESP_LOGI(TAG, "Status LED pattern test complete");
    return ESP_OK;
}

bool vazon_gpio_outputs_is_actuator_gpio(int gpio)
{
    return gpio == VAZON_GPIO_LIGHT || gpio == VAZON_GPIO_MAIN_FAN ||
           gpio == VAZON_GPIO_HUMIDIFIER_FAN || gpio == VAZON_GPIO_HUMIDIFIER_MIST;
}

esp_err_t vazon_gpio_outputs_set_raw(gpio_num_t gpio, int level)
{
    if (!vazon_gpio_outputs_is_actuator_gpio(gpio) || (level != 0 && level != 1)) {
        return ESP_ERR_INVALID_ARG;
    }
    return gpio_set_level(gpio, level);
}

esp_err_t vazon_gpio_outputs_set_all_off(void)
{
    ESP_RETURN_ON_ERROR(gpio_set_level(VAZON_GPIO_LIGHT, VAZON_OUTPUT_ACTIVE_HIGH_OFF_LEVEL), TAG,
                        "Failed to set light OFF");
    ESP_RETURN_ON_ERROR(gpio_set_level(VAZON_GPIO_MAIN_FAN, VAZON_OUTPUT_ACTIVE_HIGH_OFF_LEVEL), TAG,
                        "Failed to set main fan OFF");
    ESP_RETURN_ON_ERROR(gpio_set_level(VAZON_GPIO_HUMIDIFIER_FAN, VAZON_OUTPUT_ACTIVE_HIGH_OFF_LEVEL), TAG,
                        "Failed to set humidifier fan OFF");
    ESP_RETURN_ON_ERROR(gpio_set_level(VAZON_GPIO_HUMIDIFIER_MIST, VAZON_OUTPUT_ACTIVE_HIGH_OFF_LEVEL), TAG,
                        "Failed to set humidifier mist OFF");
    return ESP_OK;
}
