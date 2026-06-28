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
door reed sensor
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

## 9. Status

```text
ok       - door.state is determined
warning  - door.state is unstable or debounce timeout occurred
error    - sensor failure if detectable
inactive - door sensor disabled
unknown  - door.state is not determined after startup
```

## 10. Dependency

```text
Other modules may use door.state as local dependency through their module contracts.
```

## 11. Door Boundary

```text
Door Module owns only door.state and local door status.

System Status owns the top-level warning caused by door.state = open.

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
```
