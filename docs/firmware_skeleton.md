# Vazon V3 Firmware Skeleton

Document status: deprecated historical
Code status: superseded by the integrated runtime
Scope: historical PCB v1.0 bring-up result and software port plan

This file preserves the completed bring-up sequence. It is not a current
implementation-status or module-logic owner. Current status is recorded in
`README.md` and `docs/project_overview.md`; behavior remains owned by the active
contracts and service boundaries in `docs/software_architecture.md`.

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
components/settings_store
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
boost/ramp requests. Until the future four-wire PWM GPIO and driver are
assigned, GPIO13 remains a binary fallback: OFF is applied as 0% and ON is
reported honestly as 100%.

Humidifier Module debounces the GPIO35 water input and owns the GPIO26 mist and
GPIO19 local-fan outputs. It implements humidity hysteresis, manual duration,
day-window safety, water/climate interlocks, and 20-second fan post-run. The
GPIO26 driver applies 800 Hz PWM through the external RC differentiator:
low/medium/high intensity is 50/60/70% duty and OFF is 0%.

Climate Module reads SHT31 sensors at 0x44 and 0x45 every 5 seconds, validates
CRC and value ranges, tracks the 300-second stale timeout, and supplies both
zone humidity values plus `rh_delta_pct` to Humidifier and Fan Modules. Its
settings command is registered under the `climate` Command Router target.

Two Pot Module instances read averaged ADC moisture values and DS18B20
temperatures every 2 seconds while an enabled moisture calibration is incomplete
and every 30 seconds in normal runtime. During the normal wait they detect a
newly started calibration within 2 seconds. The OneWire conversion runs in its own task and
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
remain outside it. The codec/queue are initialized with the stable device ID
defined by `docs/mqtt_topic_registry.md`; MQTT transport startup still waits for
connection configuration and Wi-Fi.

Connection Config loads the proven V2 NVS namespace/keys with version and CRC
validation, while clear operations are restricted to connection keys. Wi-Fi
Service uses the proven delayed connect and bounded exponential retry sequence
on EU channels 1-13. App Core updates `GlobalContext.connection` from service
events and starts secure MQTT only after the station receives an IP address.
Missing/invalid configuration leaves local normal runtime active with Wi-Fi
and MQTT marked disconnected; it does not enter provisioning automatically.

Settings Store keeps the controller-owned runtime configuration in a separate
versioned NVS blob with CRC validation. It persists Global Core day-window
settings, Climate/Light/Fan/Humidifier/Pot settings, and both Pot moisture
calibrations, including an incomplete calibration sequence. App Core validates
stored values through their owning modules and restores defaults only for an
invalid block. A low-priority watcher writes only after an actual settings
change. Maintenance requests, sensor/output state, active manual runs,
automatic demand, and runtime timers are transient and restart safely. Device
verification across reboot and OTA passed on PCB v1.0 unit 02.

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
    water present = 0
    water empty = 1
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

Current OTA boundary:

```text
provisioning mode selected at boot only (implemented)
Wi-Fi and MQTT credentials flow (implemented)
two 1700 KiB OTA application slots (configured)
OTA service HTTPS manifest check/install path (verified on device from PWA through restart and ready status)
rollback/boot confirmation behavior (not implemented)
```

OTA Service owns target `ota` with commands `check` and `install`. Both use an
empty args object. The MQTT boundary decodes them and routes them through
Command Router; it does not call ESP-IDF OTA directly.

The build configuration supplies one full HTTPS manifest URL. An empty URL
keeps OTA commands unavailable. The manifest contains exactly `version` and
`url`; the firmware URL must also use HTTPS. `check` reports whether the
manifest contains a strictly newer numeric `major.minor.patch` version than the
running application. Equal or older versions are not offered, preventing an
accidental downgrade from a stale manifest. `install` performs the same check
and, when newer, calls the ESP-IDF HTTPS OTA path from one worker task before
rebooting. Direct URL commands and automatic checks are not part of this stage.

During the OTA worker operation, the Task Watchdog timeout is temporarily
extended from the configured 5 seconds to 15 seconds for ESP32 TLS/ECDH work.
The watchdog remains enabled and its normal timeout is restored when an OTA
check or failed install finishes.
TLS watchdog handling and stale-manifest downgrade blocking are verified on
PCB v1.0 unit 02.

OTA status is asynchronous because an accepted command only means that the
worker started. OTA Service reports `checking`, `up_to_date`,
`update_available`, `downloading`, `rebooting`, or `failed` through the
service-owned status callback. After every successful MQTT connection, OTA
Service reports `ready` with the running application version. App Core passes
that status to the registered MQTT service topic without moving OTA behavior
into MQTT.

Connection Config and Settings Store use NVS data separate from the OTA
application slots, so a normal OTA install does not erase Wi-Fi/MQTT credentials
or user runtime settings.

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
