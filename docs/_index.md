# Documentation Index

## Project

Vazon v3.

## Purpose

This file is the navigation map for active project documentation.

Agents must read it to understand which documents are active and how documentation is organized. This file does not replace the owning documents listed below.

## Current Working Baseline

```text
UI baseline: V3.0 starting UI design is fixed by the current main branch state.
Firmware baseline: PCB v1.0 board bring-up skeleton is the active embedded focus.
Hardware baseline: docs/hardware/v3_board_pinout.md and board_config must match.
Branch: main
```

## Required Agent Entry Points

```text
AGENTS.md
    rules for AI agents working with this repository

docs/_index.md
    this navigation map
```

When auditing or editing documentation, agents must first read both files.

## Primary Project Documents

```text
README.md
    repository entry point and active source-of-truth list

docs/project_overview.md
    project purpose, current state, system parts, and active ownership overview

docs/software_architecture.md
    firmware/UI/MQTT/software boundary rules

docs/ui_architecture.md
    UI structure, screens, states, and user-visible behavior

docs/hardware/v3_board_pinout.md
    board pinout and hardware signal names

components/board_config/include/vazon_board_config.h
    firmware board configuration and signal definitions
```

## Contract Documents

```text
docs/v3_contracts/00_contract_index.md
    contract navigation and ownership map

docs/v3_contracts/*.contract.md
    owning contracts for functional modules/entities and policies

docs/v3_contracts/runtime_core.md
    umbrella runtime overview; not a detailed logic owner

docs/v3_contracts/v3_language_and_terms.md
    terminology and language rules
```

## Boundary / Supporting Documents

```text
docs/v3_data_model.md
    shared data-shape boundary rules; does not own module fields

docs/mqtt_topic_registry.md
    minimal MQTT namespace, retain policy, and command envelope boundary

docs/firmware_skeleton.md
    current firmware skeleton layout, PCB smoke-test scope, and software port order

docs/migration_rules.md
    old-repository reference boundary and migration rules
```

## Workflow Documents

```text
docs/decisions/*.md
    accepted architectural, naming, terminology, and workflow decisions

docs/open_questions.md
    unresolved items; agents must not resolve these without explicit approval
```

## Deprecated Documents

```text
None declared yet.
```

## Authority Rule

If a document is not listed here, agents must not assume its authority without checking with the user or reporting it as an undocumented source.

If listed documents conflict, agents must report the conflict instead of choosing one silently. Use the conflict report format from `AGENTS.md`.
