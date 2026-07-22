# Vazon V3 Firmware Skeleton

Document status: active draft
Code status: runtime core skeleton in progress
Scope: current firmware directory layout, PCB v1.0 smoke-test result, and software port order

## Current Goal

```text
Build Runtime Core and the hardware-independent logic layer on top of the
confirmed board support and sensor services.
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
components/runtime_core
components/command_router
components/climate_module
components/door_module
components/light_module
components/fan_module
components/humidifier_module
components/pot_module
components/system_status
components/status_led
components/mqtt_service
components/mqtt_command_codec
components/mqtt_command_queue
components/connection_config
components/wifi_service
```

## Current Runtime Flow

```text
app_main()
    -> vazon_app_core_init()
        -> initialize Runtime Core and Command Router
        -> read and aggregate SHT31 sensors through Climate Module
        -> start Door Module polling task
        -> initialize and evaluate Light Module
        -> evaluate Fan Module and apply GPIO13 binary fallback
        -> evaluate Humidifier Module from GPIO35 and apply GPIO19/GPIO26
        -> read Pot Modules in an independent ADC/OneWire task
        -> aggregate local module signals into System Status
        -> render System Status and connection state on the two-color Status LED
```

Door Module reads the normalized door input through `gpio_inputs`, evaluates
debounce/state, updates the door-driven maintenance source, and logs state
transitions. MQTT publication is not connected.

Light Module evaluates its default auto mode through `day_window.active`,
applies the request through `gpio_outputs`, and registers its command target.
MQTT transport is not connected.

The temporary bring-up UART command task has been removed. GPIO4, GPIO13,
GPIO19, and GPIO26 are owned by their runtime modules.

Fan Module state-machine code calculates manual/auto demand and PWM
boost/ramp requests. Until the LEDC/PWM driver exists, GPIO13 is connected as a
binary fallback: OFF is applied as 0% and ON is reported honestly as 100%.

Humidifier Module debounces the GPIO35 water input and owns the GPIO26 mist and
GPIO19 local-fan outputs. It implements humidity hysteresis, manual duration,
day-window safety, water/climate interlocks, and 20-second fan post-run. The
selected mist intensity is stored, but the current GPIO26 output is binary:
OFF is 0% and every active level is applied as 100%.

Climate Module reads SHT31 sensors at 0x44 and 0x45 every 5 seconds, validates
CRC and value ranges, tracks the 300-second stale timeout, and supplies both
zone humidity values plus `rh_delta_pct` to Humidifier and Fan Modules. Its
settings command is registered under the `climate` Command Router target.

Two Pot Module instances read averaged ADC moisture values and DS18B20
temperatures every 10 seconds. The OneWire conversion runs in its own task and
does not block Door Module polling. Moisture stays explicitly uncalibrated
until the dry/normal/wet sequence is captured. Both instances register
`pot/0` and `pot/1` Command Router targets for sensor enable settings,
threshold/timeouts, and calibration.

Fan and Humidifier register their complete contract command surfaces under
`fan` and `humidifier`. Light, System, Climate, Pot, Fan, and Humidifier routes
are now present; no transport invokes them yet.

System Status aggregates all active local module signals, door/maintenance,
and connection state. It updates Runtime Core `sensor_missing` and
`actuator_failed` interlocks and logs only top-level status transitions.

Status LED consumes the already aggregated System Status and connection state.
It selects the contract LED pattern and drives the active-low green/red GPIOs;
it does not calculate faults or change module state. The boot-only provisioning
path supplies its dedicated green 500/500 indication.

MQTT Service now contains the isolated secure ESP-MQTT transport based on the
proven V2 connection sequence: certificate bundle verification, QoS 1 command
subscription, reconnect events, and retained availability/LWT. It exposes raw
length-delimited command messages to the boundary callback and does not parse
module commands or control runtime state. Runtime startup waits for Wi-Fi,
configuration/device identity, and the V3 command codec.

MQTT Command Codec validates V3 command topics/envelopes, converts serialized
arguments for System, Climate, Pot, Light, Fan, and Humidifier into their typed
Command Router forms, and creates `ack/reject/fail` responses. It is kept out
of the asynchronous MQTT callback until the runtime command queue serializes
module access.

MQTT Command Queue is the bounded handoff from the asynchronous ESP-MQTT event
task. It never runs owner handlers in the callback. Its consumer executes one
codec/Router call under the supplied runtime lock, releases that lock, and
then publishes the command result without retain.

App Core now creates the shared runtime mutex and the command consumer task.
Door/Climate/Light/Fan/Humidifier evaluation, Pot state updates, and routed
commands use the same mutex. Sensor bus acquisition and MQTT result publishing
remain outside it. The codec/queue are initialized with the stable
`Vazon_{MAC suffix}` device ID; MQTT transport startup still waits for stored
connection configuration and Wi-Fi.

Connection Config loads the proven V2 NVS namespace/keys with version and CRC
validation, while clear operations are restricted to connection keys. Wi-Fi
Service uses the proven delayed connect and bounded exponential retry sequence
on EU channels 1-13. App Core updates `GlobalContext.connection` from service
events and starts secure MQTT only after the station receives an IP address.
Missing/invalid configuration leaves local normal runtime active with Wi-Fi
and MQTT marked disconnected; it does not enter provisioning automatically.

The bring-up entry now checks GPIO0 before any normal-runtime module or sensor
task starts. A confirmed hold clears only Connection Config, keeps actuators in
safe state, and starts the provisioning AP/HTTP service. Missing or invalid
configuration alone still leaves local normal runtime active offline.

## Board Config Boundary

```text
components/board_config/include/vazon_board_config.h
components/board_config/include/vazon_gpio_levels.h
```

`vazon_board_config.h` owns GPIO numbers and board identity.

`vazon_gpio_levels.h` owns normalized logical GPIO levels used by firmware code.

## Current Firmware Step

```text
outbound MQTT state publishing
```

## PCB v1.0 Smoke-Test Result

```text
confirmed passed on physical PCB v1.0
confirmed by project owner: 2026-07-17
```

## PCB v1.0 Smoke-Test Checklist

The confirmed smoke-test scope was:

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
provisioning mode selected at boot only (implemented)
Wi-Fi and MQTT credentials flow (implemented)
two 1700 KiB OTA application slots (configured)
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
