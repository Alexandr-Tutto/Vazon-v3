# Decision 0002: Day window owner

Status: accepted
Date: 2026-07-08

## Decision

`day_window` is owned by Global Core.

The normal-runtime command for changing day-window schedule fields is:

```text
system.set_day_window
```

`Light Module` may use `day_window.active` in auto mode, but it does not own or update the day-window schedule.

## Consequences

UI controls that edit `day_window.time_on`, `day_window.time_off`, or `day_window.schedule_enabled` must target Global Core through `system.set_day_window`.

Module contracts must not define separate schedule commands for `light`, `fan`, or `humidifier` unless a later accepted decision changes the ownership model.
