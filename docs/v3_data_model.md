# Vazon V3 Data Model Boundary

Document status: active
Code status: architecture reference
Scope: shared data-shape rules, not module field ownership

## 1. Purpose

This document defines the minimal shared data-model rules used by firmware, MQTT, and UI.

It does not own module fields, module behavior, settings lists, command names, GPIO binding, persistence layout, or UI wording.

## 2. Ownership Rule

```text
One data fact has one owner.

Module state/settings/status fields are owned by the corresponding contract in docs/v3_contracts/.
Global/system fields are owned by global/system contracts.
UI-only labels and grouping are owned by docs/ui_architecture.md.
MQTT topic strings are owned by docs/mqtt_topic_registry.md.
GPIO names and levels are owned by docs/hardware/v3_board_pinout.md and board_config.
```

This file may define common envelope shape and naming rules only.

## 3. Common Field Categories

```text
state
    physical or logical state owned by a module

settings
    persisted configuration owned by a module or global owner

status
    ok / warning / error / inactive / unknown

status_reason
    machine-readable reason owned by the corresponding module or global policy

command_result
    accepted / rejected / failed / unknown

output
    requested/reported actuator state: on / off / unknown
```

Status values may be narrowed by the owning contract.

## 4. Top-Level Entity Names

```text
system
climate
pot[0]
pot[1]
door
light
fan
humidifier
status_led
provisioning_button
uart_debug
```

`pot[n]` is the canonical entity name for a configured pot.

UI code may use JavaScript-friendly containers such as arrays, but it must preserve the same logical identity:

```text
pot[0]
pot[1]
```

Do not introduce parallel names such as `pot0`, `pot1`, or `soil` as source-of-truth entity names.

## 5. Minimal Object Rule

Every publishable entity object should expose only fields owned by that entity contract.

Recommended common shape:

```text
entity.status
entity.status_reason
entity.state / entity.output / entity.settings as defined by owner contract
```

Do not add aggregate fields here unless an owner contract defines them.

## 6. Unknown / Disabled Rule

```text
unknown
    value cannot currently be trusted or read

inactive
    expected sensor/module is intentionally disabled by settings
```

Detailed missing/disabled behavior is owned by:

```text
docs/v3_contracts/module_sensor_presence.contract.md
```

## 7. MQTT Relation

MQTT payloads carry owner-defined fields.

This document does not list module payload fields. Use the owning module contract for that.

## 8. UI Relation

UI may create local presentation state, labels, cards, and derived summaries.

UI-local presentation fields must not become firmware or MQTT source-of-truth fields unless an owner contract is updated first.

## 9. Forbidden

```text
Do not duplicate module field lists here.
Do not define exact MQTT topics here.
Do not define command names here.
Do not define GPIO names or active levels here.
Do not define persistence keys here.
Do not use this document to bypass module contracts.
```
