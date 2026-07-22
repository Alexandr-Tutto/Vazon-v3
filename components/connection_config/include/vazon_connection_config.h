#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VAZON_WIFI_SSID_SIZE       32
#define VAZON_WIFI_PASSWORD_SIZE   64
#define VAZON_MQTT_HOST_CONFIG_SIZE 64
#define VAZON_MQTT_USER_CONFIG_SIZE 32
#define VAZON_MQTT_PASS_CONFIG_SIZE 32

typedef struct {
    char ssid[VAZON_WIFI_SSID_SIZE];
    char wifi_password[VAZON_WIFI_PASSWORD_SIZE];
    char mqtt_host[VAZON_MQTT_HOST_CONFIG_SIZE];
    uint16_t mqtt_port;
    char mqtt_username[VAZON_MQTT_USER_CONFIG_SIZE];
    char mqtt_password[VAZON_MQTT_PASS_CONFIG_SIZE];
    uint8_t config_version;
    uint32_t crc32;
} vazon_connection_config_t;

bool vazon_connection_config_is_complete(
    const vazon_connection_config_t *config);
esp_err_t vazon_connection_config_load(vazon_connection_config_t *config);
esp_err_t vazon_connection_config_save(
    const vazon_connection_config_t *config);
esp_err_t vazon_connection_config_clear(void);

#ifdef __cplusplus
}
#endif
