# Air Sensor Module Contract

Document status: draft
Code status: architecture contract
Scope: Vazon V3 Air Sensor Module

## 1. Purpose

```text
Air Sensor Module вимірює локальні параметри повітря.
```

## 2. Role

```text
Air Sensor Module є Local Sensor Module.
```

## 3. Ownership

```text
Air Sensor Module володіє тільки:

air.state
air.status
```

## 4. Inputs

```text
humidity sensor value
temperature sensor value
sensor read result
```

## 5. Outputs

```text
air.humidity.value
air.temperature.value
air.status
```

## 6. State

```text
air.humidity.value = %RH / unknown
air.temperature.value = °C / unknown
last_valid_read_time
last_read_result
```

## 7. Logic

```text
if sensor read valid:
    update air.humidity.value
    update air.temperature.value
    update last_valid_read_time
    air.status = ok
else:
    keep last valid value if allowed by stale_timeout
    update last_read_result
```

## 8. Validity

```text
air.humidity.value valid range = 0..100 %RH
air.temperature.value valid range = +10..+40 °C

If value is out of valid range:
    reading rejected
    status = warning or error by module rule
```

## 9. Stale Rule

```text
If last_valid_read_time is older than stale_timeout:
    air.humidity.value = unknown
    air.temperature.value = unknown
    air.status = error
```

## 10. Status

```text
ok       — valid fresh reading available
warning  — temporary read failure, last value still usable
error    — sensor missing, stale, or invalid reading
inactive — sensor disabled
unknown  — no valid reading after start
```

## 11. Dependency

```text
Інші модулі можуть використовувати air.humidity.value
та air.temperature.value
як local dependency через свої module contracts.
```

## 12. MQTT Surface

```text
MQTT namespace: air

Published by module:
air.state
air.humidity.value
air.temperature.value
air.status
air.status_reason

Accepted commands:
none in normal user runtime

Ack/reject/fail semantics are handled by MQTT Boundary and Command Router.
Topic strings are not owned here.
```

## 13. Boundary

```text
Air Sensor Module only reports local air readings and local air status.
Actuators, system.status, GlobalContext, and other module safety logic are owned by their own contracts.
MQTT topic string table is owned outside this module contract.
```
