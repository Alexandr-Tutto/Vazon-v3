# Decision 0004: Client cluster and device identity

Status: accepted
Date: 2026-08-01

## Decision

Each client uses a separate MQTT cluster. A client cluster may contain one or
more Vazon controllers belonging to that client.

Every Vazon V3 controller uses this stable device identifier:

```text
Vazon_v3_XX_yyyyyy
```

where:

```text
XX      two-digit physical controller number, including a leading zero
yyyyyy  last three bytes of the Wi-Fi STA MAC address in lowercase hexadecimal
```

Examples:

```text
Vazon_v3_01_a1b2c3
Vazon_v3_02_d4e5f6
```

The physical controller number is assigned to the board configuration. The MAC
suffix is read by firmware from the device. The complete identifier is used as
both the MQTT client ID and the `{device_id}` segment of the MQTT namespace.

## Initial devices

```text
00  V2 working prototype in the operating sealed zone
01  clean V3 laboratory board for bench testing
02  experimental V3 board for V3 validation and eventual replacement of 00
```

Device 00 remains a V2 device; this decision does not rename its existing V2
MQTT identity.

## Consequences

Client isolation is provided by separate MQTT clusters. Controllers within one
client cluster remain isolated from each other by their full device-specific
topic prefix.

Controller credentials should be restricted to the controller's own topic
prefix after its complete identifier is known. UI or backend credentials may
access multiple controller prefixes inside the same client cluster.
