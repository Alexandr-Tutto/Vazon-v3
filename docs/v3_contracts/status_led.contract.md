# Status LED Contract

Document status: active draft
Code status: implemented for normal runtime and provisioning
Scope: Vazon V3 Status LED

## 1. Purpose

```text
Status LED shows the basic physical device state on the enclosure.
```

## 2. Role

```text
Status LED is a System Service Module.

Its role is to physically display already available local device indication inputs.
```

Status LED is not the same level as UI.

UI may lose live MQTT data when the device is offline. Status LED remains local and may still show device-side communication state physically.

## 3. Hardware

```text
two-color LED
red GPIO
green GPIO
```

## 4. Inputs

```text
boot mode
provisioning state
local connection state
system.status
```

## 5. Outputs

```text
red LED state
green LED state
blink pattern
```

## 6. Ownership

```text
Status LED Module owns only LED outputs and LED pattern selection.
```

It does not own UI text or MQTT-published status formatting.

## 7. Priority

```text
LED indication priority:

1. provisioning active
2. system.error not caused only by MQTT offline
3. local warning / attention
4. mqtt offline
5. wi-fi offline
6. normal ok
```

MQTT offline has its own LED pattern because the LED is a local physical indication path, while UI delivery may be unavailable because of the same MQTT loss.

## 8. LED States

```text
green short blink, long pause - normal ok

green 50/50 blink             - provisioning AP active

red short blink, long pause   - wi-fi offline

red 50/50 blink               - mqtt offline

red steady                    - system error not caused only by MQTT offline

red/green alternate blink     - local warning / attention
```

Exact runtime timing:

```text
short blink, long pause       - 200 ms ON, 1800 ms OFF
50/50 blink                   - 500 ms ON, 500 ms OFF
red/green alternate blink     - 500 ms green, 500 ms red
```

When `system.status` is still `unknown` and neither offline state is known,
both LED channels stay OFF. This avoids displaying a false normal state during
startup.

The normal-runtime entry passes `provisioning active = false`. The boot-only
provisioning path passes `provisioning active = true` and owns no other status
selection inputs.

## 9. Forbidden

```text
Status LED does not calculate system.status.
Status LED does not change GlobalContext.
Status LED does not change module state.
Status LED does not control climate actuators.
Status LED does not define UI text.
Status LED does not publish MQTT messages.
```
