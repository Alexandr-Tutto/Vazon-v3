# Global Core Contract

Document status: active bring-up contract
Code status: normal-runtime context, connection state, and boot provisioning implemented
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
maintenance.manual_active = true / false
maintenance.external_active = true / false
maintenance.door_active = true / false
maintenance.reason = manual / external / door / unknown
```

Maintenance is a global runtime state, not a direct output command.

Maintenance has two independent activation sources:

```text
manual/external maintenance request
door service request
```

Confirmed `door.state = open` activates the door service request. An
undetermined door with `door.status = warning / error` also activates it.
The transient `unknown / unknown` state during initial debounce does not
activate service mode; outputs remain in their safe startup states.
`door.state = closed` clears only the door service request. It does not clear a
manual/external maintenance request. If both sources are active, the explicit
manual/external reason has priority in the single `maintenance.reason` field.

`maintenance.active` is derived as:

```text
maintenance.manual_active OR maintenance.external_active OR maintenance.door_active
```

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

Startup defaults before persisted settings are loaded:

```text
schedule_enabled = true
time_on = 08:00
time_off = 21:00
active = unknown until the clock is valid and the window is evaluated
```

## 9. Connection

```text
connection describes communication state.
connection is a global status signal.
connection is not a GlobalInterlock.
connection offline does not block local runtime.

connection:
  wifi_state = unknown / disconnected / connecting / connected
  mqtt_state = unknown / disconnected / connecting / connected

MQTT offline means loss of external channel.
MQTT offline does not change actuator outputs directly.
MQTT offline does not switch the device to provisioning.
```

Normal runtime loads stored Wi-Fi/MQTT connection configuration. If the
configuration is missing or invalid, local runtime continues and both
connection states are `disconnected`; provisioning is not entered
automatically. A valid configuration starts Wi-Fi STA, and MQTT starts only
after Wi-Fi obtains an IP address.

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
system.set_day_window
reset defaults
global service commands

Wi-Fi provisioning is not normal runtime command flow.
```

`system.set_day_window` updates provided `day_window` fields.

Command Router representation:

```text
target = system
cmd = set_day_window
args = provided day_window patch

target = system
cmd = set_maintenance
args.active = true / false
```

`system.set_maintenance` changes only the explicit manual maintenance request.
It does not clear an active door service request.

```text
Accepted args:
schedule_enabled = true / false, optional
time_on = HH:MM, optional
time_off = HH:MM, optional
```

Updates are atomic. If any provided time is not a valid strict `HH:MM` value,
the command is rejected and no day-window field changes.

Day-window evaluation:

```text
schedule_enabled = false:
    active = true

schedule_enabled = true and clock is not valid:
    active = unknown

time_on < time_off:
    active from time_on inclusive to time_off exclusive

time_on > time_off:
    window crosses midnight
    active from time_on inclusive through midnight to time_off exclusive

time_on = time_off:
    active = true for the full day
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
accepted input: Wi-Fi and MQTT connection credentials
```

## 13. Forbidden

```text
Global Core does not command actuators directly.
Global Core does not change local module state directly.
Global Core does not execute local module logic.
Global Core does not calculate final system.status.
Global Core does not start provisioning from normal runtime.
```
