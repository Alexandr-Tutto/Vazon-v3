# Vazon V3 Project Overview

Document status: active draft
Code status: architecture reference
Scope: clean Vazon V3 firmware/UI/new hardware workspace

## 1. Purpose

Vazon V3 is the structured firmware and UI architecture for the plant cabinet controller on the V3 PCB.

This repository is the clean V3 workspace. The old Vazon repository is reference-only and must not be copied into this repository as implementation architecture.

The active V3 source of truth is:

```text
docs/v3_contracts/
docs/hardware/v3_board_pinout.md
docs/ui_architecture.md
docs/software_architecture.md
docs/v3_data_model.md
docs/mqtt_topic_registry.md
docs/migration_rules.md
components/board_config/include/vazon_board_config.h
```

## 2. Current State

```text
Contracts: active source of truth
Pinout: active V3 PCB reference
Board config scaffold: active firmware-facing pinout seed
UI architecture: active presentation boundary
Data model: active minimal boundary
MQTT topic registry: active minimal boundary
Software architecture: active ownership/boundary reference
Migration rules: active old-repo reference boundary
Settings persistence implementation: postponed
Remaining hardware confirmations: none
```

No firmware or UI implementation should copy old-repository code directly.

## 3. V3 System Parts

```text
ESP32-WROVER controller
two SHT31 air climate sensors
two configured pot entities
light output
main cabinet fan
humidifier module
status LEDs
provisioning button
UART0 programming/debug service
PWA user interface
MQTT transport
OTA service
```

## 4. Entity Model

```text
system
climate
pot[0]
pot[1]
light
fan
humidifier
door
status_led
provisioning_button
uart_debug
```

`pot[n]` is a combined pot entity:

```text
soil moisture sensor
soil temperature sensor
```

Each pot sensor can be independently disabled through UI settings according to `module_sensor_presence.contract.md`.

## 5. Active Ownership

Detailed behavior is not owned by this overview.

Use:

```text
docs/v3_contracts/00_contract_index.md
```

for the current source-of-truth ownership map.

## 6. Hardware

The active new-board pinout is:

```text
docs/hardware/v3_board_pinout.md
```

The firmware-facing scaffold is:

```text
components/board_config/include/vazon_board_config.h
```

## 7. MQTT

MQTT is transport/boundary work.

Minimal MQTT topic registry is active as a transport boundary.

Module behavior belongs to module contracts, not to MQTT tables.

## 8. UI

The UI is a presentation/control surface.

UI displays state, status, settings, and user actions. It does not own device logic and does not directly control GPIO or module behavior.

Detailed UI rules are in:

```text
docs/ui_architecture.md
```

## 9. Migration Boundary

The old Vazon repository is reference-only.

Use:

```text
docs/migration_rules.md
```

before reusing any old Wi-Fi, MQTT, OTA, firmware, or UI implementation detail.
