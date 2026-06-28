# Fan Module Contract

Document status: draft
Code status: architecture contract
Scope: Vazon V3 Main Fan Module

## 1. Purpose

```text
Fan Module controls the main cabinet ventilation fan.
```

## 2. Role

```text
Fan Module is a Local Climate Module.
```

## 3. Ownership

```text
Fan Module owns only:

fan.state
fan.settings
fan.output
fan.status
fan.status_reason
```

## 4. Inputs

```text
GlobalContext
climate humidity values
door.state
fan settings
fan commands
output confirmation if available
```

## 5. Outputs

```text
fan output
fan.status
fan.status_reason
```

## 6. Settings

```text
mode = auto / manual
runtime = day / always

auto_strategy = delta / timer

manual_duration_sec

auto_delta_on_pct
auto_delta_off_pct

auto_timer_on_sec
auto_timer_off_sec
```

## 7. State

```text
fan.output = on / off / unknown
fan.auto_state = off / running / pause / blocked / alert / unknown
fan.status = ok / warning / error / inactive / unknown
fan.status_reason
last_command_result
last_output_confirmed
```

## 8. Dependencies

```text
maintenance: used
day_window: used
connection: ignored
global_interlocks: used

door.state: used
climate humidity values: used by delta strategy
```

## 9. Logic

```text
if maintenance.active:
    fan off

if door.state = open:
    fan off

if mode = manual:
    accepted manual command runs fan for manual_duration_sec

if mode = auto:
    if runtime = day and day_window.active = false:
        fan off

    if auto_strategy = delta:
        if abs(zone0_rh - zone1_rh) >= auto_delta_on_pct:
            fan on
        if abs(zone0_rh - zone1_rh) <= auto_delta_off_pct:
            fan off

    if auto_strategy = timer:
        run fan for auto_timer_on_sec
        pause fan for auto_timer_off_sec
        repeat while auto mode remains allowed
```

## 10. Rules

```text
auto_strategy default = delta

manual_duration_sec default = 600
manual_duration_sec range = 10..86400

auto_delta_on_pct default = 10
auto_delta_off_pct default = 5
auto_delta_on_pct - auto_delta_off_pct >= 5

auto_timer_on_sec default = 60
auto_timer_off_sec default = 300
auto_timer_on_sec range = 1..86400
auto_timer_off_sec range = 1..86400
```

## 11. Status

```text
ok       - normal operation
warning  - temporary condition or confirmation unavailable
error    - command failed after retries
inactive - module disabled
unknown  - state is not determined after startup
```

## 12. MQTT Surface

```text
MQTT namespace: fan

Published by module:
fan.state
fan.output
fan.auto_state
fan.status
fan.status_reason
fan.settings

Accepted commands:
fan.set_mode
fan.set_runtime
fan.set_auto_strategy
fan.manual_run
fan.stop
fan.set_settings

Ack/reject/fail semantics are handled by MQTT Boundary and Command Router.
Topic strings are not owned here.
```

## 13. Output Confirmation

```text
Fan Module follows Output Confirmation Policy.
Module-specific retry parameters:
retry_count = 3
retry_delay_sec = 1
```

## 14. Forbidden

```text
Fan Module does not modify GlobalContext.
Fan Module does not calculate system.status.
Fan Module does not use temperature delta auto logic.
Fan Module does not create hidden manual-check mode.
Fan Module does not raise error after the first transient confirmation miss.
Fan Module does not define MQTT topic strings.
```
