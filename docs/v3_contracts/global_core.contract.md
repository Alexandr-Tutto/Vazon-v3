# Global Core Contract

Document status: skeleton
Code status: architecture contract
Scope: Vazon V3 Global Core

## 1. Purpose

```text
Global Core формує GlobalContext і GlobalInterlocks у normal runtime.
```

## 2. Boot relation

```text
Boot stage спочатку визначає boot mode:
normal / provisioning.

Global Core стартує тільки в normal mode.
У provisioning mode normal runtime не стартує.
```

## 3. Ownership

```text
Global Core володіє тільки global state normal runtime.
Він не володіє local module state.
Він не володіє provisioning service logic.
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
connection описує стан зв'язку.
connection є global status signal.
connection не є GlobalInterlock.
connection offline не блокує local runtime.

MQTT offline означає втрату зовнішнього каналу.
MQTT offline не змінює actuator outputs напряму.
MQTT offline не переводить пристрій у provisioning.
```

## 10. GlobalInterlocks

```text
Global Core формує interlocks:
maintenance_active
critical_sensor_missing
critical_actuator_failed

mqtt_offline не входить у GlobalInterlocks.
```

## 11. Command handling

```text
Global Core приймає тільки global commands normal runtime:
maintenance on/off
reset defaults
global service commands

Wi-Fi provisioning не є normal runtime command flow.
```

## 12. Provisioning boundary

```text
Provisioning mode вибирається тільки на boot stage.
Provisioning button перевіряється до старту normal runtime.
Після старту normal runtime provisioning button ігнорується.

У provisioning mode:
climate runtime не стартує
all actuators off
MQTT off
telemetry off
device starts Wi-Fi AP
accepted input: Wi-Fi credentials only
```

## 13. Forbidden

```text
Global Core не керує actuator напряму.
Global Core не змінює local module state напряму.
Global Core не виконує local module logic.
Global Core не рахує final system.status.
Global Core не запускає provisioning з normal runtime.
```
