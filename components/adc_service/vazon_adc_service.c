#include "vazon_adc_service.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_log.h"
#include "vazon_board_config.h"

static const char *TAG = "adc_service";

esp_err_t vazon_adc_service_read_raw_soil_moisture(void)
{
    const adc_oneshot_unit_init_cfg_t unit_config = {.unit_id = VAZON_ADC_SOIL_MOISTURE_UNIT};
    adc_oneshot_unit_handle_t adc = NULL;
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit_config, &adc), TAG, "Failed to initialize ADC1");

    const adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    esp_err_t result = adc_oneshot_config_channel(adc, VAZON_ADC_POT0_CHANNEL, &channel_config);
    if (result == ESP_OK) {
        result = adc_oneshot_config_channel(adc, VAZON_ADC_POT1_CHANNEL, &channel_config);
    }

    const adc_channel_t channels[] = {VAZON_ADC_POT0_CHANNEL, VAZON_ADC_POT1_CHANNEL};
    const gpio_num_t gpios[] = {VAZON_GPIO_POT0_SOIL_MOISTURE, VAZON_GPIO_POT1_SOIL_MOISTURE};
    for (size_t pot = 0; result == ESP_OK && pot < 2; ++pot) {
        int raw_sum = 0;
        for (int sample = 0; sample < 32; ++sample) {
            int raw = 0;
            result = adc_oneshot_read(adc, channels[pot], &raw);
            if (result != ESP_OK) break;
            raw_sum += raw;
        }
        if (result == ESP_OK) {
            ESP_LOGI(TAG, "Raw pot[%u] soil ADC GPIO%d ADC1_CH%d=%d",
                     (unsigned int)pot, gpios[pot], channels[pot], raw_sum / 32);
        }
    }

    const esp_err_t delete_result = adc_oneshot_del_unit(adc);
    if (result != ESP_OK) return result;
    ESP_RETURN_ON_ERROR(delete_result, TAG, "Failed to release ADC1");
    return ESP_OK;
}
