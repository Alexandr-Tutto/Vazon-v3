# Documentation Index

## Project
Vazon v3.

## Purpose
This file is the navigation map for project documentation.
Agents must use it to understand which documents are active and how documentation is organized.

## Current baseline
Branch: main
Baseline commit: a9f5c3f Align V3 skeleton with contracts

## Primary documents

- `AGENTS.md` — rules for AI agents working with this repository.
- `docs/_index.md` — navigation map for active documentation.
- `docs/ui_architecture.md` — UI structure, screens, states, and user-visible logic.
- `docs/hardware/v3_board_pinout.md` — board pinout and hardware signal names.
- `docs/v3_contracts/*.contract.md` — contracts for functional modules/entities, MQTT surfaces, and module interfaces.
- `components/board_config/include/vazon_board_config.h` — firmware board configuration and signal definitions.

## Decision records

- `docs/decisions/*.md` — accepted architectural, naming, terminology, and workflow decisions.

## Open questions

- `docs/open_questions.md` — unresolved items. Agents must not resolve these without explicit approval.

## Deprecated documents

None declared yet.

## Agent rules

When auditing or editing documentation, agents must first read:

1. `AGENTS.md`
2. `docs/_index.md`

If a document is not listed here, agents must not assume its authority without checking with the user or reporting it as an undocumented source.
