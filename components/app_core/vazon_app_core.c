#include "vazon_app_core.h"

#include "esp_log.h"
#include "vazon_board_config.h"
#include "vazon_gpio_levels.h"

static const char *TAG = "app_core";

void vazon_app_core_init(void)
{
    ESP_LOGI(TAG, "App core init stub");
    ESP_LOGI(TAG, "Light OFF level=%d, door closed level=%d, water present level=%d",
             VAZON_OUTPUT_ACTIVE_LOW_OFF_LEVEL,
             VAZON_DOOR_CLOSED_LEVEL,
             VAZON_WATER_PRESENT_LEVEL);
}
