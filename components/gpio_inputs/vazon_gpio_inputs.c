#include "vazon_gpio_inputs.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "vazon_board_config.h"
#include "vazon_gpio_levels.h"

static const char *TAG = "gpio_inputs";
#if VAZON_MAIN_FAN_TACH_ENABLED
static portMUX_TYPE fan_tach_mux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t fan_tach_pulse_count;

static void IRAM_ATTR main_fan_tach_isr(void *context)
{
    (void)context;
    portENTER_CRITICAL_ISR(&fan_tach_mux);
    ++fan_tach_pulse_count;
    portEXIT_CRITICAL_ISR(&fan_tach_mux);
}
#endif

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

#if VAZON_MAIN_FAN_TACH_ENABLED
    const gpio_config_t main_fan_tach_input = {
        .pin_bit_mask = (1ULL << VAZON_GPIO_MAIN_FAN_TACH),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&main_fan_tach_input), TAG,
                        "Failed to configure main fan tach input");
    ESP_RETURN_ON_ERROR(gpio_install_isr_service(ESP_INTR_FLAG_IRAM), TAG,
                        "Failed to install GPIO ISR service");
    ESP_RETURN_ON_ERROR(
        gpio_isr_handler_add(VAZON_GPIO_MAIN_FAN_TACH, main_fan_tach_isr, NULL),
        TAG, "Failed to attach main fan tach ISR");
#else
    ESP_LOGW(TAG,
             "Main fan tachometer temporarily disabled; rotation confirmation unavailable");
#endif
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

esp_err_t vazon_gpio_inputs_take_main_fan_tach_pulses(uint32_t *pulse_count)
{
    if (pulse_count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
#if VAZON_MAIN_FAN_TACH_ENABLED
    portENTER_CRITICAL(&fan_tach_mux);
    *pulse_count = fan_tach_pulse_count;
    fan_tach_pulse_count = 0U;
    portEXIT_CRITICAL(&fan_tach_mux);
    return ESP_OK;
#else
    *pulse_count = 0U;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t vazon_gpio_inputs_run_smoke_test(void)
{
    int door_level = 0;
    int water_level = 0;
    ESP_RETURN_ON_ERROR(vazon_gpio_inputs_read_door_raw(&door_level), TAG,
                        "Failed to read door input");
    ESP_RETURN_ON_ERROR(vazon_gpio_inputs_read_humidifier_water_raw(&water_level), TAG,
                        "Failed to read water input");

    ESP_LOGI(TAG, "SENSOR_OK door GPIO%d raw=%d state=%s result=ok",
             VAZON_GPIO_DOOR_REED, door_level,
             door_level == VAZON_DOOR_CLOSED_LEVEL ? "closed" : "open");
    ESP_LOGI(TAG, "SENSOR_OK humidifier.water GPIO%d raw=%d state=%s result=ok",
             VAZON_GPIO_HUMIDIFIER_WATER, water_level,
             water_level == VAZON_WATER_EMPTY_LEVEL ? "empty" : "present");
    return ESP_OK;
}
