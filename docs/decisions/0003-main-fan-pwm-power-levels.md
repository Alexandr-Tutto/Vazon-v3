# Decision 0003: Main fan PWM power levels

Status: accepted
Date: 2026-07-17

## Decision

The main cabinet fan uses PWM power control with these user-selectable levels:

```text
20%
40%
60%
80%
100%
```

The selected setting is:

```text
fan.settings.power_level_pct
```

The normal-runtime command is:

```text
fan.set_power_level
```

with:

```text
power_level_pct = 20 / 40 / 60 / 80 / 100
```

The selected level applies whenever the fan runs in either manual or auto
mode. The default is 80%.

## Start sequence

Every transition from OFF to ON uses this sequence:

```text
100% PWM for 1 second
linear ramp from 100% to fan.settings.power_level_pct over 1 second
hold fan.settings.power_level_pct while the fan remains ON
```

If the selected level is 100%, the fan remains at 100% after the one-second
start interval and no downward ramp is required.

Changing the selected level while the fan is already ON smoothly ramps from
the currently applied level to the new level over 1 second. It does not repeat
the full-power start interval.

Any stop or safety OFF request interrupts the start/ramp sequence and applies
OFF immediately.

## Ownership

Fan Module owns the selected power level and the start/ramp sequence. Output
Driver applies the requested PWM duty and does not calculate Fan Module logic.

UI only selects and displays power levels. UI does not implement PWM timing or
the start/ramp sequence.
