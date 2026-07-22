# System Status Contract

Document status: active draft
Code status: active aggregation implementation
Scope: Vazon V3 System Status

## 1. Purpose

```text
System Status determines top-level device state for UI and physical indication consumers.
```

## 2. Role

```text
System Status is a Global Aggregation Policy.
```

## 3. Ownership

```text
System Status owns only:

system.status
system.status_reason
system.primary_fault
system.affected_system
```

System Status owns data and priority, not final UI text formatting.

UI presentation text is owned by:

```text
docs/ui_architecture.md
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
system.affected_system
```

## 6. Settings

```text
mqtt_warning_grace_sec = 60
```

## 7. State

```text
system.status = ok / warning / error / unknown
system.status_reason
system.primary_fault = affected system or null
system.affected_system = climate / pot / light / fan / humidifier / door / connection / system / null
```

## 8. Dependencies

```text
connection: used
module status: used
manual mode: ignored as global warning
door.state: used for top-level warning only
```

## 9. Logic

```text
ERROR overrides WARNING.
WARNING overrides OK.

missing expected device/module/sensor:
    system.status = error

door open:
    system.status = warning unless higher error exists
    affected_system = door

maintenance active:
    system.status = warning unless higher error exists
    affected_system = system

weak Wi-Fi:
    system.status = warning unless higher error exists
    affected_system = connection

MQTT reconnecting <= mqtt_warning_grace_sec:
    system.status = warning
    affected_system = connection

MQTT disconnected > mqtt_warning_grace_sec:
    system.status = error
    primary_fault = connection
    affected_system = connection

manual mode:
    does not create global warning by itself
```

Until the Wi-Fi service provides an RSSI/quality signal, the `weak Wi-Fi` rule
has no firmware input and is not evaluated. Disconnected/connecting Wi-Fi is
reported as a connection warning. An unknown connection state keeps the system
unknown only when no local warning or error has higher priority.

## 10. UI Boundary

```text
System Status provides status, status_reason, primary_fault, and affected_system.
UI converts those fields into main-screen text and detail-screen text.
```

Main screen and subsystem-detail wording must not be defined here.

## 11. Door Boundary

```text
Door Module owns door.state.
System Status owns the top-level warning caused by door.state = open.
Local actuator modules own their own safety reaction to door.state = open.
```

## 12. Multiple Faults

```text
For ordinary non-MQTT errors:
    show primary reason by time of occurrence

If several failures exist:
    detail screens may list them

MQTT connection loss overrides ordinary UI status display because UI data may be stale.
```

This UI override does not define Status LED behavior.

Status LED behavior is owned by:

```text
status_led.contract.md
```

## 13. Forbidden

```text
System Status does not control actuators.
System Status does not change module state.
System Status does not own final UI wording.
System Status does not make manual mode a global warning.
System Status does not own Status LED patterns.
```
