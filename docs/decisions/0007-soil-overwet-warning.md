# Decision 0007: Soil overwet is a warning

Status: accepted
Date: 2026-08-23

## Context

After soil-moisture calibration was completed on device 02, the Pot Module
correctly classified a reading at or above 90% as `overwet`, but the pot and
top-level system remained `ok`. This allowed the main UI to report that
everything was normal while the detailed pot view showed overwatering.

## Decision

A fresh, valid soil-moisture reading with class `overwet` produces:

```text
soil_moisture.status = warning
pot.status = warning
pot.status_reason = soil_overwet
system.status = warning unless a higher-priority error exists
system.affected_system = pot
```

`soil_overwet` describes a plant-condition warning, not a sensor or controller
failure. It does not control an actuator automatically.

## Priority

Sensor errors and stale/invalid data retain error priority. If the current read
fails while a previous value remains usable, `moisture_read_failed` retains the
warning reason because the current condition cannot be confirmed.

## UI

The UI presents `soil_overwet` as a yellow warning and tells the user to stop
watering and allow the substrate to dry. It must not label an overwet pot as
normal.
