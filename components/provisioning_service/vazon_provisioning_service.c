#include "vazon_provisioning_service.h"

#include "esp_check.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "vazon_connection_config.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROVISIONING_POST_MAX_SIZE 1024U
#define PROVISIONING_AP_PASSWORD   "vazon1234"
#define PROVISIONING_RESTART_MS    1500U

static const char *TAG = "provisioning_service";
static httpd_handle_t http_server;
static bool started;

static const char INDEX_HTML[] =
    "<!doctype html><html lang='uk'><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Vazon setup</title><style>body{font-family:sans-serif;max-width:"
    "520px;margin:24px auto;padding:0 16px;background:#f3f5f7}form{background:"
    "#fff;padding:20px;border-radius:10px}label{display:block;margin-top:14px}"
    "input,button{box-sizing:border-box;width:100%;padding:11px;margin-top:5px}"
    "button{margin-top:22px}</style></head><body><h1>Vazon setup</h1>"
    "<form method='post' action='/config'>"
    "<label>Wi-Fi SSID<input name='ssid' maxlength='31' required></label>"
    "<label>Wi-Fi password<input type='password' name='pass' maxlength='63' "
    "required></label><hr>"
    "<label>MQTT host<input name='mqtt_host' maxlength='63' required></label>"
    "<label>MQTT port<input type='number' name='mqtt_port' value='8883' min='1' "
    "max='65535' required></label>"
    "<label>MQTT username<input name='mqtt_user' maxlength='31' required></label>"
    "<label>MQTT password<input type='password' name='mqtt_pass' maxlength='31' "
    "required></label><button type='submit'>Save and restart</button>"
    "</form></body></html>";

static int hex_value(char value)
{
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

static esp_err_t url_decode(const char *source, char *destination,
                            size_t destination_size)
{
    if (source == NULL || destination == NULL || destination_size == 0U) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t output = 0U;
    for (size_t input = 0U; source[input] != '\0'; ++input) {
        unsigned char decoded = (unsigned char)source[input];
        if (source[input] == '+') {
            decoded = ' ';
        } else if (source[input] == '%') {
            if (source[input + 1U] == '\0' ||
                source[input + 2U] == '\0') {
                return ESP_ERR_INVALID_ARG;
            }
            const int high = hex_value(source[input + 1U]);
            const int low = high >= 0 ? hex_value(source[input + 2U]) : -1;
            if (high < 0 || low < 0) return ESP_ERR_INVALID_ARG;
            decoded = (unsigned char)((high << 4) | low);
            input += 2U;
        }
        if (decoded == 0U || decoded < 0x20U || decoded == 0x7fU ||
            output + 1U >= destination_size) {
            return ESP_ERR_INVALID_ARG;
        }
        destination[output++] = (char)decoded;
    }
    destination[output] = '\0';
    return output > 0U ? ESP_OK : ESP_ERR_INVALID_ARG;
}

static esp_err_t read_form_value(const char *body, const char *key,
                                 char *destination, size_t destination_size)
{
    char encoded[256];
    const esp_err_t result =
        httpd_query_key_value(body, key, encoded, sizeof(encoded));
    if (result != ESP_OK) return result;
    return url_decode(encoded, destination, destination_size);
}

static esp_err_t parse_port(const char *body, uint16_t *port)
{
    char encoded[16];
    if (port == NULL ||
        httpd_query_key_value(body, "mqtt_port", encoded,
                              sizeof(encoded)) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }

    errno = 0;
    char *end = NULL;
    const unsigned long parsed = strtoul(encoded, &end, 10);
    if (errno != 0 || end == encoded || *end != '\0' || parsed == 0UL ||
        parsed > UINT16_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    *port = (uint16_t)parsed;
    return ESP_OK;
}

static esp_err_t parse_config(const char *body,
                              vazon_connection_config_t *config)
{
    if (body == NULL || config == NULL) return ESP_ERR_INVALID_ARG;
    memset(config, 0, sizeof(*config));

    if (read_form_value(body, "ssid", config->ssid,
                        sizeof(config->ssid)) != ESP_OK ||
        read_form_value(body, "pass", config->wifi_password,
                        sizeof(config->wifi_password)) != ESP_OK ||
        read_form_value(body, "mqtt_host", config->mqtt_host,
                        sizeof(config->mqtt_host)) != ESP_OK ||
        parse_port(body, &config->mqtt_port) != ESP_OK ||
        read_form_value(body, "mqtt_user", config->mqtt_username,
                        sizeof(config->mqtt_username)) != ESP_OK ||
        read_form_value(body, "mqtt_pass", config->mqtt_password,
                        sizeof(config->mqtt_password)) != ESP_OK) {
        memset(config, 0, sizeof(*config));
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static esp_err_t root_get_handler(httpd_req_t *request)
{
    httpd_resp_set_type(request, "text/html; charset=utf-8");
    return httpd_resp_send(request, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static void delayed_restart_task(void *context)
{
    (void)context;
    vTaskDelay(pdMS_TO_TICKS(PROVISIONING_RESTART_MS));
    esp_restart();
}

static esp_err_t config_post_handler(httpd_req_t *request)
{
    if (request->content_len == 0U ||
        request->content_len > PROVISIONING_POST_MAX_SIZE) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST,
                            "Invalid form size");
        return ESP_FAIL;
    }

    char body[PROVISIONING_POST_MAX_SIZE + 1U];
    size_t received = 0U;
    while (received < (size_t)request->content_len) {
        const int result = httpd_req_recv(
            request, body + received,
            (size_t)request->content_len - received);
        if (result <= 0) {
            ESP_LOGW(TAG, "Provisioning form receive failed");
            return ESP_FAIL;
        }
        received += (size_t)result;
    }
    body[received] = '\0';

    vazon_connection_config_t config;
    esp_err_t result = parse_config(body, &config);
    memset(body, 0, sizeof(body));
    if (result != ESP_OK) {
        memset(&config, 0, sizeof(config));
        ESP_LOGW(TAG, "Provisioning form rejected");
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST,
                            "Invalid configuration");
        return result;
    }

    result = vazon_connection_config_save(&config);
    memset(&config, 0, sizeof(config));
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save connection config: %s",
                 esp_err_to_name(result));
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "Failed to save configuration");
        return result;
    }

    if (xTaskCreate(delayed_restart_task, "provisioning_restart", 2048,
                    NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Configuration saved but restart task failed");
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "Saved; restart device manually");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Provisioning config accepted; restarting");
    httpd_resp_set_type(request, "text/html; charset=utf-8");
    return httpd_resp_sendstr(
        request, "<h1>Configuration saved. Restarting...</h1>");
}

static const httpd_uri_t ROOT_URI = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
    .user_ctx = NULL,
};

static const httpd_uri_t CONFIG_URI = {
    .uri = "/config",
    .method = HTTP_POST,
    .handler = config_post_handler,
    .user_ctx = NULL,
};

esp_err_t vazon_provisioning_service_start(void)
{
    if (started) return ESP_OK;

    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    ESP_RETURN_ON_FALSE(ap_netif != NULL, ESP_ERR_NO_MEM, TAG,
                        "Failed to create provisioning AP netif");

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&init_config), TAG,
                        "Failed to initialize Wi-Fi AP");

    uint8_t mac[6];
    ESP_RETURN_ON_ERROR(esp_wifi_get_mac(WIFI_IF_AP, mac), TAG,
                        "Failed to read AP MAC");
    char ssid[32];
    const int ssid_length = snprintf(ssid, sizeof(ssid), "Vazon_%02X%02X",
                                     mac[4], mac[5]);
    ESP_RETURN_ON_FALSE(ssid_length > 0 &&
                            (size_t)ssid_length < sizeof(ssid),
                        ESP_ERR_INVALID_SIZE, TAG,
                        "Failed to create provisioning SSID");

    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.ap.ssid, ssid, (size_t)ssid_length + 1U);
    wifi_config.ap.ssid_len = (uint8_t)ssid_length;
    memcpy(wifi_config.ap.password, PROVISIONING_AP_PASSWORD,
           sizeof(PROVISIONING_AP_PASSWORD));
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), TAG,
                        "Failed to select AP mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &wifi_config), TAG,
                        "Failed to configure provisioning AP");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG,
                        "Failed to start provisioning AP");

    httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();
    ESP_RETURN_ON_ERROR(httpd_start(&http_server, &http_config), TAG,
                        "Failed to start provisioning HTTP server");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(http_server, &ROOT_URI),
                        TAG, "Failed to register provisioning page");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(http_server, &CONFIG_URI),
                        TAG, "Failed to register provisioning form");

    started = true;
    ESP_LOGI(TAG, "Provisioning AP started ssid=%s", ssid);
    ESP_LOGI(TAG, "Open http://192.168.4.1/");
    return ESP_OK;
}
