#include "vazon_gpio_inputs.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "vazon_board_config.h"
#include "vazon_gpio_levels.h"

static const char *TAG = "gpio_inputs";

esp_err_t vazon_gpio_inputs_run_smoke_test(void)
{
    const gpio_config_t door_config = {
        .pin_bit_mask = (1ULL << VAZON_GPIO_DOOR_REED),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&door_config), TAG, "Failed to configure door input");

    const gpio_config_t water_config = {
        .pin_bit_mask = (1ULL << VAZON_GPIO_HUMIDIFIER_WATER),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&water_config), TAG, "Failed to configure water input");

    const int door_level = gpio_get_level(VAZON_GPIO_DOOR_REED);
    const int water_level = gpio_get_level(VAZON_GPIO_HUMIDIFIER_WATER);
    ESP_LOGI(TAG, "Raw door GPIO%d=%d (%s)", VAZON_GPIO_DOOR_REED, door_level,
             door_level == VAZON_DOOR_CLOSED_LEVEL ? "closed" : "open");
    ESP_LOGI(TAG, "Raw water GPIO%d=%d (%s)", VAZON_GPIO_HUMIDIFIER_WATER, water_level,
             water_level == VAZON_WATER_EMPTY_LEVEL ? "empty" : "present");
    return ESP_OK;
}
