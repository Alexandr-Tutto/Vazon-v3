#include "vazon_adc_service.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_log.h"
#include "vazon_board_config.h"

static const char *TAG = "adc_service";
static adc_oneshot_unit_handle_t s_adc;

static esp_err_t channel_for_pot(uint8_t pot_id, adc_channel_t *channel)
{
    if (channel == NULL || pot_id > 1) {
        return ESP_ERR_INVALID_ARG;
    }

    *channel = pot_id == 0 ? VAZON_ADC_POT0_CHANNEL : VAZON_ADC_POT1_CHANNEL;
    return ESP_OK;
}

esp_err_t vazon_adc_service_init(void)
{
    if (s_adc != NULL) {
        return ESP_OK;
    }

    const adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = VAZON_ADC_SOIL_MOISTURE_UNIT,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit_config, &s_adc), TAG,
                        "Failed to initialize ADC1");

    const adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    esp_err_t result = adc_oneshot_config_channel(s_adc, VAZON_ADC_POT0_CHANNEL,
                                                  &channel_config);
    if (result != ESP_OK) {
        adc_oneshot_del_unit(s_adc);
        s_adc = NULL;
        return result;
    }
    result = adc_oneshot_config_channel(s_adc, VAZON_ADC_POT1_CHANNEL, &channel_config);
    if (result != ESP_OK) {
        adc_oneshot_del_unit(s_adc);
        s_adc = NULL;
    }
    return result;
}

esp_err_t vazon_adc_service_read_raw(uint8_t pot_id, int *raw_value)
{
    if (s_adc == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    adc_channel_t channel;
    ESP_RETURN_ON_ERROR(channel_for_pot(pot_id, &channel), TAG, "Invalid pot id");
    if (raw_value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return adc_oneshot_read(s_adc, channel, raw_value);
}

esp_err_t vazon_adc_service_read_raw_average(uint8_t pot_id,
                                             size_t sample_count,
                                             int *raw_value)
{
    if (raw_value == NULL || sample_count == 0 || sample_count > 1024) {
        return ESP_ERR_INVALID_ARG;
    }

    int64_t raw_sum = 0;
    for (size_t sample = 0; sample < sample_count; ++sample) {
        int sample_value = 0;
        ESP_RETURN_ON_ERROR(vazon_adc_service_read_raw(pot_id, &sample_value), TAG,
                            "Failed to read pot[%u] ADC", (unsigned int)pot_id);
        raw_sum += sample_value;
    }
    *raw_value = (int)(raw_sum / (int64_t)sample_count);
    return ESP_OK;
}

esp_err_t vazon_adc_service_read_raw_soil_moisture(void)
{
    const adc_channel_t channels[] = {VAZON_ADC_POT0_CHANNEL, VAZON_ADC_POT1_CHANNEL};
    const gpio_num_t gpios[] = {VAZON_GPIO_POT0_SOIL_MOISTURE,
                                VAZON_GPIO_POT1_SOIL_MOISTURE};

    for (uint8_t pot_id = 0; pot_id < 2; ++pot_id) {
        int raw_value = 0;
        ESP_RETURN_ON_ERROR(vazon_adc_service_read_raw_average(pot_id, 32, &raw_value), TAG,
                            "Failed to average pot[%u] ADC", (unsigned int)pot_id);
        ESP_LOGI(TAG, "Raw pot[%u] soil ADC GPIO%d ADC1_CH%d=%d",
                 (unsigned int)pot_id, gpios[pot_id], channels[pot_id], raw_value);
    }
    return ESP_OK;
}
