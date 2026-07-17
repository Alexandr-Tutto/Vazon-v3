# UART Debug Contract

Document status: draft
Code status: architecture contract
Scope: Vazon V3 UART0 serial debug output

## 1. Purpose

```text
UART Debug defines what firmware may print to UART0 in normal, diagnostic,
and test modes.
```

## 2. Role

```text
UART Debug is a System Service Boundary.
It is not a runtime module and not a control channel.
```

## 3. Hardware Binding

```text
UART0 TXD = GPIO1
UART0 RXD = GPIO3
```

## 4. Normal Runtime Output

```text
Allowed in normal runtime:

boot mode
firmware version
reset reason
fatal errors
provisioning start/stop
Wi-Fi connection state
MQTT connection state
OTA state
module startup result
hardware init failure
```

## 5. Diagnostic / Test Output

```text
Allowed only in diagnostic or test mode:

raw sensor readings
GPIO levels
I2C scan results
OneWire scan results
timing measurements
heap/task diagnostics
command parsing traces
module reevaluate traces
```

## 6. Input Policy

```text
UART input is ignored in normal runtime.
UART commands are not available in normal runtime.
Temporary raw GPIO commands are allowed only in board bring-up firmware.
```

## 7. Forbidden

```text
UART Debug does not change device state.
UART Debug does not change settings.
UART Debug does not control outputs outside board bring-up firmware.
UART Debug does not bypass Command Router.
UART Debug does not print passwords, tokens, private keys, certificates, or full config dumps.
UART Debug does not stream unbounded sensor spam in normal runtime.
```
