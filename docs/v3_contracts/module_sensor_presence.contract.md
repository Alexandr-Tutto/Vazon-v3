# Module / Sensor Presence Contract

Document status: draft
Code status: architecture contract
Scope: Vazon V3 Module / Sensor Presence Policy

## 1. Purpose

```text
Module / Sensor Presence Policy defines how the device handles an expected device, module, or sensor that is disabled, missing, or not producing data.
```

## 2. Role

```text
Module / Sensor Presence Policy is a Global Runtime Policy.
```

## 3. Ownership

```text
Policy owns only:

presence handling rule
manual sensor enable/disable settings

Specific module state/status remains owned by the corresponding module contract.
```

## 4. Inputs

```text
expected device/module/sensor list
sensor read result
module health result
manual sensor enable/disable setting
```

## 5. Outputs

```text
presence failure event
affected module status impact
system status input event
```

## 6. Settings

```text
pot[0].soil_moisture_enabled = true / false
pot[0].soil_temperature_enabled = true / false
pot[1].soil_moisture_enabled = true / false
pot[1].soil_temperature_enabled = true / false
```

These four settings are exposed in UI as four independent switches.

## 7. State

```text
device/module/sensor presence = present / missing / unknown
```

## 8. Dependencies

```text
System Status: used
module status: used
UI availability: not used for autodetection now
```

## 9. Logic

```text
Autodetection is postponed.

if expected device/module/sensor is manually disabled:
    affected module status = inactive
    no red alarm for that disabled sensor

if expected enabled device/module/sensor disappears or provides no data:
    affected module status = error
    emit presence failure event for System Status

No graded absence/degradation logic is implemented now.
No automatic hiding of UI controls based on autodetection now.
```

## 10. Pot Sensor Exception

```text
Manual disable is allowed only for configured pot sensors:

pot[0].soil_moisture
pot[0].soil_temperature
pot[1].soil_moisture
pot[1].soil_temperature

Each configured pot sensor can be disabled independently.
```

## 11. Status

```text
ok       - expected enabled devices/modules/sensors are present
error    - expected enabled device/module/sensor is missing or provides no data
unknown  - presence state is not determined yet
```

## 12. Forbidden

```text
Policy does not implement automatic module discovery now.
Policy does not create optional-module gray state logic now.
Policy does not hide controls based on autodetection now.
Policy does not create sensor/module inventory screen now.
Policy does not allow manual disable for SHT31 climate sensors, humidifier water sensor, door sensor, or actuators.
```
