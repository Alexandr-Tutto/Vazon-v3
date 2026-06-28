# Output Confirmation Policy Contract

Document status: draft
Code status: architecture contract
Scope: Vazon V3 output confirmation policy

## 1. Purpose

```text
Output Confirmation Policy defines how firmware treats physical output command result and optional output feedback.
```

This policy is shared by actuator modules.

## 2. Ownership

```text
This policy owns:

output command result semantics
output confirmation semantics
retry / fail transition rule
confirmation-unavailable rule
```

Individual modules own their own output request and module-specific retry parameters.

## 3. Terms

```text
output_request
    requested logical output state from module logic:
    on / off / pwm / level / no_change

command_result
    result of asking Output Driver to apply output_request:
    accepted / rejected / failed

confirmation
    optional physical or driver-level proof that output reached requested state

confirmation_unavailable
    no hardware feedback exists, or driver cannot verify physical state
```

## 4. Normal Path

```text
1. Module logic calculates output_request.
2. Output Driver applies output_request to GPIO/PWM/driver.
3. Output Driver returns command_result.
4. If confirmation exists, module checks confirmation.
5. Module updates local state/status.
```

## 5. Confirmation Unavailable

```text
If confirmation is not available:
    module may keep status = ok if command_result = accepted
    module may expose status_reason = confirmation_unavailable
    module must not claim physical confirmation
```

This is not automatically an error.

## 6. Retry Rule

```text
If command_result = failed or confirmation mismatch:
    retry according to module retry settings

If retries are exhausted:
    module.status = error
    module.status_reason = output_failed or confirmation_failed
```

## 7. Transient Rule

```text
A single transient confirmation miss is not enough to raise module.status = error.
```

The module may use warning during retry or uncertainty window.

## 8. Safe State Rule

```text
If GlobalContext or local safety requires safe state:
    module requests safe output state
    confirmation policy still applies to that safe output command
```

If safe state command fails, this is an error owned by the affected module.

## 9. MQTT / UI Boundary

```text
MQTT and UI may display command_result, output state, status, and status_reason.
MQTT and UI do not interpret physical output confirmation themselves.
```

## 10. Forbidden

```text
Output Driver does not calculate module.status.
Output Driver does not read GlobalContext.
MQTT does not confirm physical output directly.
UI does not confirm physical output directly.
A module does not confirm another module's output.
```
