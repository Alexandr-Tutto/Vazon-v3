# V3 Migration Rules

Document status: active
Code status: implementation rule

## Core Rule

The old Vazon project is not an architecture donor.

The old Vazon project may be used only as a reference for proven parameters, timings, ESP-IDF API sequences, service configuration, and known edge cases.

## Allowed

```text
Reuse proven constants and timings.
Reuse tested Wi-Fi connection parameters and init order.
Reuse tested MQTT connection parameters, TLS/cert handling, and reconnect behavior.
Reuse tested OTA manifest/download/install flow after isolating it as a service.
Reuse known ESP-IDF API sequences.
Reuse bug fixes and edge-case handling after identifying the reason.
```

## Forbidden

```text
Do not paste old module code as-is.
Do not import old MQTT topic logic as source of truth.
Do not import old UI AppState structure.
Do not import old Kconfig names unless renamed through V3 board_config/settings.
Do not let old service code call actuators or local modules directly.
Do not preserve old coupling just because it worked.
Do not bypass V3 contracts to make old code fit.
```

## Required Migration Process

```text
1. Read the relevant V3 contract or architecture document.
2. Inspect old code only for parameters, timing, init order, and edge cases.
3. Write the V3 implementation in the new module/service boundary.
4. Keep service layers isolated from device business logic.
5. Verify the result against active V3 docs before committing.
```
