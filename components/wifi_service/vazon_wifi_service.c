#include "vazon_wifi_service.h"

#include "esp_log.h"
#include "esp_random.h"
#include "esp_wifi.h"

#include <string.h>

static const char *TAG = "wifi_service";

#define WIFI_START_DELAY_MS       700
#define WIFI_BACKOFF_MIN_MS       500
#define WIFI_BACKOFF_MAX_MS       6000
#define WIFI_RETRY_WINDOW_SEC     20
#define WIFI_RETRY_WINDOW_MAX     6

static int clamp_int(int value, int minimum, int maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static int with_jitter(int delay_ms)
{
    const int unit = delay_ms / 10;
    const int percent = (int)(esp_random() % 21U) - 10;
    return clamp_int(delay_ms + percent * unit / 10,
                     WIFI_BACKOFF_MIN_MS, WIFI_BACKOFF_MAX_MS);
}

static void notify_connection(vazon_wifi_service_t *service,
                              vazon_wifi_connection_state_t state)
{
    if (service->on_connection != NULL) {
        service->on_connection(state, service->callback_context);
    }
}

static void connect_timer_callback(void *context)
{
    vazon_wifi_service_t *service = (vazon_wifi_service_t *)context;
    if (service == NULL || !service->started) return;
    service->connect_pending = false;
    notify_connection(service, VAZON_WIFI_CONNECTION_CONNECTING);
    const esp_err_t result = esp_wifi_connect();
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "esp_wifi_connect failed: %s", esp_err_to_name(result));
        notify_connection(service, VAZON_WIFI_CONNECTION_DISCONNECTED);
    }
}

static void retry_window_timer_callback(void *context)
{
    vazon_wifi_service_t *service = (vazon_wifi_service_t *)context;
    if (service != NULL) service->retries_in_window = 0;
}

static void schedule_connect(vazon_wifi_service_t *service, int delay_ms)
{
    if (service->connect_pending) return;
    const esp_err_t result = esp_timer_start_once(
        service->connect_timer, (uint64_t)delay_ms * 1000ULL);
    if (result == ESP_OK) {
        service->connect_pending = true;
        ESP_LOGI(TAG, "Wi-Fi connect scheduled in %d ms", delay_ms);
    } else {
        ESP_LOGE(TAG, "Failed to schedule Wi-Fi connect: %s",
                 esp_err_to_name(result));
        notify_connection(service, VAZON_WIFI_CONNECTION_DISCONNECTED);
    }
}

static void wifi_event_handler(void *handler_args, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    vazon_wifi_service_t *service = (vazon_wifi_service_t *)handler_args;
    if (service == NULL) return;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        schedule_connect(service, WIFI_START_DELAY_MS);
        const esp_err_t result = esp_timer_start_periodic(
            service->retry_window_timer,
            (uint64_t)WIFI_RETRY_WINDOW_SEC * 1000000ULL);
        if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "Failed to start retry window: %s",
                     esp_err_to_name(result));
        }
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        const wifi_event_sta_disconnected_t *event =
            (const wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGW(TAG, "Wi-Fi disconnected reason=%d",
                 event != NULL ? event->reason : -1);
        notify_connection(service, VAZON_WIFI_CONNECTION_DISCONNECTED);

        ++service->retries_in_window;
        if (service->retries_in_window > WIFI_RETRY_WINDOW_MAX) {
            service->retries_in_window = 0;
            service->backoff_ms = WIFI_BACKOFF_MAX_MS;
        } else {
            service->backoff_ms = service->backoff_ms == 0
                                      ? WIFI_BACKOFF_MIN_MS
                                      : clamp_int(service->backoff_ms * 2,
                                                  WIFI_BACKOFF_MIN_MS,
                                                  WIFI_BACKOFF_MAX_MS);
        }
        schedule_connect(service, with_jitter(service->backoff_ms));
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event = (const ip_event_got_ip_t *)event_data;
        if (event != NULL) {
            ESP_LOGI(TAG, "Wi-Fi connected, IPv4=" IPSTR,
                     IP2STR(&event->ip_info.ip));
        } else {
            ESP_LOGI(TAG, "Wi-Fi connected");
        }
        service->backoff_ms = 0;
        service->retries_in_window = 0;
        notify_connection(service, VAZON_WIFI_CONNECTION_CONNECTED);
    }
}

esp_err_t vazon_wifi_service_init(
    vazon_wifi_service_t *service,
    const vazon_wifi_service_config_t *config)
{
    if (service == NULL || config == NULL || config->ssid == NULL ||
        config->password == NULL || config->ssid[0] == '\0' ||
        config->password[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    if (strnlen(config->ssid, 32U) >= 32U ||
        strnlen(config->password, 64U) >= 64U) {
        return ESP_ERR_INVALID_SIZE;
    }

    memset(service, 0, sizeof(*service));
    service->on_connection = config->on_connection;
    service->callback_context = config->callback_context;

    service->netif = esp_netif_create_default_wifi_sta();
    if (service->netif == NULL) return ESP_ERR_NO_MEM;

    const esp_timer_create_args_t connect_timer_config = {
        .callback = connect_timer_callback,
        .arg = service,
        .name = "wifi_connect",
        .skip_unhandled_events = true,
    };
    esp_err_t result = esp_timer_create(&connect_timer_config,
                                        &service->connect_timer);
    if (result != ESP_OK) return result;
    const esp_timer_create_args_t window_timer_config = {
        .callback = retry_window_timer_callback,
        .arg = service,
        .name = "wifi_window",
        .skip_unhandled_events = true,
    };
    result = esp_timer_create(&window_timer_config,
                              &service->retry_window_timer);
    if (result != ESP_OK) return result;

    result = esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, service,
        &service->wifi_event_instance);
    if (result != ESP_OK) return result;
    result = esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, service,
        &service->ip_event_instance);
    if (result != ESP_OK) return result;

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    result = esp_wifi_init(&init_config);
    if (result != ESP_OK) return result;
    const wifi_country_t country = {
        .cc = "EU",
        .schan = 1,
        .nchan = 13,
        .policy = WIFI_COUNTRY_POLICY_MANUAL,
    };
    result = esp_wifi_set_country(&country);
    if (result != ESP_OK) return result;

    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.sta.ssid, config->ssid, strlen(config->ssid) + 1U);
    memcpy(wifi_config.sta.password, config->password,
           strlen(config->password) + 1U);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    wifi_config.sta.bssid_set = false;
    wifi_config.sta.listen_interval = 3U;

    result = esp_wifi_set_mode(WIFI_MODE_STA);
    if (result == ESP_OK) {
        result = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    }
    if (result != ESP_OK) return result;

    service->initialized = true;
    return ESP_OK;
}

esp_err_t vazon_wifi_service_start(vazon_wifi_service_t *service)
{
    if (service == NULL || !service->initialized) return ESP_ERR_INVALID_ARG;
    if (service->started) return ESP_ERR_INVALID_STATE;
    service->started = true;
    notify_connection(service, VAZON_WIFI_CONNECTION_CONNECTING);
    const esp_err_t result = esp_wifi_start();
    if (result != ESP_OK) {
        service->started = false;
        notify_connection(service, VAZON_WIFI_CONNECTION_DISCONNECTED);
    }
    return result;
}
