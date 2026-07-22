# Door Module Contract

Document status: draft
Code status: architecture contract
Scope: Vazon V3 Door Module

## 1. Purpose

```text
Door Module determines local cabinet door state.
```

## 2. Role

```text
Door Module is a Local Sensor Module.
```

## 3. Ownership

```text
Door Module owns only:

door.state
door.status
door.status_reason
door debounce state
```

## 4. Inputs

```text
raw door reed level
monotonic sample time in milliseconds
firmware configuration:
    debounce_ms
    unstable_timeout_ms
```

`debounce_ms` must be greater than zero. `unstable_timeout_ms` must be greater
than or equal to `debounce_ms`. These are firmware configuration values, not
user settings.

Current runtime integration values:

```text
input poll interval = 20 ms
debounce_ms = 100
unstable_timeout_ms = 1000
```

## 5. Hardware Binding

```text
firmware input: DOOR_REED_GPIO
V3 board pinout: GPIO33

electrical input:
pull-up input
active-low door signal

semantic mapping:
GPIO33 = 0 means door closed
GPIO33 = 1 means door open
unstable input means door.state = unknown
```

Closed door is low level. Open door is high level.

## 6. Outputs

```text
door.state = open / closed / unknown
door.status
door.status_reason
```

## 7. State

```text
door.state = open / closed / unknown
door.status = ok / warning / error / inactive / unknown
door.status_reason
last_change_time
door debounce state
```

`last_change_time` is the monotonic millisecond time when the debounced semantic
state last changed. It is `0` until the first state is accepted.

## 8. Logic

```text
if debounced GPIO33 = 0:
    door.state = closed
    door.status = ok

if debounced GPIO33 = 1:
    door.state = open
    door.status = ok

if input is unstable or debounce timeout occurs:
    door.state = unknown
    door.status = warning
    door.status_reason = door_unstable
```

The first raw sample and every changed raw level become a debounce candidate.
The candidate is accepted only after it remains unchanged for `debounce_ms`.
Candidate changes restart the debounce interval but do not restart the overall
unstable interval. A stable candidate is accepted before the unstable timeout
check when both limits are reached by the same sample.

After `unstable_timeout_ms` without an accepted candidate, the module reports
`unknown / warning / door_unstable`. It returns to `ok` when a candidate is
subsequently accepted.

During the initial debounce, `unknown / unknown` keeps actuator outputs in
their safe startup states and does not yet activate the door service source.
After startup, `unknown / warning / door_unstable` activates the service source
as a safe reaction to an undetermined door state.

## 9. Status

```text
ok       - door.state is determined
warning  - door.state is unstable or debounce timeout occurred
error    - sensor failure if detectable
inactive - reserved common module status; not emitted by current Door Module
unknown  - door.state is not determined after startup
```

Manual disable of the door sensor is not supported by the current Module /
Sensor Presence Policy.

## 10. Dependency

```text
Other modules may use door.state as local dependency through their module contracts.
```

## 11. Door Boundary

```text
Door Module owns only door.state and local door status.

System Status owns the top-level warning caused by door.state = open.

Global Core uses `door.state = open / unknown` as the automatic service-mode
source. Closing the door clears only this source and preserves independently
requested manual maintenance.

Fan Module owns its own reaction to door.state = open.

Humidifier Module owns its own reaction to door.state = open.
```

## 12. MQTT Surface

```text
MQTT namespace: door

Published by module:
door.state
door.status
door.status_reason

Accepted commands:
none in normal user runtime

Ack/reject/fail semantics are handled by MQTT Boundary and Command Router.
Topic strings are not owned here.
```

## 13. Boundary

```text
Door Module only reports door state and local door status.
Provisioning, actuator control, system.status, GlobalContext, and other module safety logic are owned by their own contracts.
MQTT topic string table is owned outside this module contract.
Door Module does not read GPIO directly; the caller supplies the normalized raw level.
```
