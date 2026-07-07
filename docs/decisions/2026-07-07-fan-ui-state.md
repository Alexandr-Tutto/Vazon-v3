# Decision: Fan UI state presentation

Date: 2026-07-07

## Status

Accepted.

## Scope

PWA normal UI for `Вентиляція` / `fan`.

## Decision

When `fan.status_reason` is present, the main overview tile for `Вентиляція` shows the short state text:

```text
Увага
```

When `fan.status_reason = door_open`, the subsystem status panel header shows:

```text
Вентиляція - заблоковано через відкриті двері
```

The `Вентиляція` subsystem status panel does not show separate cards for:

```text
Вихід
Підтвердження
Стан підсистеми
```

The previous status card label `Поточний стан` is replaced with:

```text
Режим
```

## Notes

This is UI presentation only. It does not change `fan.status`, `fan.status_reason`, `fan.output`, MQTT names, or firmware logic.
