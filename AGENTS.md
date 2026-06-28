# AGENTS.md

## Project
Vazon v3.

## Main rule
This repository is the source of truth.
Do not rely on memory, guesses, or previous chats.

## Default mode
AUDIT_ONLY.

## Work modes

### AUDIT_ONLY
Find inconsistencies only.
Do not edit files.
Do not rename concepts.
Do not add architecture.
Do not resolve conflicts silently.

### PATCH_ONLY
Edit only files explicitly listed in the prompt.
Apply only approved changes.
Do not perform cleanup outside the requested scope.

### REVIEW_ONLY
Review existing changes.
Do not edit files.
Report only whether the requested changes were applied correctly.

## Source of truth order
1. docs/decisions/*.md
2. docs/v3_contracts/*.md
3. docs/hardware/*.md
4. docs/ui_architecture.md
5. README.md

If documents conflict, report the conflict instead of choosing one.

## Documentation rules
- Every sensor or actuator must have one contract file.
- Contract names, MQTT names, UI names, and board_config names must not diverge silently.
- Deprecated terms must be explicitly marked as deprecated.
- New terms require a decision file.

## Conflict report format
For every conflict, output:

- File A:
- File B:
- Conflict:
- Severity: critical / major / minor
- Proposed resolution:
- Requires user decision: yes / no

## Hard prohibitions
Do not:
- rewrite documents for style;
- merge documents unless requested;
- delete documents unless requested;
- introduce new abstractions;
- rename entities without explicit approval;
- treat newer file timestamp as higher authority;
- fix issues while auditing.
