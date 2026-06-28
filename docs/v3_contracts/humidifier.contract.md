# Humidifier Module Contract

Document status: draft
Code status: architecture contract
Scope: Vazon V3 Humidifier Module

## 1. Purpose

```text
Humidifier Module controls local air humidification.

Entity:
humidifier = water sensor + mist generator + local humidifier fan
```

## 2. Role

```text
Humidifier Module is a Local Climate Module.
```

## 3. Ownership

```text
Humidifier Module owns only:

humidifier.state
humidifier.settings
humidifier.output
humidifier.status
humidifier.status_reason
humidifier water sensor debounce state
```

## 4. Inputs

```text
GlobalContext
climate humidity values
door.state
water present sensor
humidifier settings
humidifier commands
output confirmation if available
```

## 5. Hardware Binding

```text
water sensor firmware input: HUMIDIFIER_WATER_PRESENT_GPIO
V3 board pinout: GPIO35

electrical input:
input-only GPIO
external pull-up on PCB
no internal pull-up

semantic mapping:
GPIO35 = 0 means reed closed
reed closed means humidifier.water_status = empty
GPIO35 = 1 means reed open
reed open means humidifier.water_status = present
unstable input means humidifier.water_status = unknown
```

```text
mist firmware output: HUMIDIFIER_MIST_GPIO
V3 board pinout: GPIO26
active-low switched output
0 = mist ON
1 = mist OFF

local fan firmware output: HUMIDIFIER_FAN_GPIO
V3 board pinout: GPIO19
active-low switched output
0 = fan ON
1 = fan OFF
```

## 6. Outputs

```text
mist output
local humidifier fan output
humidifier.status
humidifier.status_reason
```

## 7. Settings

```text
mode = auto / manual
runtime = day / always

rh_start
rh_stop
rh_delta

mist_power_level = low / medium / high

manual_mist_duration_sec = 180
post_fan_sec = 20
```

## 8. State

```text
humidifier.water_status = present / empty / unknown
humidifier.mist_output = on / off / unknown
humidifier.fan_output = off / on / post_run / unknown
humidifier.status = ok / warning / error / inactive / unknown
humidifier.status_reason
humidifier.water_sensor_debounce_state
```

## 9. Dependencies

```text
maintenance: used
day_window: used
connection: ignored
global_interlocks: used

door.state: used
climate humidity values: used
```

## 10. Logic

```text
if debounced GPIO35 = 1:
    water_status = present
else if debounced GPIO35 = 0:
    water_status = empty
else:
    water_status = unknown

if maintenance.active:
    mist off
    fan off

if door.state = open:
    mist off
    fan off

if water_status = empty:
    mist off
    fan post-run for post_fan_sec if mist was active
    status_reason = no_water

if water_status = unknown:
    mist off
    fan post-run for post_fan_sec if mist was active
    status_reason = water_unknown

if mode = manual:
    accepted manual mist command runs mist for manual_mist_duration_sec
    then mist off
    fan post-run for post_fan_sec

if mode = auto:
    if runtime = day and day_window.active = false:
        mist off
    if min(zone0_rh, zone1_rh) <= rh_start:
        mist may start
    if max(zone0_rh, zone1_rh) >= rh_stop:
        mist stops

When mist is on:
    local fan is on

After mist turns off:
    local fan stays in post_run for post_fan_sec
```

## 11. Rules

```text
Water status changes only after debounce accepts a stable input.
Raw GPIO transitions are not published as humidifier.water_status.

rh_stop - rh_start >= rh_delta
rh_delta default = 10%
rh_delta minimum = 5%
rh_stop <= 90%

mist_power_level has three levels:
low
medium
high

post_fan_sec default = 20
post_fan_sec range = 0..600
```

## 12. Status

```text
ok       - normal operation
warning  - temporary condition blocks humidification or confirmation is unavailable
error    - no water, water unknown, output failed, confirmation failed, or required sensor invalid
inactive - module disabled or not detected
unknown  - state is not determined after startup
```

## 13. MQTT Surface

```text
MQTT namespace: humidifier

Published by module:
humidifier.state
humidifier.water_status
humidifier.mist_output
humidifier.fan_output
humidifier.status
humidifier.status_reason
humidifier.settings

Accepted commands:
humidifier.set_mode
humidifier.set_runtime
humidifier.manual_mist
humidifier.stop
humidifier.set_mist_power_level
humidifier.set_settings

Ack/reject/fail semantics are handled by MQTT Boundary and Command Router.
Topic strings are not owned here.
```

## 14. Output Confirmation

```text
Humidifier Module follows Output Confirmation Policy for:
mist output
local humidifier fan output

Water sensor state is not output confirmation.
```

## 15. Forbidden

```text
Humidifier Module does not own air/climate sensors.
Humidifier Module does not own door sensor.
Humidifier Module does not modify GlobalContext.
Humidifier Module does not calculate system.status.
Humidifier Module does not control main fan output.

Humidifier Module does not create top-level water/*, pump/*, watering/* namespace.
Humidifier Module does not use timer auto mode.
Humidifier Module does not use cyclic on_sec/off_sec pump model.
Humidifier Module does not give normal user control for the local humidifier fan.
Humidifier Module does not define MQTT topic strings.
```
