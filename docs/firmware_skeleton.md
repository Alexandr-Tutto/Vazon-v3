# Vazon V3 Firmware Skeleton

Document status: active draft
Code status: board bring-up skeleton
Scope: current firmware directory layout, PCB v1.0 smoke-test scope, and software port order

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
components/gpio_outputs
components/gpio_inputs
components/i2c_service
components/onewire_service
components/adc_service
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
safe LOW/OFF initialization for active-high outputs
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
LIGHT_GPIO            OFF = 0
MAIN_FAN_GPIO         OFF = 0
HUMIDIFIER_FAN_GPIO   OFF = 0
HUMIDIFIER_MIST_GPIO  OFF = 0
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

## Software Port Plan

The V3 firmware port order is:

```text
1. Board bring-up firmware
2. Board support layer
3. Sensor drivers
4. Runtime Core
5. Local Modules
6. System Status
7. MQTT boundary
8. UI integration
9. OTA / provisioning
```

## 1. Board Bring-Up Firmware

Implement first:

```text
UART boot log
safe OFF GPIO initialization
status LED pattern test
raw door GPIO read
raw humidifier water GPIO read
I2C scan
OneWire scan
ADC raw reads
temporary serial/debug output toggle commands
```

The result of this step is physical PCB confirmation. It must not contain
production module logic.

## 2. Board Support Layer

After smoke testing, isolate low-level hardware access:

```text
board_config:
    pinout, GPIO levels, board identity only

gpio_outputs:
    safe init and output set helpers

gpio_inputs:
    door, water, provisioning button reads

adc_service:
    raw ADC reads

i2c_service:
    bus init and scan/read helpers

onewire_service:
    scan/read helpers
```

## 3. Sensor Drivers

Bring sensors up before module behavior:

```text
SHT31:
    two sensors, address detection, missing/error state

DS18B20:
    enumeration on POT_TEMP_ONEWIRE_GPIO

soil moisture ADC:
    raw reads first; normalization later

door reed:
    raw level then debounced state

humidifier water reed:
    raw level then water present/empty state
```

## 4. Runtime Core

Implement V3 runtime skeleton after hardware reads are stable:

```text
boot mode decision
normal runtime vs provisioning mode
GlobalContext
maintenance
day_window
connection
global_interlocks
command router skeleton
```

`day_window` belongs to Global Core and is updated through
`system.set_day_window`.

## 5. Local Modules

Port modules one at a time:

```text
door:
    debounce, open/closed state, status

light:
    manual mode, auto mode through day_window.active, maintenance lighting

fan:
    manual mode, auto mode, maintenance off behavior

humidifier:
    water state, mist output, local fan output, maintenance off behavior

climate:
    two SHT31 sensors, aggregation, temperature delta warning/error

pot:
    soil moisture, soil temperature, sensor presence/absence handling
```

## 6. System Status

Add system-level status only after local modules report stable state:

```text
system.status
system.status_reason
primary_fault
expected missing sensor policy
actuator failed policy
global interlocks
```

System status aggregation must not be owned by UI, MQTT, or local modules.

## 7. MQTT Boundary

Add MQTT after Runtime Core and module state are stable:

```text
topic namespace
retained state
command envelope
command result
state/settings/status publishing
```

MQTT must route commands and publish state. It must not own module behavior.

## 8. UI Integration

After MQTT is available:

```text
replace mock state with live state
verify system.set_day_window
verify manual/auto module commands
verify maintenance behavior
verify warning/error/status display
```

The V3.0 starting UI design baseline is already fixed in the current main
branch.

## 9. OTA / Provisioning

Add after the basic normal runtime is stable:

```text
provisioning mode selected at boot only
Wi-Fi credentials flow
OTA update path
version/manifest handling
rollback/failure behavior
```

## Port Stop Conditions

Do not port old code when it:

```text
uses old names
mixes UI/MQTT/module logic
bypasses board_config
controls GPIO directly from module logic without the output boundary
conflicts with V3 contracts
```

## Forbidden In Skeleton Phase

```text
Do not port old Vazon module logic directly.
Do not implement MQTT before data model and smoke-test are stable.
Do not implement UI-specific assumptions in firmware.
Do not duplicate contract behavior inside this document.
```
