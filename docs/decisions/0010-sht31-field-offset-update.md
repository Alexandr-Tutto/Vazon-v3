# Decision 0010: SHT31 field offset update

Status: accepted
Date: 2026-10-27

## Context

After approximately three months of colocated cabinet observation across the
displayed 55–85 %RH range, both device 02 SHT31 channels continued to read
lower than household reference indicators. The initial `+5.0` percentage-point
field correction was therefore judged insufficient for the next field-trial
iteration.

The household indicators are not traceably calibrated, so this change is an
operational field correction rather than a claim about absolute SHT31 error.

## Decision

The total additive field-trial offset for each SHT31 channel is:

```text
climate.settings.rh_offset_0x44_pct = +12.0
climate.settings.rh_offset_0x45_pct = +12.0
```

This is the total offset, not an additional `+12.0` on top of the existing
value. The valid adjustable range remains `-20..20` percentage points.

## Persistence and migration

Runtime-settings schema advances from version 2 to version 3. When a valid
schema-v2 blob is loaded, every persisted setting and both soil calibration
records are preserved except the two SHT31 RH offsets, which are set once to
`+12.0`. The migrated blob is then saved as schema 3.

Schema-v1 migration and defaults for a controller without valid stored
settings also use `+12.0` for both channels. Later user adjustments remain
persistent and are not overwritten when loading schema 3.

## Verification boundary

The next field trial must compare both corrected readings with the colocated
references over time and record whether the remaining error is positive,
negative, or acceptably small. The correction applies before MQTT publication
and before fan or humidifier logic, as defined by the Climate Module contract.
