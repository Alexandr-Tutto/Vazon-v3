# Global Core Contract

Document status: skeleton
Code status: architecture contract
Scope: Vazon V3 Global Core

## 1. Purpose

```text
Global Core forms GlobalContext and GlobalInterlocks in normal runtime.
```

## 2. Boot relation

```text
Boot stage first determines boot mode:
normal / provisioning.

Global Core starts only in normal mode.
In provisioning mode normal runtime does not start.
```

## 3. Ownership

```text
Global Core owns only global state normal runtime.
It does not own local module state.
It does not own provisioning service logic.
```

## 4. Inputs

```text
local module status
selected local module signals
connection state
internal timers
system commands
```

## 5. Outputs

```text
GlobalContext
global_interlocks
global status signals for Status Aggregator
```

## 6. GlobalContext signals

```text
maintenance
day_window
connection
global_interlocks
```

## 7. Maintenance

```text
maintenance.active = true / false
maintenance.reason = manual / external / unknown
```

Maintenance is a global runtime state, not a direct output command.

Each local module owns its own reaction to `maintenance.active`:

```text
fan: maintenance.active -> fan off
humidifier: maintenance.active -> mist off and local fan off
light: maintenance.active -> light on as service lighting
```

Service lighting is intentionally different from fan and humidifier maintenance behavior.

## 8. Day Window

```text
Global Core owns day_window.

day_window:
  active = true / false / unknown
  schedule_enabled = true / false
  time_on = HH:MM
  time_off = HH:MM

day_window is part of GlobalContext.
```

## 9. Connection

```text
connection describes communication state.
connection is a global status signal.
connection is not a GlobalInterlock.
connection offline does not block local runtime.

MQTT offline means loss of external channel.
MQTT offline does not change actuator outputs directly.
MQTT offline does not switch the device to provisioning.
```

## 10. GlobalInterlocks

```text
Global Core forms interlocks:
maintenance_active
sensor_missing
actuator_failed

mqtt_offline is not part of GlobalInterlocks.
```

## 11. Command handling

```text
Global Core accepts only global commands in normal runtime:
maintenance on/off
reset defaults
global service commands

Wi-Fi provisioning is not normal runtime command flow.
```

## 12. Provisioning boundary

```text
Provisioning mode is selected only at boot stage.
Provisioning button is checked before normal runtime starts.
After normal runtime starts, provisioning button is ignored.

In provisioning mode:
normal runtime does not start
outputs remain in their board-defined safe state
MQTT off
telemetry off
device starts Wi-Fi AP
accepted input: Wi-Fi credentials only
```

## 13. Forbidden

```text
Global Core does not command actuators directly.
Global Core does not change local module state directly.
Global Core does not execute local module logic.
Global Core does not calculate final system.status.
Global Core does not start provisioning from normal runtime.
```
