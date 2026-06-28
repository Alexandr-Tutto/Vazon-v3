# Door Module Contract

Document status: draft
Code status: architecture contract
Scope: Vazon V3 Door Module

## 1. Purpose

```text
Door Module визначає локальний стан дверей.
```

## 2. Role

```text
Door Module є Local Sensor Module.
```

## 3. Ownership

```text
Door Module володіє тільки:

door.state
door.status
```

## 4. Inputs

```text
door reed sensor
```

## 5. Outputs

```text
door.state = open / closed / unknown
door.status
```

## 6. State

```text
door.state = open / closed / unknown
last_change_time
```

## 7. Logic

```text
if reed sensor active:
    door.state = closed
else:
    door.state = open
```

## 8. Status

```text
ok       — door.state визначений
warning  — door.state нестабільний або debounce timeout
error    — sensor failure якщо detectable
inactive — door sensor disabled
unknown  — door.state ще не визначений після старту
```

## 9. Dependency

```text
Інші модулі можуть використовувати door.state
як local dependency через свої module contracts.
```

## 10. MQTT Surface

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

## 11. Boundary

```text
Door Module only reports door state and local door status.
Provisioning, actuator control, system.status, GlobalContext, and other module safety logic are owned by their own contracts.
MQTT topic string table is owned outside this module contract.
```
