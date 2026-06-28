# Vazon V3 Runtime Core

Document status: skeleton
Code status: architecture overview
Scope: Vazon V3 firmware runtime overview

This document is an umbrella overview for normal runtime.

It does not own detailed module behavior, global signal definitions, MQTT topic
names, UI presentation, or output confirmation rules.

## 1. Purpose

Runtime Core describes how active runtime parts are connected.

Detailed rules belong to the owning documents listed below.

## 2. Runtime Shape

```text
boot mode decision
    -> normal runtime or provisioning mode

normal runtime
    -> Global Core creates GlobalContext
    -> Local Modules evaluate their own state/settings/status/outputs
    -> Global Runtime Policies aggregate or constrain system-level behavior
    -> Transport/UI boundaries publish state and submit commands

provisioning mode
    -> normal runtime does not start
```

## 3. Runtime Flow

```text
UI/MQTT input
    -> boundary
    -> Command Router
    -> owning Global Policy or Local Module
    -> module/global validation
    -> state/status/output request
    -> output driver when needed
    -> publish/display updated state/status/settings
```

## 4. Owner References

```text
boot mode / normal vs provisioning
    owner: global_core.contract.md

GlobalContext, maintenance, day_window, connection, GlobalInterlocks
    owner: global_core.contract.md

local module state/settings/status/output behavior
    owner: each module contract

system.status and system.status_reason aggregation
    owner: system_status.contract.md

expected module/sensor missing behavior
    owner: module_sensor_presence.contract.md

output confirmation and retry-to-error policy
    owner: output_confirmation_policy.contract.md

UI/MQTT/Command Router/software boundary rules
    owner: docs/software_architecture.md

UI presentation details
    owner: docs/ui_architecture.md

MQTT namespace, retain policy, and command envelope
    owner: docs/mqtt_topic_registry.md

module MQTT fields and command names
    owner: corresponding module contract
```

## 5. Local Module Rule

Local Modules may use `GlobalContext`, local sensors, local settings, and
commands.

Local Modules must not redefine global signal meaning, write `GlobalContext`,
calculate final `system.status`, or control another module's output.

## 6. Boundary Rule

UI, MQTT, Command Router, topic registry, persistence helper, and Output Driver
are not device-logic owners.

They route, persist, display, publish, or apply already requested physical
output state according to their boundary role.

## 7. Duplication Rule

If this file conflicts with an owning contract or active architecture document,
the owner file wins and this overview must be corrected.

Do not add detailed module rules here.
