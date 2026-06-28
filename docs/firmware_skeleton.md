# Vazon V3 Firmware Skeleton

Document status: draft
Code status: skeleton reference
Scope: current firmware directory layout

## Current Goal

```text
Create a minimal ESP-IDF project skeleton before porting old implementation code.
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

Expected smoke-test scope:

```text
UART boot log
safe OFF initialization for active-low outputs
status LED manual pattern test
raw door GPIO read
raw humidifier water GPIO read
I2C scan for SHT31
OneWire scan for DS18B20
ADC raw read for pot0 and pot1
manual output toggle test through temporary serial commands
```

## Forbidden In Skeleton Phase

```text
Do not port old Vazon module logic directly.
Do not implement MQTT before data model and smoke-test are stable.
Do not implement UI-specific assumptions in firmware.
Do not duplicate contract behavior inside this document.
```
