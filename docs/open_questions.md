# Open Questions

This file contains unresolved project questions.

Agents must add items here when a conflict cannot be resolved from existing accepted documents.
Agents must not resolve these questions without explicit user approval.

## Format

### OQ-000
Status: unresolved
Question:
Affected files:
Decision needed:

## Active questions

### OQ-001
Status: unresolved
Question: Which authentication, credential-delivery, user-identity, and access-revocation model replaces direct browser MQTT credentials before public or customer deployment?
Affected files: `docs/decisions/0005-prototype-pwa-direct-mqtt.md`, `ui/pwa/src/mqtt-config.js`, `docs/mqtt_topic_registry.md`
Decision needed: production access model and migration point

### OQ-002
Status: unresolved
Question: Which working tachometer source and input circuit will be used to commission GPIO34 rotation confirmation for the main fan?
Affected files: `docs/v3_contracts/fan.contract.md`, `docs/hardware/v3_board_pinout.md`, `components/board_config/include/vazon_board_config.h`
Decision needed: replacement fan or verified tachometer hardware

### OQ-003
Status: unresolved
Question: Does the humidifier require physical presence/output feedback beyond the water reed input?
Affected files: `docs/v3_contracts/humidifier.contract.md`, `docs/v3_contracts/output_confirmation_policy.contract.md`
Decision needed: whether and how to add hardware confirmation

### OQ-004
Status: unresolved
Question: At which release stage must OTA rollback and boot confirmation be implemented?
Affected files: `components/ota_service/vazon_ota_service.c`, `docs/software_architecture.md`
Decision needed: required OTA recovery policy before wider deployment
