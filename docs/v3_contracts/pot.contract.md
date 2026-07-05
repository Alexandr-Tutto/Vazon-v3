# Pot Module Contract

Document status: draft
Code status: architecture contract
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
pot[pot_id].soil_moisture_enabled = true / false
pot[pot_id].soil_temperature_enabled = true / false

dry_calibration_mv
normal_calibration_mv
wet_calibration_mv

moisture_stale_timeout_sec
temperature_stale_timeout_sec
temperature_low_warn_c
temperature_high_warn_c
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

class thresholds:
0..30 %    = dry
30..70 %   = normal
70..90 %   = wet
90..100 %  = overwet
```

## 12. Stale Rule

```text
If enabled soil moisture reading is older than moisture_stale_timeout_sec:
    soil_moisture.value = unknown
    soil_moisture.class = unknown
    soil_moisture.status = error

If enabled soil temperature reading is older than temperature_stale_timeout_sec:
    soil_temperature.temperature_c = unknown
    soil_temperature.status = error
```

## 13. Status

```text
ok       - enabled pot sensors have valid fresh readings
warning  - temporary read failure, last value still usable, or warning threshold exceeded
error    - enabled sensor missing, stale, invalid reading, or invalid calibration
inactive - both pot sensors are disabled
unknown  - no valid state after startup
```

## 14. MQTT Surface

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

Command payload for pot[pot_id].calibrate_soil_moisture:
point = dry / normal / wet / reset

point = dry:
    store current valid soil_moisture.raw_mv as dry_calibration_mv

point = normal:
    store current valid soil_moisture.raw_mv as normal_calibration_mv

point = wet:
    store current valid soil_moisture.raw_mv as wet_calibration_mv

point = reset:
    clear dry_calibration_mv, normal_calibration_mv and wet_calibration_mv
    for this pot_id
    soil moisture calibration becomes invalid until valid calibration points exist

Ack/reject/fail semantics are handled by MQTT Boundary and Command Router.
Topic strings are not owned here.
```

## 15. Forbidden

```text
Pot Module does not control actuators.
Pot Module does not calculate system.status.
Pot Module does not modify GlobalContext.
Pot Module does not execute watering logic.
Pot Module does not define MQTT topic strings.
```

## Change Log

- 2026-06-29: defined `pot[pot_id].calibrate_soil_moisture` payload `point = dry / normal / wet / reset`.
