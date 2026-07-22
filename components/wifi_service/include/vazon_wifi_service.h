#pragma once

#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VAZON_WIFI_CONNECTION_DISCONNECTED = 0,
    VAZON_WIFI_CONNECTION_CONNECTING,
    VAZON_WIFI_CONNECTION_CONNECTED,
} vazon_wifi_connection_state_t;

typedef void (*vazon_wifi_connection_callback_t)(
    vazon_wifi_connection_state_t state, void *context);

typedef struct {
    const char *ssid;
    const char *password;
    vazon_wifi_connection_callback_t on_connection;
    void *callback_context;
} vazon_wifi_service_config_t;

typedef struct {
    esp_netif_t *netif;
    esp_timer_handle_t connect_timer;
    esp_timer_handle_t retry_window_timer;
    esp_event_handler_instance_t wifi_event_instance;
    esp_event_handler_instance_t ip_event_instance;
    vazon_wifi_connection_callback_t on_connection;
    void *callback_context;
    int backoff_ms;
    int retries_in_window;
    bool connect_pending;
    bool initialized;
    bool started;
} vazon_wifi_service_t;

esp_err_t vazon_wifi_service_init(
    vazon_wifi_service_t *service,
    const vazon_wifi_service_config_t *config);
esp_err_t vazon_wifi_service_start(vazon_wifi_service_t *service);

#ifdef __cplusplus
}
#endif
