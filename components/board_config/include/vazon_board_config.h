#pragma once

/*
 * Vazon V3 board configuration scaffold.
 *
 * This header is intentionally not wired into runtime firmware yet.
 * It captures the new PCB pinout before module firmware migration.
 */

#include "driver/gpio.h"

/* Board identity */
#define VAZON_BOARD_NAME              "vazon_v3"
#define VAZON_BOARD_REVISION          "PCB v1.0"
#define VAZON_MCU_MODULE              "ESP32-WROVER"
#define VAZON_MODULE_MEMORY           "8M"

/* Boot / service */
#define VAZON_GPIO_PROVISIONING_BTN   GPIO_NUM_0

/* UART programming */
#define VAZON_GPIO_UART0_TXD          GPIO_NUM_1
#define VAZON_GPIO_UART0_RXD          GPIO_NUM_3

/* Status LED */
#define VAZON_GPIO_STATUS_LED_GREEN   GPIO_NUM_5
#define VAZON_GPIO_STATUS_LED_RED     GPIO_NUM_15

/* Actuators */
#define VAZON_GPIO_LIGHT              GPIO_NUM_4
#define VAZON_GPIO_MAIN_FAN           GPIO_NUM_13
#define VAZON_GPIO_HUMIDIFIER_FAN     GPIO_NUM_19
#define VAZON_GPIO_HUMIDIFIER_MIST    GPIO_NUM_26

/* Sensors */
#define VAZON_GPIO_DOOR_REED          GPIO_NUM_33
#define VAZON_GPIO_HUMIDIFIER_WATER   GPIO_NUM_35
#define VAZON_GPIO_POT_TEMP_ONEWIRE   GPIO_NUM_32
#define VAZON_GPIO_POT0_SOIL_MOISTURE GPIO_NUM_36
#define VAZON_GPIO_POT1_SOIL_MOISTURE GPIO_NUM_39

/* Climate I2C */
#define VAZON_GPIO_CLIMATE_I2C_SDA    GPIO_NUM_21
#define VAZON_GPIO_CLIMATE_I2C_SCL    GPIO_NUM_22
#define VAZON_CLIMATE_I2C_PORT        0
#define VAZON_CLIMATE_I2C_FREQ_HZ     100000

/* Polarity / electrical confirmation placeholders */
#define VAZON_STATUS_LED_GREEN_ACTIVE_LOW   1
#define VAZON_STATUS_LED_RED_ACTIVE_LOW     1
#define VAZON_LIGHT_ACTIVE_LOW              1
#define VAZON_MAIN_FAN_ACTIVE_LOW           1
#define VAZON_HUMIDIFIER_FAN_ACTIVE_LOW     1
#define VAZON_HUMIDIFIER_MIST_ACTIVE_LOW    1
#define VAZON_HUMIDIFIER_WATER_PRESENT_LEVEL 1

/* Open hardware confirmations before firmware migration: none. */
