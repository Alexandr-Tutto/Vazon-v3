# Pot Module Contract

Document status: active draft
Code status: active local module implementation
Scope: Vazon V3 Pot Module

## 1. Purpose

```text
Pot Module represents one configured pot as a pair of local soil sensors:
soil moisture + soil temperature.
```

## 2. Role

```text
Pot Module is a Local Sensor Module.
```

## 3. Instance

```text
V3 has two configured pot instances:

pot_id = 0
pot_id = 1

Each pot has:
one soil moisture sensor
one soil temperature sensor
```

## 4. Ownership

```text
Pot Module owns only:

pot[pot_id].state
pot[pot_id].settings
pot[pot_id].status
pot[pot_id].status_reason
pot[pot_id].soil_moisture
pot[pot_id].soil_temperature
pot[pot_id].soil_moisture_calibration
pot[pot_id].sensor_enable_settings
```

## 5. Hardware Binding

```text
pot_id 0 soil moisture:
firmware input: POT0_SOIL_MOISTURE_ADC_GPIO
V3 board pinout: GPIO36
ADC channel: ADC1_CH0

pot_id 1 soil moisture:
firmware input: POT1_SOIL_MOISTURE_ADC_GPIO
V3 board pinout: GPIO39
ADC channel: ADC1_CH3

soil temperature bus:
firmware bus: POT_TEMP_ONEWIRE_GPIO
V3 board pinout: GPIO32
bus type: OneWire
external 4.7 kOhm pull-up required

pot_id 0 soil temperature:
DS18B20 binding: current prototype enumeration index 0

pot_id 1 soil temperature:
DS18B20 binding: current prototype enumeration index 1
```

## 6. Inputs

```text
soil moisture ADC value for pot_id
DS18B20 temperature value for pot_id
sensor read result
calibration settings for pot_id
pot[pot_id].soil_moisture_enabled
pot[pot_id].soil_temperature_enabled
```

## 7. Outputs

```text
pot[pot_id].soil_moisture.value
pot[pot_id].soil_moisture.class
pot[pot_id].soil_temperature.temperature_c
pot[pot_id].status
pot[pot_id].status_reason
```

## 8. Settings

```text
pot[pot_id].soil_moisture_enabled = true / false; default = true
pot[pot_id].soil_temperature_enabled = true / false; default = true

dry_calibration_mv
normal_calibration_mv
wet_calibration_mv

moisture_stale_timeout_sec = 300
temperature_stale_timeout_sec = 300
temperature_low_warn_c = 5
temperature_high_warn_c = 40
```

## 9. State

```text
soil_moisture.raw_adc_value
soil_moisture.raw_mv
soil_moisture.value = 0..100 % / unknown
soil_moisture.class = dry / normal / wet / overwet / unknown
soil_moisture.status = ok / warning / error / inactive / unknown

soil_temperature.temperature_c = degrees C / unknown
soil_temperature.status = ok / warning / error / inactive / unknown

pot.status = ok / warning / error / inactive / unknown
pot.status_reason
last_valid_moisture_read_time
last_valid_temperature_read_time
```

## 10. Logic

```text
if pot[pot_id].soil_moisture_enabled = false:
    soil_moisture.status = inactive
    soil_moisture.value = unknown
    soil_moisture.class = unknown

if pot[pot_id].soil_temperature_enabled = false:
    soil_temperature.status = inactive
    soil_temperature.temperature_c = unknown

if soil moisture is enabled and ADC read valid:
    convert raw_mv to soil_moisture.value using dry/normal/wet calibration
    clamp soil_moisture.value to 0..100 %
    calculate soil_moisture.class
    update last_valid_moisture_read_time

if soil temperature is enabled and DS18B20 read valid:
    update soil_temperature.temperature_c
    update last_valid_temperature_read_time

pot.status is derived from enabled child sensor statuses:
    any enabled child sensor error -> pot.status = error
    any enabled child sensor warning -> pot.status = warning
    all enabled child sensors ok -> pot.status = ok
    both child sensors disabled -> pot.status = inactive
```

## 11. Moisture Class

```text
soil_moisture.value is calculated from dry_calibration_mv..wet_calibration_mv.

Calibration mapping is piecewise linear:
dry_calibration_mv = 0%
normal_calibration_mv = 50%
wet_calibration_mv = 100%

Both ascending and descending monotonic sensor characteristics are valid.
Until all three calibration points form a strictly monotonic sequence, raw ADC
and mV values remain available, normalized value/class are unknown, and status
reason is calibration_invalid.

class thresholds:
0 <= value < 30 %    = dry
30 <= value < 70 %   = normal
70 <= value < 90 %   = wet
90 <= value <= 100 % = overwet
```

## 12. Soil Moisture Calibration Command

```text
Command:
pot[pot_id].calibrate_soil_moisture

Accepted args:
point = dry
point = normal
point = wet
point = reset
```

```text
point = dry:
    capture current soil moisture reading as dry_calibration_mv

point = normal:
    capture current soil moisture reading as normal_calibration_mv

point = wet:
    capture current soil moisture reading as wet_calibration_mv

point = reset:
    clear the current soil_moisture_calibration sequence for the selected pot
    user must repeat calibration from dry point
```

The UI text and step-by-step user procedure are owned by `docs/ui_architecture.md`.

## 13. Stale Rule

```text
If enabled soil moisture reading is older than moisture_stale_timeout_sec:
    soil_moisture.value = unknown
    soil_moisture.class = unknown
    soil_moisture.status = error

If enabled soil temperature reading is older than temperature_stale_timeout_sec:
    soil_temperature.temperature_c = unknown
    soil_temperature.status = error
```

## 14. Status

```text
ok       - enabled pot sensors have valid fresh readings
warning  - temporary read failure, last value still usable, or warning threshold exceeded
error    - enabled sensor missing, stale, invalid reading, or invalid calibration
inactive - both pot sensors are disabled
unknown  - no valid state after startup
```

## 15. MQTT Surface

```text
MQTT namespace: pot/{pot_id}

Published by module:
pot[pot_id].soil_moisture.value
pot[pot_id].soil_moisture.class
pot[pot_id].soil_moisture.status
pot[pot_id].soil_temperature.temperature_c
pot[pot_id].soil_temperature.status
pot[pot_id].status
pot[pot_id].status_reason
pot[pot_id].settings

Accepted commands:
pot[pot_id].set_soil_moisture_enabled
pot[pot_id].set_soil_temperature_enabled
pot[pot_id].set_settings
pot[pot_id].calibrate_soil_moisture

Ack/reject/fail semantics are handled by MQTT Boundary and Command Router.
Topic strings are not owned here.
```

Command Router targets are `pot/0` and `pot/1`. UI/MQTT boundaries translate
serialized argument values into the typed owner arguments before routing.

## 16. Forbidden

```text
Pot Module does not control actuators.
Pot Module does not calculate system.status.
Pot Module does not modify GlobalContext.
Pot Module does not execute watering logic.
Pot Module does not define MQTT topic strings.
```
