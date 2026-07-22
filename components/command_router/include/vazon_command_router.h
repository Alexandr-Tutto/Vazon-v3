#pragma once

#include "esp_err.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VAZON_COMMAND_ROUTER_MAX_TARGETS 12

typedef enum {
    VAZON_COMMAND_RESULT_UNKNOWN = 0,
    VAZON_COMMAND_RESULT_ACCEPTED,
    VAZON_COMMAND_RESULT_REJECTED,
    VAZON_COMMAND_RESULT_FAILED,
} vazon_command_result_status_t;

typedef enum {
    VAZON_COMMAND_SOURCE_UNKNOWN = 0,
    VAZON_COMMAND_SOURCE_UI,
    VAZON_COMMAND_SOURCE_SERVICE,
    VAZON_COMMAND_SOURCE_TEST,
} vazon_command_source_t;

typedef struct {
    const char *cmd_id;
    const char *target;
    const char *cmd;
    const void *args;
    vazon_command_source_t source;
    int64_t ts;
} vazon_command_t;

typedef struct {
    vazon_command_result_status_t status;
    esp_err_t error;
} vazon_command_result_t;

typedef vazon_command_result_t (*vazon_command_handler_t)(const vazon_command_t *command,
                                                          void *context);

typedef struct {
    const char *target;
    vazon_command_handler_t handler;
    void *context;
} vazon_command_route_t;

typedef struct {
    vazon_command_route_t routes[VAZON_COMMAND_ROUTER_MAX_TARGETS];
    size_t route_count;
} vazon_command_router_t;

void vazon_command_router_init(vazon_command_router_t *router);
esp_err_t vazon_command_router_register(vazon_command_router_t *router,
                                        const char *target,
                                        vazon_command_handler_t handler,
                                        void *context);
vazon_command_result_t vazon_command_router_route(const vazon_command_router_t *router,
                                                  const vazon_command_t *command);

#ifdef __cplusplus
}
#endif
