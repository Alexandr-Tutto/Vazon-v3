# Vazon V3 Software Architecture

Document status: active draft
Code status: architecture reference
Scope: Vazon V3 firmware/UI software boundaries

## 1. Purpose

This document defines software ownership boundaries for Vazon V3.

Detailed module behavior is owned by contracts in:

```text
docs/v3_contracts/
```

This document must not duplicate module logic.

## 2. Device Logic Authority

Only firmware runtime layers inside the device may affect device logic.

Allowed logic owners:

```text
Global Core / Global Runtime Policies
owning Local Module
```

Not logic owners:

```text
UI
MQTT
Command Router
topic registry
persistence helper
Output Driver
documentation index
```

## 3. Firmware Layers

```text
Hardware Drivers
    GPIO, ADC, I2C, OneWire, UART, PWM or output drivers

Output Driver
    applies requested physical output state
    does not calculate module state/status

Local Modules
    climate
    pot
    door
    light
    fan
    humidifier

Global Runtime Policies
    global core
    system status
    module/sensor presence
    output confirmation policy

Transport / Service Boundaries
    MQTT boundary
    UART debug service
    OTA service
```

## 4. Command Flow

```text
UI or MQTT input
    -> MQTT/UI boundary
    -> Command Router
    -> owning module or global policy
    -> module reevaluate
    -> output request if needed
    -> output driver
    -> module state/status update
    -> publish/display state/status/settings
```

External interfaces submit commands. They do not mutate module state directly.

## 5. State Categories

```text
state
    physical or logical state owned by a module

settings
    persisted module/global configuration owned by that module/global owner

status
    ok / warning / error / inactive / unknown

status_reason
    machine-readable reason code

output
    requested and reported actuator state
```

State, settings, status, and output must not be mixed into one field.

## 6. UI Boundary

UI may:

```text
display state/status/settings
create user commands
show pending command state
show local presentation state
```

UI must not:

```text
calculate device logic
decide safety
control GPIO
modify module state directly
duplicate module behavior rules
```

Detailed UI presentation rules are in:

```text
docs/ui_architecture.md
```

## 7. MQTT Boundary

MQTT is transport.

MQTT may:

```text
receive messages
validate routing surface
convert payload to command input
publish state/status/settings
publish ack/reject/fail responses
```

MQTT must not:

```text
own module behavior
calculate system.status
control outputs directly
change GlobalContext directly
bypass Command Router
```

Full MQTT topic cleanup is intentionally postponed until active contracts are
clean.

## 8. Output Boundary

Output Driver applies physical output requests.

Output confirmation and retry-to-error semantics are owned by:

```text
docs/v3_contracts/output_confirmation_policy.contract.md
```

Each actuator module owns its own requested output and module-specific retry
parameters.

## 9. Hardware Boundary

The active pinout is:

```text
docs/hardware/v3_board_pinout.md
```

The board config scaffold is:

```text
components/board_config/include/vazon_board_config.h
```

## 10. Migration Boundary

Old-repository code is reference-only.

Use:

```text
docs/migration_rules.md
```

when extracting proven Wi-Fi, MQTT, OTA, ESP-IDF, or UI implementation details.