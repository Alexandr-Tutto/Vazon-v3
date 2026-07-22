#include "vazon_command_router.h"

#include <string.h>

static vazon_command_result_t result(vazon_command_result_status_t status, esp_err_t error)
{
    const vazon_command_result_t command_result = {
        .status = status,
        .error = error,
    };
    return command_result;
}

void vazon_command_router_init(vazon_command_router_t *router)
{
    if (router != NULL) {
        memset(router, 0, sizeof(*router));
    }
}

esp_err_t vazon_command_router_register(vazon_command_router_t *router,
                                        const char *target,
                                        vazon_command_handler_t handler,
                                        void *context)
{
    if (router == NULL || target == NULL || target[0] == '\0' || handler == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < router->route_count; ++i) {
        if (strcmp(router->routes[i].target, target) == 0) {
            return ESP_ERR_INVALID_STATE;
        }
    }
    if (router->route_count >= VAZON_COMMAND_ROUTER_MAX_TARGETS) {
        return ESP_ERR_NO_MEM;
    }

    vazon_command_route_t *route = &router->routes[router->route_count++];
    route->target = target;
    route->handler = handler;
    route->context = context;
    return ESP_OK;
}

vazon_command_result_t vazon_command_router_route(const vazon_command_router_t *router,
                                                  const vazon_command_t *command)
{
    if (router == NULL || command == NULL || command->target == NULL ||
        command->target[0] == '\0' || command->cmd == NULL || command->cmd[0] == '\0') {
        return result(VAZON_COMMAND_RESULT_REJECTED, ESP_ERR_INVALID_ARG);
    }

    for (size_t i = 0; i < router->route_count; ++i) {
        const vazon_command_route_t *route = &router->routes[i];
        if (strcmp(route->target, command->target) == 0) {
            return route->handler(command, route->context);
        }
    }

    return result(VAZON_COMMAND_RESULT_REJECTED, ESP_ERR_NOT_FOUND);
}
