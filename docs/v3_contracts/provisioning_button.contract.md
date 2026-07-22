# Provisioning Button and Service Contract

Document status: active bring-up contract
Code status: boot selection and local AP/HTTP flow implemented
Scope: Vazon V3 Provisioning Button and Provisioning Service

## 1. Purpose

```text
Provisioning Button запускає connection provisioning mode при старті пристрою.
```

## 2. Role

```text
Provisioning Button є boot-time service input.
Provisioning Service володіє лише локальним AP/HTTP потоком введення connection config.
```

## 3. Hardware

```text
button GPIO0, active low, internal pull-up
```

## 4. Inputs

```text
button state during 5000 ms boot check window
continuous hold time = 5000 ms
```

## 5. Outputs

```text
boot_mode = normal / provisioning
connection_config_clear request
```

## 6. Behavior

```text
Після safe-off та ініціалізації системних сервісів пристрій протягом 5000 ms
перевіряє Provisioning Button з інтервалом 50 ms.

Якщо кнопку натиснуто у цьому вікні та безперервно утримано 5000 ms:
  boot_mode = provisioning
  stored Wi-Fi and MQTT connection settings are cleared
  normal runtime не стартує
  actuator outputs залишаються у safe-off
  MQTT і telemetry не стартують
  запускається SoftAP `Vazon_XXXX`
  provisioning AP password = `vazon1234`
  HTTP form доступна на `http://192.168.4.1/`
  form приймає Wi-Fi SSID/password та MQTT host/port/username/password
  після валідного збереження пристрій перезапускається

Якщо кнопку не натиснуто або відпущено до завершення hold time:
  boot_mode = normal
  normal runtime стартує

Відсутній або невалідний connection config сам по собі не запускає provisioning.
У цьому випадку normal runtime працює локально offline.
```

## 7. Runtime Rule

```text
Після старту normal runtime Provisioning Button ігнорується.
Provisioning Button не є органом керування.
Provisioning Service не є частиною normal runtime.
```

## 8. Forbidden

```text
Provisioning Button не запускає provisioning з normal runtime.
Provisioning Button не керує actuator.
Provisioning Button не змінює module state.
Provisioning Button не проходить через Command Router.
Provisioning Service не публікує telemetry і не запускає MQTT.
Provisioning Service не виводить паролі або HTTP form body в UART log.
```
