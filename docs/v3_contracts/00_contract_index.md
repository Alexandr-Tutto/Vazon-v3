# Vazon V3 Contract Index

Document status: active
Code status: architecture reference
Scope: Vazon V3 contracts

## 1. Purpose

This file defines the current source-of-truth map for Vazon V3 contracts.

It exists to prevent recursive documentation, duplicated rules, and multiple files describing the same decision in different words.

## 2. Documentation Layers

```text
contract documents
    current implementation-facing source of truth
    one rule has one owning contract

index document
    navigation and ownership map
    does not duplicate contract content

migration rules
    old-repository reference boundary
    not module or firmware logic
```

## 3. General Rule

```text
One fact must have one owner file.
Other files may reference that fact but must not restate or reinterpret it.
```

If a rule appears in more than one contract, only the owner file is authoritative. The duplicated text should be removed or replaced with a short reference.

## 4. Existing Contract Files In Repo

This section lists files that actually exist in `docs/v3_contracts/` in this repository.

### Core

```text
global_core.contract.md
runtime_core.md
system_status.contract.md
```

### System Services

```text
status_led.contract.md
provisioning_button.contract.md
uart_debug.contract.md
```

### Local Sensor Modules

```text
door.contract.md
air_sensor.contract.md
climate.contract.md
pot.contract.md
```

### Local Actuator / Climate Modules

```text
light.contract.md
fan.contract.md
humidifier.contract.md
```

### Global Policies

```text
module_sensor_presence.contract.md
output_confirmation_policy.contract.md
```

### Index / Supporting Rules

```text
00_contract_index.md
v3_language_and_terms.md
```

## 5. Ownership Map

```text
system boot mode / normal vs provisioning
    owner: global_core.contract.md

provisioning physical button behavior
    owner: provisioning_button.contract.md

status LED patterns and priority display
    owner: status_led.contract.md

UART0 serial debug output policy
    owner: uart_debug.contract.md

system.status, primary fault, main-screen alarm priority
    owner: system_status.contract.md

expected device/module/sensor missing behavior
    owner: module_sensor_presence.contract.md

output command result / physical confirmation / retry-to-error rule
    owner: output_confirmation_policy.contract.md

light output and light auto/manual behavior
    owner: light.contract.md

door state and debounce-level state ownership
    owner: door.contract.md

single air sensor reading contract
    owner: air_sensor.contract.md

SHT31 two-zone climate aggregation
    owner: climate.contract.md

pot soil moisture + soil temperature state, calibration, and per-sensor enable settings
    owner: pot.contract.md

main cabinet fan behavior
    owner: fan.contract.md

humidifier water/mist/local-fan behavior
    owner: humidifier.contract.md

module-owned MQTT surface fields and command names
    owner: corresponding module contract

UI/MQTT/Command Router/software boundary rules
    owner: docs/software_architecture.md

system.day_window derivation
    owner: global_core.contract.md

old-repository migration rules
    owner: docs/migration_rules.md
```

## 6. Rules Not to Duplicate

```text
system.day_window meaning
    do not restate in local module contracts beyond "uses system.day_window"

missing expected sensor/module -> alarm
    do not restate in every module beyond "handled by Module / Sensor Presence Policy"

main screen alarm formatting and priority
    do not restate in module contracts

output confirmation policy
    do not restate generic confirmation/retry semantics in every module;
    module contracts may state local retry parameters and reference the policy

MQTT topic strings
    do not define exact topic strings inside module contracts;
    module contracts own namespace, exposed fields, and command names only

UI formatting details
    not part of module contracts unless the module owns that UI-facing state

old implementation behavior
    do not copy directly; use docs/migration_rules.md
```

## 7. Runtime Core Note

`runtime_core.md` is retained as an umbrella architecture overview for V3 runtime.

It may mention broad runtime flow and point to the owning documents.

It must not own detailed definitions for GlobalContext, GlobalInterlocks, Status Aggregator, Command Flow, MQTT Boundary, UI Boundary, or module logic.

It must not be used as a reason to create duplicate detailed contracts unless a later phase explicitly requires them.

## 8. Migration Reference

Old Vazon repository documents and code are not implementation source of truth for this repository.

Old implementation may be inspected only through:

```text
docs/migration_rules.md
```

## 9. Freeze Rule

Do not create a new contract for a rule if an existing contract already owns that rule.

Before adding a new contract:

```text
1. check this index
2. identify the owner file
3. update the owner file instead of creating a parallel description
```

## 10. Current Phase Boundary

This phase covers clean V3 seed documentation and contract inventory accuracy.

The following are intentionally postponed to a later phase:

```text
full MQTT topic registry
settings persistence implementation
firmware implementation
UI implementation
```

Do not add these unless the project phase is explicitly changed.

## 11. Implementation References Outside Contracts

```text
docs/ui_architecture.md
    UI-only status/status_reason display mapping and screen behavior

docs/hardware/v3_board_pinout.md
    active V3 PCB pinout

docs/software_architecture.md
    active software ownership and boundary rules

docs/migration_rules.md
    old-repository reference boundary

components/board_config/include/vazon_board_config.h
    firmware board config scaffold for V3 pinout migration
```

## 12. Change Log

- 2026-06-28: seeded clean Vazon V3 repository with active source-of-truth documents only.
- 2026-06-28: added migration boundary rule for using the old Vazon repository as reference only.