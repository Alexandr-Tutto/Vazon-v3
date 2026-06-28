# Provisioning Button Contract

Document status: draft
Code status: architecture contract
Scope: Vazon V3 Provisioning Button

## 1. Purpose

```text
Provisioning Button запускає Wi-Fi provisioning mode при старті пристрою.
```

## 2. Role

```text
Provisioning Button є boot-time service input.
```

## 3. Hardware

```text
button GPIO
```

## 4. Inputs

```text
button state during boot check window
hold time
```

## 5. Outputs

```text
boot_mode = normal / provisioning
wifi_settings_clear request
```

## 6. Behavior

```text
Після power-on / reset пристрій перевіряє Provisioning Button.

Якщо кнопка утримана потрібний час:
  boot_mode = provisioning
  stored Wi-Fi settings are cleared
  normal runtime не стартує

Якщо кнопка не утримана:
  boot_mode = normal
  normal runtime стартує
```

## 7. Runtime Rule

```text
Після старту normal runtime Provisioning Button ігнорується.
Provisioning Button не є органом керування.
```

## 8. Forbidden

```text
Provisioning Button не запускає provisioning з normal runtime.
Provisioning Button не керує actuator.
Provisioning Button не змінює module state.
Provisioning Button не проходить через Command Router.
```
