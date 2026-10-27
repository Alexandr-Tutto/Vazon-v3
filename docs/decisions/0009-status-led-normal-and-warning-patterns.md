# Decision 0009: Status LED normal and warning patterns

Status: accepted
Date: 2026-08-23

## Context

A short green blink in normal operation can look like delayed or irregular
controller activity. Local subsystem warnings also need to remain visually
distinct from door-origin maintenance.

## Decision

Status LED uses these patterns for the affected states:

```text
normal ok                    = green steady
local warning / attention   = green 500 ms ON, 500 ms OFF
door-origin maintenance     = 500 ms green, 500 ms red
```

`door_maintenance` and `local_warning` are separate internal patterns. Their
existing priority order remains unchanged: door-origin maintenance is checked
before system error, and local warning is checked after system error.

## Boundaries

This decision changes only physical LED pattern selection and output timing.
It does not change `system.status`, subsystem warning calculation, maintenance
state, MQTT state, UI presentation, or the remaining LED patterns and
priorities.
