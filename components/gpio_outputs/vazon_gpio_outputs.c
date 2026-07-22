#include "vazon_gpio_outputs.h"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "vazon_board_config.h"
#include "vazon_gpio_levels.h"

static const char *TAG = "gpio_outputs";
static const gpio_num_t ACTUATOR_GPIOS[VAZON_GPIO_OUTPUT_COUNT] = {
    [VAZON_GPIO_OUTPUT_LIGHT] = VAZON_GPIO_LIGHT,
    [VAZON_GPIO_OUTPUT_MAIN_FAN] = VAZON_GPIO_MAIN_FAN,
    [VAZON_GPIO_OUTPUT_HUMIDIFIER_FAN] = VAZON_GPIO_HUMIDIFIER_FAN,
    [VAZON_GPIO_OUTPUT_HUMIDIFIER_MIST] = VAZON_GPIO_HUMIDIFIER_MIST,
};

esp_err_t vazon_gpio_outputs_init_safe_off(void)
{
    uint64_t actuator_pin_mask = 0;
    for (size_t i = 0; i < VAZON_GPIO_OUTPUT_COUNT; ++i) {
        actuator_pin_mask |= (1ULL << ACTUATOR_GPIOS[i]);
        ESP_RETURN_ON_ERROR(
            gpio_set_level(ACTUATOR_GPIOS[i], VAZON_OUTPUT_ACTIVE_HIGH_OFF_LEVEL),
            TAG,
            "Failed to preset GPIO%d to OFF",
            ACTUATOR_GPIOS[i]);
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

esp_err_t vazon_gpio_outputs_init_status_led_off(void)
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

    ESP_LOGI(TAG, "Status LEDs initialized OFF");
    return ESP_OK;
}

esp_err_t vazon_gpio_outputs_run_status_led_test(void)
{
    ESP_RETURN_ON_ERROR(vazon_gpio_outputs_init_status_led_off(), TAG,
                        "Failed to initialize status LEDs");

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

esp_err_t vazon_gpio_outputs_set_status_led(bool green_on, bool red_on)
{
    ESP_RETURN_ON_ERROR(
        gpio_set_level(VAZON_GPIO_STATUS_LED_GREEN,
                       green_on ? VAZON_GPIO_LEVEL_LOW : VAZON_GPIO_LEVEL_HIGH),
        TAG, "Failed to set green status LED");
    ESP_RETURN_ON_ERROR(
        gpio_set_level(VAZON_GPIO_STATUS_LED_RED,
                       red_on ? VAZON_GPIO_LEVEL_LOW : VAZON_GPIO_LEVEL_HIGH),
        TAG, "Failed to set red status LED");
    return ESP_OK;
}

esp_err_t vazon_gpio_outputs_set(vazon_gpio_output_t output, bool on)
{
    if ((unsigned int)output >= VAZON_GPIO_OUTPUT_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    return gpio_set_level(ACTUATOR_GPIOS[output],
                          on ? VAZON_OUTPUT_ACTIVE_HIGH_ON_LEVEL
                             : VAZON_OUTPUT_ACTIVE_HIGH_OFF_LEVEL);
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
    for (vazon_gpio_output_t output = VAZON_GPIO_OUTPUT_LIGHT;
         output < VAZON_GPIO_OUTPUT_COUNT;
         ++output) {
        ESP_RETURN_ON_ERROR(vazon_gpio_outputs_set(output, false), TAG,
                            "Failed to set output %d OFF", output);
    }
    return ESP_OK;
}
