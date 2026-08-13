# Fan Module Contract

Document status: active
Code status: active local module implementation; tachometer confirmation deferred
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
fan.rpm = non-negative integer / unknown
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
if maintenance.manual_active or maintenance.external_active:
    fan off immediately
    fan.auto_state = blocked
    fan.status = ok
    fan.status_reason = null
    skip manual and auto run logic

if door.state = open:
    fan.status = warning
    fan.status_reason = door_open
    block automatic run logic
    if mode = auto:
        fan off
        fan.auto_state = blocked
    if mode = manual and no manual run was accepted after the door opened:
        fan off
        fan.auto_state = blocked
    if mode = manual and fan.manual_run was accepted after the door opened:
        run fan for manual_duration_sec
        fan.auto_state = running
        keep fan.status_reason = door_open

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
    if runtime = day and day_window.active = unknown:
        fan off
        fan.auto_state = alert
        fan.status = warning
        fan.status_reason = day_window_unknown
        skip selected auto strategy

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

if door opens:
    fan off immediately
    cancel any manual run that was active before the door opened
    reset timer auto cycle

while door remains open:
    a new fan.manual_run command may start a bounded manual service run
    automatic operation remains blocked

if door becomes unknown, or manual/external maintenance becomes active:
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
    switch GPIO13 master +12 V power off immediately
    release the GPIO25 open-collector PWM output
    report applied_power_pct = 0
    fan.output = off
```

## 10. Rules

```text
auto_strategy default = delta
mode default = auto
runtime default = day

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
warning  - temporary blocking or invalid-input condition
error    - command failed after retries
unknown  - state is not determined after startup
```

Fan Module status reasons added by runtime evaluation:

```text
door_open
door_unknown
climate_invalid
day_window_unknown
output_failed
confirmation_failed
confirmation_unavailable
```

While fan output is ON, GPIO34 tachometer pulses provide rotation confirmation
and RPM. The current interface uses two pulses per revolution. Absence of a
tachometer pulse for one one-second sample does not raise an error. Three
consecutive seconds without pulses after master power ON produce
`last_output_confirmed = false`, `fan.status = error`, and
`fan.status_reason = confirmation_failed`. A valid pulse sample clears that
failure and sets `last_output_confirmed = true`.

Tachometer confirmation cannot distinguish a missing fan, a stalled fan, a
broken tachometer wire, or a failed tachometer output.

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

Command Router target: `fan`.

## 13. Output Confirmation

```text
Fan Module follows Output Confirmation Policy.
Module-specific retry parameters:
retry_count = 3
retry_delay_sec = 1

tachometer_gpio = GPIO34
tachometer_pulses_per_revolution = 2
tachometer_sample_sec = 1
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

## 15. Current Implementation Boundary

```text
Fan Module calculates output_request and requested_power_pct, including boost
and ramp timing.

GPIO13 applies switched +12 V master power.
GPIO25 applies the requested 25 kHz PWM through an external inverting
open-collector stage. Output Driver converts logical fan duty to the inverted
GPIO duty; Fan Module remains unaware of electrical inversion.
GPIO34 captures externally conditioned tachometer pulses.

Current tachometer commissioning exception:
    GPIO34 tachometer capture is disabled
    RPM and physical rotation confirmation remain unknown
    this exception is removed when a working tachometer source is available

output_request = off:
    GPIO13 LOW before PWM changes
    GPIO25 open-collector stage released
    applied_power_pct = 0

output_request = on:
    requested PWM preset before GPIO13 HIGH
    applied_power_pct follows requested_power_pct
```
