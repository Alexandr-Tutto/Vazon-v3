#pragma once

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VAZON_GPIO_LEVEL_LOW   0
#define VAZON_GPIO_LEVEL_HIGH  1

#define VAZON_OUTPUT_ACTIVE_HIGH_OFF_LEVEL  VAZON_GPIO_LEVEL_LOW
#define VAZON_OUTPUT_ACTIVE_HIGH_ON_LEVEL   VAZON_GPIO_LEVEL_HIGH

#define VAZON_DOOR_CLOSED_LEVEL            VAZON_GPIO_LEVEL_LOW
#define VAZON_DOOR_OPEN_LEVEL              VAZON_GPIO_LEVEL_HIGH

#define VAZON_WATER_EMPTY_LEVEL            VAZON_GPIO_LEVEL_LOW
#define VAZON_WATER_PRESENT_LEVEL          VAZON_GPIO_LEVEL_HIGH

#ifdef __cplusplus
}
#endif
