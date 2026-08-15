# Decision 0006: SHT31 relative-humidity calibration offsets

Status: accepted
Date: 2026-08-15

## Context

During device 02 field trials, both SHT31 sensors reported relative humidity
approximately 5 to 6 percentage points below three colocated household
reference instruments while temperature readings remained aligned.

## Decision

Climate Module owns two independent additive relative-humidity calibration
settings:

```text
climate.settings.rh_offset_0x44_pct
climate.settings.rh_offset_0x45_pct
```

Each offset is added to the corresponding valid raw SHT31 relative-humidity
reading before the value is published or used by climate, fan, or humidifier
logic. The corrected value is limited to `0..100 %RH`.

The accepted offset range is `-20..20` percentage points. The field-trial
default and the migrated value for existing schema-v1 settings are `+5.0` for
both sensors. Each value remains independently adjustable for later
calibration.

## Persistence and migration

The runtime-settings schema advances from version 1 to version 2. Loading a
valid schema-v1 blob preserves every existing setting and adds `+5.0` for both
new offsets. The migrated settings are saved as schema version 2 by the normal
settings persistence task.

## Boundaries

The I2C service continues to decode the factory SHT31 value without project
calibration. Climate Module applies the project calibration. UI may edit and
display the offsets but does not correct sensor values locally.
