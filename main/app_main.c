#include "esp_log.h"
#include "vazon_board_config.h"
#include "vazon_app_core.h"

static const char *TAG = "vazon_v3";

void app_main(void)
{
    ESP_LOGI(TAG, "Booting %s %s on %s %s",
             VAZON_BOARD_NAME,
             VAZON_BOARD_REVISION,
             VAZON_MCU_MODULE,
             VAZON_MODULE_MEMORY);

    vazon_app_core_init();
}
