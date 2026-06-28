# Climate Module Contract

Document status: draft
Code status: architecture contract
Scope: Vazon V3 Climate Module

## 1. Purpose

```text
Climate Module determines air climate state from two independent SHT31 sensors.
```

## 2. Role

```text
Climate Module is a Local Sensor/Aggregation Module.
```

## 3. Ownership

```text
Climate Module owns only:

climate.state
climate.settings
climate.status
climate.status_reason
SHT31 address binding
SHT31 read/stale state
```

## 4. Inputs

```text
SHT31 sensor at address 0x44
SHT31 sensor at address 0x45
sensor read result
climate settings
```

## 5. Hardware Binding

```text
I2C bus:
SDA = CLIMATE_I2C_SDA_GPIO = GPIO21
SCL = CLIMATE_I2C_SCL_GPIO = GPIO22

SHT31 devices:
I2C address 0x44
I2C address 0x45
```

## 6. Outputs

```text
climate.sensor_0x44.temperature_c
climate.sensor_0x44.humidity_pct
climate.sensor_0x44.status

climate.sensor_0x45.temperature_c
climate.sensor_0x45.humidity_pct
climate.sensor_0x45.status

climate.rh_delta_pct
climate.temperature_delta_c
climate.status
climate.status_reason
```

## 7. Settings

```text
temperature_low_warn = 18
temperature_high_warn = 35
humidity_low_warn = 20
humidity_high_warn = 95

sht31_stale_timeout_sec = 300

temperature_delta_warn = 5
temperature_delta_error = 10
```

## 8. State

```text
sensor_0x44.status = ok / warning / error / unknown
sensor_0x45.status = ok / warning / error / unknown

climate.status = ok / warning / error / inactive / unknown
climate.status_reason
```

## 9. Dependencies

```text
connection: ignored
GlobalContext: not used for measurement
```

## 10. Logic

```text
Both SHT31 sensors are required for normal operation.

if SHT31 0x44 is missing:
    sensor_0x44.status = error
    climate.status = error
    status_reason = sht31_0x44_missing

if SHT31 0x45 is missing:
    sensor_0x45.status = error
    climate.status = error
    status_reason = sht31_0x45_missing

if any SHT31 data is stale for sht31_stale_timeout_sec:
    climate.status = error
    status_reason = sht31_stale

if any value is invalid:
    climate.status = error
    status_reason = invalid_value

if temperature out of warning range:
    climate.status = warning

if humidity out of warning range:
    climate.status = warning

calculate:
    rh_delta_pct = abs(sensor_0x44_rh - sensor_0x45_rh)
    temperature_delta_c = abs(sensor_0x44_temp - sensor_0x45_temp)

if RH delta too high:
    climate.status = warning
    status_reason = rh_delta_high

if temperature_delta_c > temperature_delta_warn:
    climate.status = warning
    status_reason = temperature_delta_high

if temperature_delta_c > temperature_delta_error:
    climate.status = error
    status_reason = temperature_delta_critical
```

## 11. Rules

```text
The two SHT31 sensors are independent sensors.
In this project they are logically used as two air zones.
Physical placement labels are installation/UI metadata, not a climate logic rule.

Main UI must show both sensors/zones.
Do not collapse sensors into one averaged value.
```

## 12. Status

```text
ok       - both sensors are valid
warning  - value is outside warning range or sensor delta is high
error    - missing sensor, stale data, invalid value, or critical temperature delta
inactive - module disabled or not detected
unknown  - state is not determined after startup
```

## 13. MQTT Surface

```text
MQTT namespace: climate

Published by module:
climate.state
climate.sensor_0x44.temperature_c
climate.sensor_0x44.humidity_pct
climate.sensor_0x44.status
climate.sensor_0x45.temperature_c
climate.sensor_0x45.humidity_pct
climate.sensor_0x45.status
climate.rh_delta_pct
climate.temperature_delta_c
climate.status
climate.status_reason
climate.settings

Accepted commands:
climate.set_settings

Ack/reject/fail semantics are handled by MQTT Boundary and Command Router.
Topic strings are not owned here.
```

## 14. Forbidden

```text
Climate Module does not control actuators.
Climate Module does not modify GlobalContext.
Climate Module does not calculate system.status.
Climate Module does not replace two sensors with one averaged value.
Climate Module does not define MQTT topic strings.
```
