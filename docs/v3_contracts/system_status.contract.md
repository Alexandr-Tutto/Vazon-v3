# System Status Contract

Document status: draft
Code status: architecture contract
Scope: Vazon V3 System Status

## 1. Purpose

```text
System Status визначає top-level стан пристрою для UI та індикації.
```

## 2. Role

```text
System Status є Global Aggregation Policy.
```

## 3. Ownership

```text
System Status володіє тільки:

system.status
system.status_reason
system.primary_fault
```

## 4. Inputs

```text
module statuses
connection status
door state
maintenance state
missing device/module/sensor events
```

## 5. Outputs

```text
system.status = ok / warning / error / unknown
system.status_reason
system.primary_fault
```

## 6. Settings

```text
mqtt_warning_grace_sec <= 60
```

## 7. State

```text
system.status = ok / warning / error / unknown
system.status_reason
system.primary_fault = affected system or null
```

## 8. Dependencies

```text
connection: used
module status: used
manual mode: ignored as global warning
```

## 9. Logic

```text
ERROR overrides WARNING.
WARNING overrides OK.

missing expected device/module/sensor:
    system.status = error

door open:
    system.status = warning unless higher error exists

maintenance active:
    system.status = warning unless higher error exists

weak Wi-Fi:
    system.status = warning unless higher error exists

MQTT reconnecting <= mqtt_warning_grace_sec:
    system.status = warning

MQTT disconnected > mqtt_warning_grace_sec:
    system.status = error
    primary_fault = connection

manual mode:
    does not create global warning by itself
```

## 10. Rules

```text
Main screen shows only the affected system, not detailed technical reason.

Top-level format:
Аварія — <збійна система>

Examples:
Аварія — Зв'язок
Аварія — Вентиляція
Аварія — Зволоження
Аварія — Клімат
Аварія — Ґрунт

Detailed reason is shown on the related detail screen.
```

## 11. Multiple Faults

```text
For ordinary non-MQTT errors:
    show primary reason by time of occurrence

If several failures exist:
    detail screens may list them

MQTT connection loss overrides ordinary status display because device state may be stale.
```

## 12. Forbidden

```text
System Status не керує actuator.
System Status не змінює module state.
System Status не дублює detailed diagnostics on main screen.
System Status не робить manual mode глобальним warning.
```
