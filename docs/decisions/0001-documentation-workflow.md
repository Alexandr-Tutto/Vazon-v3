# Decision 0001: Documentation workflow

Status: accepted
Date: 2026-06-28

## Decision
All structural documentation work must use explicit work modes:

1. `AUDIT_ONLY` — find inconsistencies, no edits.
2. `PATCH_ONLY` — apply approved or explicitly requested changes only.
3. `REVIEW_ONLY` — verify existing changes, no edits.

User explicit command overrides the default mode for that task.

## Consequences
Agents must not audit and patch in the same task unless the user explicitly requests that combined action.
Unresolved conflicts must be reported, not silently fixed.
The repository documentation is the source of truth.
