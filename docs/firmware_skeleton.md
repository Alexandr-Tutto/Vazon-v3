# Vazon V3 Firmware Skeleton

Document status: active draft
Code status: board bring-up skeleton
Scope: current firmware directory layout and PCB v1.0 smoke-test scope

## Current Goal

```text
Keep a minimal ESP-IDF project skeleton ready for PCB v1.0 board bring-up.

Tomorrow's embedded focus is hardware smoke testing, not production module
implementation.
```

## Current Build Entry

```text
CMakeLists.txt
main/CMakeLists.txt
main/app_main.c
```

## Active Components

```text
components/board_config
components/app_core
```

## Current Runtime Flow

```text
app_main()
    -> vazon_app_core_init()
```

No module behavior is implemented yet.

## Board Config Boundary

```text
components/board_config/include/vazon_board_config.h
components/board_config/include/vazon_gpio_levels.h
```

`vazon_board_config.h` owns GPIO numbers and board identity.

`vazon_gpio_levels.h` owns normalized logical GPIO levels used by firmware code.

## Next Firmware Step

```text
hardware smoke test
```

## PCB v1.0 Smoke-Test Checklist

Run this scope before porting production module behavior:

```text
UART boot log
safe OFF initialization for active-low outputs
status LED manual pattern test
raw door GPIO read
raw humidifier water GPIO read
I2C scan for SHT31
OneWire scan for DS18B20
ADC raw read for pot[0] and pot[1]
manual output toggle test through temporary serial commands
```

Expected initial safe states:

```text
LIGHT_GPIO            OFF = 1
MAIN_FAN_GPIO         OFF = 1
HUMIDIFIER_FAN_GPIO   OFF = 1
HUMIDIFIER_MIST_GPIO  OFF = 1
```

Expected input checks:

```text
DOOR_REED_GPIO:
    closed = 0
    open = 1

HUMIDIFIER_WATER_PRESENT_GPIO:
    water empty = 0
    water present = 1
```

Expected bus checks:

```text
I2C:
    detect two SHT31 sensors or record the observed addresses

OneWire:
    detect DS18B20 sensors on POT_TEMP_ONEWIRE_GPIO

ADC:
    read raw values from pot[0] and pot[1] channels
```

Temporary serial commands are allowed only for bring-up output toggles and raw
sensor reads. They must not define production command names or UI/MQTT behavior.

## Forbidden In Skeleton Phase

```text
Do not port old Vazon module logic directly.
Do not implement MQTT before data model and smoke-test are stable.
Do not implement UI-specific assumptions in firmware.
Do not duplicate contract behavior inside this document.
```
