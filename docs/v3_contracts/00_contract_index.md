# Vazon V3 Contract Index

Document status: active
Code status: architecture reference
Scope: Vazon V3 contracts and ownership map

## Existing Contract Files

```text
global_core.contract.md
runtime_core.md
system_status.contract.md
status_led.contract.md
provisioning_button.contract.md
uart_debug.contract.md
door.contract.md
climate.contract.md
pot.contract.md
light.contract.md
fan.contract.md
humidifier.contract.md
module_sensor_presence.contract.md
output_confirmation_policy.contract.md
v3_language_and_terms.md
```

## Ownership Map

```text
boot mode / normal vs provisioning -> global_core.contract.md
provisioning button behavior -> provisioning_button.contract.md
status LED patterns -> status_led.contract.md
UART0 debug policy -> uart_debug.contract.md
system.status / system.status_reason / primary_fault -> system_status.contract.md
expected module or sensor absence handling -> module_sensor_presence.contract.md
output command result / confirmation / retry rule -> output_confirmation_policy.contract.md
light output and behavior -> light.contract.md
door state and debounce state -> door.contract.md
SHT31 two-zone climate measurement and aggregation -> climate.contract.md
pot soil moisture and soil temperature -> pot.contract.md
main cabinet fan behavior -> fan.contract.md
humidifier water / mist / local fan behavior -> humidifier.contract.md
shared data-shape boundary rules -> docs/v3_data_model.md
MQTT namespace / retain policy / command envelope -> docs/mqtt_topic_registry.md
UI presentation text and screen behavior -> docs/ui_architecture.md
software boundary rules -> docs/software_architecture.md
old-repository migration rules -> docs/migration_rules.md
```

## Module Groups

```text
Sensor modules:
    door
    climate
    pot

Actuator modules:
    light
    fan
    humidifier

System service modules:
    status_led
    provisioning_button
    uart_debug
```

## Rules

```text
One fact must have one owner file.
Other files may reference that fact but must not restate or reinterpret it.
Data model and MQTT registry are boundary documents, not module logic owners.
Settings persistence implementation is postponed.
Firmware board bring-up skeleton is active.
V3.0 starting UI design baseline is active.
Production module implementation is postponed.
```
