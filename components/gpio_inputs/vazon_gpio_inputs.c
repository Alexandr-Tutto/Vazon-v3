#include "vazon_gpio_inputs.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "vazon_board_config.h"
#include "vazon_gpio_levels.h"

static const char *TAG = "gpio_inputs";

static esp_err_t read_raw_level(gpio_num_t gpio, int *level)
{
    if (level == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *level = gpio_get_level(gpio);
    return ESP_OK;
}

esp_err_t vazon_gpio_inputs_init(void)
{
    const gpio_config_t pull_up_inputs = {
        .pin_bit_mask = (1ULL << VAZON_GPIO_DOOR_REED) |
                        (1ULL << VAZON_GPIO_PROVISIONING_BTN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&pull_up_inputs), TAG,
                        "Failed to configure pull-up inputs");

    const gpio_config_t external_pull_up_input = {
        .pin_bit_mask = (1ULL << VAZON_GPIO_HUMIDIFIER_WATER),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&external_pull_up_input), TAG,
                        "Failed to configure water input");
    return ESP_OK;
}

esp_err_t vazon_gpio_inputs_read_door_raw(int *level)
{
    return read_raw_level(VAZON_GPIO_DOOR_REED, level);
}

esp_err_t vazon_gpio_inputs_read_humidifier_water_raw(int *level)
{
    return read_raw_level(VAZON_GPIO_HUMIDIFIER_WATER, level);
}

esp_err_t vazon_gpio_inputs_read_provisioning_button_raw(int *level)
{
    return read_raw_level(VAZON_GPIO_PROVISIONING_BTN, level);
}

esp_err_t vazon_gpio_inputs_run_smoke_test(void)
{
    int door_level = 0;
    int water_level = 0;
    ESP_RETURN_ON_ERROR(vazon_gpio_inputs_read_door_raw(&door_level), TAG,
                        "Failed to read door input");
    ESP_RETURN_ON_ERROR(vazon_gpio_inputs_read_humidifier_water_raw(&water_level), TAG,
                        "Failed to read water input");

    ESP_LOGI(TAG, "Raw door GPIO%d=%d (%s)", VAZON_GPIO_DOOR_REED, door_level,
             door_level == VAZON_DOOR_CLOSED_LEVEL ? "closed" : "open");
    ESP_LOGI(TAG, "Raw water GPIO%d=%d (%s)", VAZON_GPIO_HUMIDIFIER_WATER, water_level,
             water_level == VAZON_WATER_EMPTY_LEVEL ? "empty" : "present");
    return ESP_OK;
}
