#pragma once

#include "esp_err.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned int check_window_ms;
    unsigned int hold_ms;
    unsigned int poll_interval_ms;
} vazon_provisioning_button_config_t;

esp_err_t vazon_provisioning_button_check(
    const vazon_provisioning_button_config_t *config,
    bool *provisioning_requested);

#ifdef __cplusplus
}
#endif
