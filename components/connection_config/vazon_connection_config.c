#include "vazon_connection_config.h"

#include "esp_crc.h"
#include "esp_log.h"
#include "nvs.h"

#include <stddef.h>
#include <string.h>

static const char *TAG = "connection_config";

#define CONFIG_NAMESPACE       "vazon_config"
#define CURRENT_CONFIG_VERSION 1U

static uint32_t calculate_crc(const vazon_connection_config_t *config)
{
    return esp_crc32_le(0U, (const uint8_t *)config,
                        offsetof(vazon_connection_config_t, crc32));
}

static bool terminated_nonempty(const char *value, size_t capacity)
{
    return value != NULL && strnlen(value, capacity) > 0U &&
           strnlen(value, capacity) < capacity;
}

bool vazon_connection_config_is_complete(
    const vazon_connection_config_t *config)
{
    return config != NULL &&
           terminated_nonempty(config->ssid, sizeof(config->ssid)) &&
           terminated_nonempty(config->wifi_password,
                               sizeof(config->wifi_password)) &&
           terminated_nonempty(config->mqtt_host, sizeof(config->mqtt_host)) &&
           config->mqtt_port != 0U &&
           terminated_nonempty(config->mqtt_username,
                               sizeof(config->mqtt_username)) &&
           terminated_nonempty(config->mqtt_password,
                               sizeof(config->mqtt_password)) &&
           config->config_version == CURRENT_CONFIG_VERSION &&
           config->crc32 == calculate_crc(config);
}

static esp_err_t load_string(nvs_handle_t handle, const char *key,
                             char *destination, size_t capacity)
{
    size_t length = capacity;
    const esp_err_t result = nvs_get_str(handle, key, destination, &length);
    if (result != ESP_OK) return result;
    return length > 1U && length <= capacity ? ESP_OK : ESP_ERR_INVALID_SIZE;
}

esp_err_t vazon_connection_config_load(vazon_connection_config_t *config)
{
    if (config == NULL) return ESP_ERR_INVALID_ARG;
    memset(config, 0, sizeof(*config));

    nvs_handle_t handle;
    esp_err_t result = nvs_open(CONFIG_NAMESPACE, NVS_READONLY, &handle);
    if (result != ESP_OK) return result;

    result = load_string(handle, "ssid", config->ssid, sizeof(config->ssid));
    if (result == ESP_OK) {
        result = load_string(handle, "pass", config->wifi_password,
                             sizeof(config->wifi_password));
    }
    if (result == ESP_OK) {
        result = load_string(handle, "mqtt_host", config->mqtt_host,
                             sizeof(config->mqtt_host));
    }
    if (result == ESP_OK) result = nvs_get_u16(handle, "mqtt_port",
                                               &config->mqtt_port);
    if (result == ESP_OK) {
        result = load_string(handle, "mqtt_user", config->mqtt_username,
                             sizeof(config->mqtt_username));
    }
    if (result == ESP_OK) {
        result = load_string(handle, "mqtt_pass", config->mqtt_password,
                             sizeof(config->mqtt_password));
    }
    if (result == ESP_OK) result = nvs_get_u8(handle, "cfg_ver",
                                              &config->config_version);
    if (result == ESP_OK) result = nvs_get_u32(handle, "crc32", &config->crc32);
    nvs_close(handle);

    if (result != ESP_OK) {
        memset(config, 0, sizeof(*config));
        return result;
    }
    if (!vazon_connection_config_is_complete(config)) {
        ESP_LOGW(TAG, "Stored connection config is incomplete or invalid");
        memset(config, 0, sizeof(*config));
        return ESP_ERR_INVALID_CRC;
    }
    ESP_LOGI(TAG, "Stored connection config loaded");
    return ESP_OK;
}

esp_err_t vazon_connection_config_save(
    const vazon_connection_config_t *config)
{
    if (config == NULL) return ESP_ERR_INVALID_ARG;
    if (!terminated_nonempty(config->ssid, sizeof(config->ssid)) ||
        !terminated_nonempty(config->wifi_password,
                            sizeof(config->wifi_password)) ||
        !terminated_nonempty(config->mqtt_host, sizeof(config->mqtt_host)) ||
        config->mqtt_port == 0U ||
        !terminated_nonempty(config->mqtt_username,
                            sizeof(config->mqtt_username)) ||
        !terminated_nonempty(config->mqtt_password,
                            sizeof(config->mqtt_password))) {
        return ESP_ERR_INVALID_ARG;
    }
    vazon_connection_config_t stored = {0};
    memcpy(stored.ssid, config->ssid, strlen(config->ssid) + 1U);
    memcpy(stored.wifi_password, config->wifi_password,
           strlen(config->wifi_password) + 1U);
    memcpy(stored.mqtt_host, config->mqtt_host,
           strlen(config->mqtt_host) + 1U);
    stored.mqtt_port = config->mqtt_port;
    memcpy(stored.mqtt_username, config->mqtt_username,
           strlen(config->mqtt_username) + 1U);
    memcpy(stored.mqtt_password, config->mqtt_password,
           strlen(config->mqtt_password) + 1U);
    stored.config_version = CURRENT_CONFIG_VERSION;
    stored.crc32 = calculate_crc(&stored);
    if (!vazon_connection_config_is_complete(&stored)) return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t result = nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &handle);
    if (result != ESP_OK) return result;

    result = nvs_set_str(handle, "ssid", stored.ssid);
    if (result == ESP_OK) result = nvs_set_str(handle, "pass",
                                               stored.wifi_password);
    if (result == ESP_OK) result = nvs_set_str(handle, "mqtt_host",
                                               stored.mqtt_host);
    if (result == ESP_OK) result = nvs_set_u16(handle, "mqtt_port",
                                               stored.mqtt_port);
    if (result == ESP_OK) result = nvs_set_str(handle, "mqtt_user",
                                               stored.mqtt_username);
    if (result == ESP_OK) result = nvs_set_str(handle, "mqtt_pass",
                                               stored.mqtt_password);
    if (result == ESP_OK) result = nvs_set_u8(handle, "cfg_ver",
                                              stored.config_version);
    if (result == ESP_OK) result = nvs_set_u32(handle, "crc32", stored.crc32);
    if (result == ESP_OK) result = nvs_commit(handle);
    nvs_close(handle);
    if (result == ESP_OK) ESP_LOGI(TAG, "Connection config saved");
    return result;
}

esp_err_t vazon_connection_config_clear(void)
{
    nvs_handle_t handle;
    esp_err_t result = nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &handle);
    if (result == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (result != ESP_OK) return result;

    static const char *const keys[] = {
        "ssid", "pass", "mqtt_host", "mqtt_port",
        "mqtt_user", "mqtt_pass", "cfg_ver", "crc32",
    };
    for (size_t index = 0; index < sizeof(keys) / sizeof(keys[0]); ++index) {
        const esp_err_t erase_result = nvs_erase_key(handle, keys[index]);
        if (erase_result != ESP_OK && erase_result != ESP_ERR_NVS_NOT_FOUND) {
            result = erase_result;
            break;
        }
    }
    if (result == ESP_OK) result = nvs_commit(handle);
    nvs_close(handle);
    if (result == ESP_OK) ESP_LOGI(TAG, "Connection config cleared");
    return result;
}
