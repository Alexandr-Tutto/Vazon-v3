# Command Router Contract

Document status: active draft
Code status: runtime skeleton
Scope: Vazon V3 normal-runtime command routing boundary

## 1. Purpose

```text
Command Router sends a validated command envelope to its owning target.
```

## 2. Ownership

Command Router owns only:

```text
target handler registration
target lookup
route result when target or envelope is invalid
```

Command Router does not own target names, command names, command arguments,
module validation, or device behavior.

## 3. Input

```text
cmd_id
target
cmd
opaque owner-defined args
source
ts
```

MQTT/UI boundaries may create the envelope. The owning module or global policy
interprets and validates args.

## 4. Routing

```text
valid registered target:
    call exactly one registered target handler
    return the handler result

missing or empty target/command name:
    result = rejected

unregistered target:
    result = rejected

duplicate target registration:
    registration fails
```

Registered target strings and handler contexts must remain valid for the
lifetime of the router.

## 5. Result

```text
accepted
rejected
failed
unknown
```

The target owner decides accepted/rejected/failed for commands delivered to
that owner. Command Router only creates rejection for invalid routing input or
an unknown target.

## 6. Forbidden

```text
Command Router does not mutate GlobalContext.
Command Router does not mutate local module state.
Command Router does not control outputs.
Command Router does not calculate status.
Command Router does not define MQTT topics.
Command Router does not interpret owner-defined args.
Command Router does not bypass the owning target handler.
```
