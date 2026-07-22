# Light Module Contract

Document status: draft
Code status: architecture contract
Scope: Vazon V3 Light Module

## 1. Purpose

```text
Light Module controls the cabinet light output.
```

## 2. Role

```text
Light Module is a Local Actuator Module.

Its role level is the same as the Main Fan Module:
it owns one local actuator output and its module behavior.

The difference from the Main Fan Module is maintenance behavior:
light is forced ON during maintenance as service lighting.
```

## 3. Ownership

```text
Light Module owns only:

light.state
light.settings
light.output
light.status
light.status_reason
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
maintenance: used as service-light force-on condition
day_window: used in auto mode
connection: ignored
global_interlocks: used
```

## 6. Settings

```text
mode = auto / manual
manual_state = on / off
```

Current startup defaults:

```text
mode = auto
manual_state = off
```

## 7. State

```text
light.output = on / off / unknown
light.status = ok / warning / error / inactive / unknown
light.status_reason
output_request = on / off
last_command_result
last_output_confirmed
```

## 8. Rule Order

```text
global -> maintenance service light -> local safety -> mode/runtime -> manual -> auto -> output confirmation
```

## 9. Maintenance Logic

```text
if maintenance.active:
    output_request = on
    status_reason = maintenance_service_light
    skip normal auto/manual light logic
```

Maintenance does not mean all actuators are forced off.

For Light Module only, maintenance means service lighting is enabled.

## 10. Manual Logic

```text
if maintenance.active = false and mode = manual:
    output_request = manual_state
```

## 11. Auto Logic

```text
if maintenance.active = false and mode = auto:
    output_request = day_window.active
```

If `day_window.active = unknown`, the safe request is `off` and the module
reports `warning / day_window_unknown`.

## 12. Output

```text
Light Module controls only light output.

output_request:
on
off
```

## 13. Status

```text
ok       - output command accepted and no error is active
warning  - maintenance_service_light is active or day_window is unknown
error    - output command failed or confirmation failed
inactive - module disabled
unknown  - output state is not determined after startup
```

The current PCB has no physical light-output feedback. A successfully accepted
GPIO command therefore keeps `status = ok` and
`last_output_confirmed = unknown`. Confirmation unavailability alone is not a
warning. A GPIO driver failure produces `error / output_failed`.

## 14. MQTT Surface

```text
MQTT namespace: light

Published by module:
light.state
light.output
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

`light.set_settings` does not accept `day_window` schedule fields.
`day_window` is owned by Global Core.

## 15. Output Confirmation

```text
Light Module follows Output Confirmation Policy.
Module-specific state fields:
last_command_result
last_output_confirmed
```

## 16. Forbidden

```text
Light Module does not own day_window schedule.
Light Module does not modify GlobalContext.
Light Module does not calculate system.status.
Light Module does not control other outputs.
Light Module does not define day/night for other modules.
Light Module does not define MQTT topic strings.
```
