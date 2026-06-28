# Light Module Contract

Document status: draft
Code status: architecture contract
Scope: Vazon V3 Light Module

## 1. Purpose

```text
Light Module керує локальним освітленням.
```

## 2. Role

```text
Light Module є Local Climate Module.
```

## 3. Ownership

```text
Light Module володіє тільки:

light.state
light.settings
light.output
light.status
```

## 4. Inputs

```text
GlobalContext
light settings
light commands
output confirmation if available
```

## 5. Global Dependencies

```text
maintenance: used
day_window: used
connection: ignored
global_interlocks: used
```

## 6. Settings

```text
mode = auto / manual
manual_state = on / off
```

## 7. State

```text
output_request = on / off
last_command_result
last_output_confirmed
```

## 8. Rule Order

```text
global -> local safety -> sensor validity -> mode/runtime -> manual -> auto -> output confirmation
```

## 9. Global Logic

```text
if maintenance.active:
    output_request = on
else:
    continue normal light logic
```

## 10. Manual Logic

```text
if mode = manual:
    output_request = manual_state
```

## 11. Auto Logic

```text
if mode = auto:
    output_request = day_window.active
```

## 12. Output

```text
Light Module керує тільки light output.

output_request:
on
off
```

## 13. Status

```text
ok       — output відповідає очікуваному стану
warning  — confirmation unavailable
error    — output command failed або confirmation failed
inactive — module disabled
unknown  — output state ще не визначений після старту
```

## 14. MQTT Surface

```text
MQTT namespace: light

Published by module:
light.state
light.status
light.status_reason
light.settings

Accepted commands:
light.set_mode
light.set_manual_state
light.set_settings

Ack/reject/fail semantics are handled by MQTT Boundary and Command Router.
Topic strings are not owned here.
```

## 15. Output Confirmation

```text
Light Module follows Output Confirmation Policy.
Module-specific state fields:
last_command_result
last_output_confirmed
```

## 16. Forbidden

```text
Light Module не володіє day_window schedule.
Light Module не змінює GlobalContext.
Light Module не рахує system.status.
Light Module не керує чужими outputs.
Light Module не визначає day/night для інших модулів.
Light Module не визначає MQTT topic strings.
```
