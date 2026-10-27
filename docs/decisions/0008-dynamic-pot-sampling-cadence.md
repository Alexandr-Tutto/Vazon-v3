# Decision 0008: Dynamic pot sampling cadence

Status: accepted
Date: 2026-08-23

## Context

Soil calibration needs fresh readings soon after the substrate condition
changes, while normal soil conditions change slowly. A fixed 10-second read
cycle provides little calibration feedback and produces more normal-runtime
state changes than needed.

## Decision

Pot sensor acquisition uses two cadences:

```text
calibration cadence = 2 seconds
normal cadence = 30 seconds
```

Calibration cadence is active while at least one enabled soil-moisture sensor
has an incomplete `dry_valid / normal_valid / wet_valid` sequence. Normal
cadence resumes when every enabled moisture sensor has a complete sequence.
Disabled moisture sensors do not keep calibration cadence active.

During normal-cadence waiting, App Core checks for a newly started calibration
at least every 2 seconds and advances the next sensor acquisition when needed.

## Boundaries

The cadence controls ADC and soil-temperature acquisition. It does not change
the 32-sample ADC averaging rule, calibration-point acceptance rules, retained
MQTT semantics, or stale timeouts. MQTT State Publisher continues to suppress
unchanged payloads.
