#pragma once

#include "esp_err.h"
#include "vazon_command_router.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VAZON_COMMAND_TARGET_OTA "ota"
#define VAZON_COMMAND_OTA_CHECK "check"
#define VAZON_COMMAND_OTA_INSTALL "install"

typedef void (*vazon_ota_status_callback_t)(const char *state,
                                            const char *version,
                                            esp_err_t error,
                                            void *context);

typedef struct {
    vazon_ota_status_callback_t status_callback;
    void *callback_context;
    bool initialized;
} vazon_ota_service_t;

esp_err_t vazon_ota_service_init(
    vazon_ota_service_t *service,
    vazon_ota_status_callback_t status_callback,
    void *callback_context);
esp_err_t vazon_ota_service_register_command_target(
    vazon_ota_service_t *service,
    vazon_command_router_t *router);

#ifdef __cplusplus
}
#endif
