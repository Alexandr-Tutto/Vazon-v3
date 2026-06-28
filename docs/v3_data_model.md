# Vazon V3 Data Model

Document status: draft
Code status: architecture reference
Scope: Vazon V3 state/data shape only

## 1. Purpose

```text
This document defines the shared V3 data model for firmware structs, MQTT payloads, and UI state.
```

This file describes data shape only.

It does not own module behavior, GPIO binding, UI wording, MQTT topic strings, persistence rules, or safety logic.

## 2. Ownership References

```text
module behavior -> docs/v3_contracts/*.contract.md
software boundaries -> docs/software_architecture.md
UI presentation -> docs/ui_architecture.md
pinout -> docs/hardware/v3_board_pinout.md
board config -> components/board_config/include/vazon_board_config.h
MQTT exact topic strings -> postponed MQTT registry
```

## 3. Common Types

```text
status:
    ok
    warning
    error
    inactive
    unknown

status_reason:
    string enum owned by the corresponding module or global policy

command_result:
    accepted
    rejected
    failed
    unknown

output_state:
    on
    off
    unknown

mode:
    auto
    manual
```

## 4. Top-Level Entities

```text
system
climate
pot[0]
pot[1]
door
light
fan
humidifier
status_led
provisioning_button
uart_debug
```

## 5. system

```text
system.status: status without inactive
system.status_reason: string enum / null
system.primary_fault: string enum / null
system.affected_system:
    climate
    pot
    light
    fan
    humidifier
    door
    connection
    system
    null
```

```text
system.global_context.maintenance.active: bool
system.global_context.maintenance.reason:
    manual
    external
    unknown
    null

system.global_context.day_window.active: bool / unknown
system.global_context.day_window.schedule_enabled: bool
system.global_context.day_window.time_on: HH:MM
system.global_context.day_window.time_off: HH:MM

system.global_context.connection.wifi_state:
    connected
    weak
    disconnected
    unknown

system.global_context.connection.mqtt_state:
    connected
    reconnecting
    disconnected
    unknown
```

## 6. climate

```text
climate.sensor_0x44.temperature_c: float / unknown
climate.sensor_0x44.humidity_pct: float / unknown
climate.sensor_0x44.status: status without inactive
climate.sensor_0x44.status_reason: string enum / null

climate.sensor_0x45.temperature_c: float / unknown
climate.sensor_0x45.humidity_pct: float / unknown
climate.sensor_0x45.status: status without inactive
climate.sensor_0x45.status_reason: string enum / null

climate.rh_delta_pct: float / unknown
climate.temperature_delta_c: float / unknown
climate.status: status
climate.status_reason: string enum / null
climate.settings: object
```

UI placement labels such as `top` and `bottom` are presentation/installation metadata and do not replace SHT31 address identity.

## 7. pot[n]

There are two configured pot instances:

```text
pot[0]
pot[1]
```

Each pot has one soil moisture sensor and one soil temperature sensor.

```text
pot[n].soil_moisture.raw_adc_value: integer / unknown
pot[n].soil_moisture.raw_mv: integer / unknown
pot[n].soil_moisture.value_pct: float 0..100 / unknown
pot[n].soil_moisture.class:
    dry
    normal
    wet
    overwet
    unknown
pot[n].soil_moisture.status: status
pot[n].soil_moisture.status_reason: string enum / null

pot[n].soil_temperature.temperature_c: float / unknown
pot[n].soil_temperature.status: status
pot[n].soil_temperature.status_reason: string enum / null

pot[n].status: status
pot[n].status_reason: string enum / null
pot[n].settings: object
```

Sensor enable settings are part of `pot[n].settings`:

```text
pot[n].settings.soil_moisture_enabled: bool
pot[n].settings.soil_temperature_enabled: bool
```

## 8. door

```text
door.state:
    closed
    open
    unknown

door.status: status
door.status_reason: string enum / null
door.last_change_time: timestamp / null
```

GPIO semantic mapping is owned by `door.contract.md` and the board pinout.

## 9. light

```text
light.output: output_state
light.status: status
light.status_reason: string enum / null
light.last_command_result: command_result
light.last_output_confirmed: bool / unknown
light.settings.mode: mode
light.settings.manual_state:
    on
    off
```

Maintenance service-light behavior is owned by `light.contract.md`.

## 10. fan

```text
fan.output: output_state
fan.auto_state:
    off
    running
    pause
    blocked
    alert
    unknown
fan.status: status
fan.status_reason: string enum / null
fan.last_command_result: command_result
fan.last_output_confirmed: bool / unknown
fan.settings: object
```

Minimum expected settings shape:

```text
fan.settings.mode: mode
fan.settings.runtime:
    day
    always
fan.settings.auto_strategy:
    delta
    timer
fan.settings.manual_duration_sec: integer
fan.settings.auto_delta_on_pct: integer
fan.settings.auto_delta_off_pct: integer
fan.settings.auto_timer_on_sec: integer
fan.settings.auto_timer_off_sec: integer
```

## 11. humidifier

```text
humidifier.water_status:
    present
    empty
    unknown

humidifier.mist_output:
    on
    off
    unknown

humidifier.fan_output:
    off
    on
    post_run
    unknown

humidifier.status: status
humidifier.status_reason: string enum / null
humidifier.water_sensor_debounce_state: object / null
humidifier.last_mist_command_result: command_result
humidifier.last_fan_command_result: command_result
humidifier.last_mist_output_confirmed: bool / unknown
humidifier.last_fan_output_confirmed: bool / unknown
humidifier.settings: object
```

Minimum expected settings shape:

```text
humidifier.settings.mode: mode
humidifier.settings.runtime:
    day
    always
humidifier.settings.rh_start: integer
humidifier.settings.rh_stop: integer
humidifier.settings.rh_delta: integer
humidifier.settings.mist_power_level:
    low
    medium
    high
humidifier.settings.manual_mist_duration_sec: integer
humidifier.settings.post_fan_sec: integer
```

## 12. status_led

```text
status_led.red_output: output_state
status_led.green_output: output_state
status_led.pattern:
    normal_ok
    provisioning_active
    wifi_offline
    mqtt_offline
    system_error
    local_warning
    unknown
```

LED pattern selection is owned by `status_led.contract.md`.

## 13. provisioning_button

```text
provisioning_button.state:
    pressed
    released
    unknown

provisioning_button.boot_sample_result:
    normal
    provisioning
    unknown
```

Provisioning behavior is boot-time only and is owned by `provisioning_button.contract.md` and `global_core.contract.md`.

## 14. uart_debug

```text
uart_debug.enabled: bool
uart_debug.log_level:
    off
    error
    warning
    info
    debug
    trace
```

UART0 ownership and restrictions are defined by `uart_debug.contract.md` and board pinout.

## 15. Explicit Non-Model Items

The following are not defined in this file:

```text
exact MQTT topic strings
JSON serialization format
firmware C struct names
UI wording
GPIO numbers
GPIO active levels
module behavior rules
persistence storage layout
```

They must be defined by their owner documents only.
