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

power_level_pct = 20 / 40 / 60 / 80 / 100
```

Mode combinations used by UI:

```text
mode = auto + runtime = day
mode = auto + runtime = always
auto_strategy = delta
auto_strategy = timer
```

UI status panel may provide quick commands for `mode`, `runtime`, and
`auto_strategy`. Numeric parameter editing is in extended settings.

## 7. State

```text
fan.output = on / off / unknown
fan.applied_power_pct = 0..100 / unknown
fan.auto_state = off / running / pause / blocked / alert / unknown
fan.status = ok / warning / error / unknown
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
climate.rh_delta_pct: used by delta strategy
```

## 9. Logic

Evaluation order:

```text
if maintenance.active:
    fan off immediately
    fan.auto_state = blocked
    fan.status = ok
    fan.status_reason = null
    skip manual and auto run logic

if door.state = open:
    fan off immediately
    fan.auto_state = blocked
    fan.status = warning
    fan.status_reason = door_open
    skip manual and auto run logic

if door.state = unknown:
    fan off immediately
    fan.auto_state = blocked
    fan.status = warning
    fan.status_reason = door_unknown
    skip manual and auto run logic

if mode = manual:
    automatic delta and timer strategies do not run
    accepted fan.manual_run command runs fan for manual_duration_sec
    while manual run is active, fan.auto_state = running
    after manual_duration_sec expires, fan off
    accepted fan.stop command stops the fan immediately
    after fan.stop, fan stays off without a time limit
    while manual output is off, fan.auto_state = off
    fan stays off until fan.manual_run is accepted or mode returns to auto

if mode = auto:
    if runtime = day and day_window.active = false:
        fan off
        fan.auto_state = off
        fan.status = ok
        fan.status_reason = null
        skip selected auto strategy

    if auto_strategy = delta:
        if climate.rh_delta_pct is unknown, stale, or cannot be calculated:
            fan off
            fan.auto_state = alert
            fan.status = warning
            fan.status_reason = climate_invalid
            skip remaining delta logic
        if climate.rh_delta_pct >= auto_delta_on_pct:
            automatic demand = on
        if climate.rh_delta_pct <= auto_delta_off_pct:
            automatic demand = off
        if auto_delta_off_pct < climate.rh_delta_pct < auto_delta_on_pct:
            retain previous automatic demand
        apply automatic demand to fan output
        if automatic demand = on, fan.auto_state = running
        if automatic demand = off, fan.auto_state = off

    if auto_strategy = timer:
        run fan for auto_timer_on_sec
        while running, fan.auto_state = running
        pause fan for auto_timer_off_sec
        while paused, fan.auto_state = pause
        repeat while auto mode remains allowed
```

Runtime transition rules:

```text
initial delta automatic demand after boot = off

if door opens, door becomes unknown, or maintenance becomes active:
    fan off immediately
    reset timer auto cycle

when the door/maintenance block clears in timer auto mode:
    start a new timer cycle with the running phase

manual mode by itself:
    fan.status = ok
    fan.status_reason = null

when a temporary blocking/warning condition clears:
    clear its fan.status_reason
    reevaluate fan output from current mode and inputs

output command/confirmation failure:
    follow Output Confirmation Policy
    may override normal or warning status with error after retries
```

PWM power behavior:

```text
power_level_pct applies in both manual and auto mode

on every fan transition from off to on:
    apply 100% PWM for 1 second
    linearly ramp from 100% to power_level_pct over 1 second
    hold power_level_pct while fan remains on

if power_level_pct = 100:
    remain at 100% after the one-second start interval

if power_level_pct changes while fan is already on:
    do not repeat the full-power start interval
    linearly ramp from applied_power_pct to the new power_level_pct over 1 second

if fan.stop or a safety rule requests off:
    interrupt boost or ramp immediately
    apply 0% PWM
    fan.output = off
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

power_level_pct default = 80
power_level_pct allowed values = 20 / 40 / 60 / 80 / 100
```

## 11. Status

```text
ok       - normal operation
warning  - temporary condition or confirmation unavailable
error    - command failed after retries
unknown  - state is not determined after startup
```

Fan Module status reasons added by runtime evaluation:

```text
door_open
door_unknown
climate_invalid
output_failed
confirmation_failed
confirmation_unavailable
```

`fan.output = off` does not mean that Fan Module is inactive. Normal user
control keeps the module active so it continues to report status and accept
commands. Fan Module has no manual enable/disable setting.

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
fan.set_power_level
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
Fan Module does not accept arbitrary PWM percentages outside the five allowed levels.
Fan Module does not create hidden manual-check mode.
Fan Module does not raise error after the first transient confirmation miss.
Fan Module does not define MQTT topic strings.
```
