# UI-архітектура PWA

Статус документа: target
Статус коду: not implemented
Repository scope: clean Vazon V3 workspace

## Мета

Vazon PWA — це проста панель догляду за рослинною шафою.

Головне питання першого екрана:

```text
Все добре, чи потрібна увага?
```

UI не є технічним MQTT/debug dashboard.
Технічні деталі показуються тільки в контекстних панелях або сервісному режимі.

## Рівні інтерфейсу

```text
1. Огляд
   головне повідомлення
   плитки підсистем
   основні кліматичні числа

2. Деталі підсистеми
   причини
   стан
   рекомендована дія
   налаштування саме цієї підсистеми

3. Сервіс
   OTA
   технічна інформація
   прихована діагностика
```

## Перший екран

```text
[Головне повідомлення]

[Клімат] [Ґрунт] [Зволоження] [Світло] [Вентиляція] [Wi-Fi]

[Температура]
Вгорі / Внизу

[Вологість]
Вгорі / Внизу
```

Головний екран не показує всі органи керування одразу.
Деталі відкриваються кліком по плитці підсистеми.

## UI-only Status Mapping

Цей розділ є UI-only mapping.

```text
UI читає:
system.status
system.status_reason
module.status
module.status_reason
module.state
module.output

UI формує тільки:
текст
колір
іконку
анімацію
рекомендовану дію
```

UI не рахує `system.status`, не змінює `module.status`, не інтерпретує safety і не керує GPIO.

## Базове відображення status -> колір

```text
ok       -> зелений
warning  -> жовтий
error    -> червоний
inactive -> сірий
unknown  -> сірий
```

Ручний режим у UI показується як жовтий, якщо це знижує автоматичність системи, але не є аварією.

## Головне повідомлення system.status -> текст

```text
system.status = ok:
    Все добре

system.status = warning:
    Потрібна увага: <підсистема>

system.status = error:
    Потрібна дія: <підсистема>

system.status = unknown:
    Немає актуальних даних
```

Якщо активних причин кілька, UI показує короткий список підсистем.
Детальний технічний reason показується у відповідній панелі, не в головному повідомленні.

## status_reason -> текст / дія

| status_reason | Текст на деталях | Дія користувача |
|---|---|---|
| `no_water` | Немає води в резервуарі зволожувача. | Долийте воду. |
| `water_unknown` | Стан води невідомий. | Перевірте датчик/роз'єм резервуара. |
| `door_open` | Дверцята відкриті. | Закрийте дверцята для штатної роботи. |
| `sht31_missing` | Один із датчиків клімату не відповідає. | Перевірте підключення датчика. |
| `sht31_stale` | Дані клімату застаріли. | Перевірте підключення датчика/шину. |
| `invalid_value` | Отримано некоректне значення датчика. | Перевірте датчик або калібрування. |
| `rh_delta_high` | Вологість вгорі й внизу сильно відрізняється. | Перевірте циркуляцію повітря. |
| `temperature_delta_high` | Температура вгорі й внизу сильно відрізняється. | Перевірте циркуляцію повітря. |
| `temperature_delta_critical` | Різниця температур критична. | Перевірте вентиляцію та джерела тепла. |
| `sensor_missing` | Очікуваний датчик відсутній. | Перевірте підключення. |
| `sensor_stale` | Дані датчика застаріли. | Перевірте датчик або кабель. |
| `calibration_invalid` | Калібрування некоректне. | Повторіть калібрування. |
| `manual_mode` | Підсистема у ручному режимі. | Поверніть auto, якщо потрібна автономна робота. |
| `confirmation_unavailable` | Немає підтвердження фізичного стану виходу. | Інформаційно; дія не потрібна, якщо підсистема працює. |
| `output_failed` | Вихід не виконав команду. | Перевірте драйвер, живлення, навантаження. |
| `confirmation_failed` | Стан виходу не підтвердився. | Перевірте драйвер або feedback. |
| `wifi_weak` | Слабкий Wi-Fi. | Перемістіть пристрій або точку доступу. |
| `mqtt_reconnecting` | Зв'язок відновлюється. | Зачекайте або перевірте мережу. |
| `mqtt_offline` | Немає зв'язку із сервісом. | Перевірте інтернет/живлення пристрою. |
| `device_offline` | Пристрій не відповідає. | Перевірте живлення пристрою. |

Невідомий `status_reason` показується як:

```text
Потрібна перевірка: <підсистема>
```

## UI-visible labels / vocabulary

Цей розділ визначає український user-visible текст для contract-owned англійських entity names, state keys, settings keys, command names і code identifiers.

Це UI-only presentation mapping. Він не створює нових data fields, не змінює ownership і не переносить логіку з module contracts у UI.

```text
contracts / data model / MQTT registry:
    own English technical names, state keys, setting keys, commands, MQTT names

ui_architecture:
    owns Ukrainian user-visible labels, screen text, action text, and status_reason explanations

UI code:
    implements this vocabulary; it must not invent parallel labels silently
```

### Common labels

| Contract/code term | UI label |
|---|---|
| `status` | Стан |
| `settings` | Налаштування |
| `enabled` | Активний |
| `disabled` | Пасивний |
| `active` | Активний |
| `inactive` | Пасивний |
| `cancel` | Відмінити |
| `close` | Закрити |

### Entity labels

| Contract entity | UI label |
|---|---|
| `system` | Система |
| `climate` | Клімат |
| `pot` | Ґрунт |
| `humidifier` | Зволоження |
| `light` | Світло |
| `fan` | Вентиляція |
| `connection` | Wi-Fi / зв'язок |
| `door` | Двері |

### Climate labels

| Contract/code term | UI label |
|---|---|
| `sensor_0x44` | Верх |
| `sensor_0x45` | Низ |
| `temperature_c` | Температура |
| `humidity_pct` | Вологість |
| `temperature_delta_c` | Різниця температур |
| `rh_delta_pct` | Різниця вологості |
| `temperature_low_warn` | Нижній поріг температури |
| `temperature_high_warn` | Верхній поріг температури |
| `humidity_low_warn` | Нижній поріг вологості |
| `humidity_high_warn` | Верхній поріг вологості |
| `temperature_delta_warn` | Попередження різниці температур |
| `temperature_delta_error` | Критична різниця температур |

### Pot / soil labels

| Contract/code term | UI label |
|---|---|
| `pot[0]` | Вазон 1 |
| `pot[1]` | Вазон 2 |
| `soil_moisture` | Волога |
| `soil_temperature` | Температура |
| `soil_moisture_enabled` | Волога: Активний / Пасивний |
| `soil_temperature_enabled` | Температура: Активний / Пасивний |
| `calibrate_soil_moisture` | Калібрування вологості |
| `calibrate_soil_moisture.point = dry` | Сухо |
| `calibrate_soil_moisture.point = normal` | Норма |
| `calibrate_soil_moisture.point = wet` | Мокро |
| `calibrate_soil_moisture.point = reset` | Скинути |
| `moisture_stale_timeout_sec` | Таймаут вологості |
| `temperature_stale_timeout_sec` | Таймаут температури |
| `temperature_low_warn_c` | Нижній поріг температури |
| `temperature_high_warn_c` | Верхній поріг температури |

### Humidifier labels

| Contract/code term | UI label |
|---|---|
| `water_status` | Вода |
| `water_status = present` | Вода є |
| `water_status = empty` | Немає води |
| `water_status = unknown` | Стан води невідомий |
| `mist_output` | Пар |
| `fan_output` | Локальний вентилятор |
| `mode` | Режим |
| `runtime` | Робота за часом |
| `mist_power_level` | Потужність пару |
| `manual_mist_duration_sec` | Ручний запуск пару |
| `post_fan_sec` | Продув після пару |
| `rh_start` | Старт за вологістю |
| `rh_stop` | Стоп за вологістю |
| `rh_delta` | Гістерезис вологості |

## Плитки підсистем

На огляді є такі плитки:

```text
Клімат
Ґрунт
Зволоження
Світло
Вентиляція
Wi-Fi / зв'язок
```

Вода не є окремою плиткою верхнього рівня.
Вода є частиною `humidifier`.

Вентилятор зволожувача не є окремою плиткою верхнього рівня.
Він показується в деталях `Зволоження`.

## Анімація

```text
Зелена рамка + без анімації = підсистема в нормі, пристрій зараз неактивний
Зелена рамка + анімація     = підсистема в нормі, пристрій зараз активний
Жовта рамка + анімація      = потрібна увага / ручний режим, пристрій активний
```

Анімація показує активність, а не статус.

Приклади:

```text
fan.output = on                  -> вентилятор обертається
humidifier.mist_output = on       -> пар анімується
light.output = on                 -> світло/сонце активне
light.output = off у штатному auto -> місяць або спокійна off-іконка
```

## Клімат

Клімат показує дві зони:

```text
zone0 = Вгорі
zone1 = Внизу
```

UI не показує середнє значення замість двох зон.

На огляді:

```text
Температура
Вгорі 28.1°
Внизу 23.4°

Вологість
Вгорі 61%
Внизу 78%
```

## Ґрунт

На головному екрані ґрунт показується агреговано за найгіршим станом серед вазонів.

```text
усі вазони ok       -> зелений
будь-який warning   -> жовтий
будь-який error     -> червоний
немає даних         -> сірий
```

Деталі по вазонах показуються тільки в панелі `Ґрунт`.

Панель `Ґрунт` показує калібрування вологості для кожного вазона.

```text
Калібрування вологості:
Сухо
Норма
Мокро
Скинути
```

## Зволоження

`Зволоження` відповідає сутності:

```text
humidifier
```

Плитка показує:

```text
humidifier.status
humidifier.status_reason
humidifier.water_status
humidifier.mist_output
humidifier.fan_output
```

Якщо `humidifier.water_status = empty`, головне повідомлення:

```text
Потрібна дія: зволоження
```

Деталі:

```text
Немає води. Долийте воду в резервуар зволожувача.
```

## Світло

Плитка `Світло` показує:

```text
light.status
light.status_reason
light.output
light.settings.mode
```

Час увімкнення/вимкнення належить до панелі `Світло`, не до головного екрана.

UI не описує MQTT namespace для світла.

## Вентиляція

Плитка `Вентиляція` показує основний вентилятор шафи:

```text
fan.status
fan.status_reason
fan.output
fan.settings.mode
```

Нерівномірний клімат не жовтить `Вентиляція` автоматично.
Вентилятор зволожувача належить до `Зволоження`.

## Wi-Fi і зв'язок

У звичайному UI не використовуємо слово MQTT.

На головному екрані зв'язок показується як `Wi-Fi / зв'язок`.

```text
зелений = дані актуальні, зв'язок нормальний
жовтий  = слабкий або нестабільний зв'язок
червоний = сервіс/пристрій недоступний
сірий   = немає актуальних даних
```

Якщо пристрій не відповідає, останній Wi-Fi RSSI не показується як актуальний стан.

## Контекстні панелі

Клік по плитці відкриває панель підсистеми.

Панель показує:

```text
поточний стан
причину
рекомендовану дію
релевантні налаштування
```

Жовті/червоні плитки відкривають причини й дії одразу.

## Шестерня / сервіс

У сервісному меню:

```text
OTA / оновлення прошивки
технічна інформація про пристрій
прихована діагностика
service/developer mode
```

## Deferred UI Features

```text
UART log streaming in Advanced Settings is postponed.

Default UI:
no terminal/log window is shown
no UART log stream is active

Future behavior:
user enables UART log streaming with an explicit button in Advanced Settings
UI opens an additional terminal/log window only after activation
```

MQTT-логи не є частиною звичайного UI.
Якщо вони залишаються, то тільки у developer/service mode.

## Не показуємо у звичайному UI

```text
MQTT-терміни
сирі MQTT-логи
великі графіки за замовчуванням
усі налаштування одразу
RSSI як головне технічне число
окрему плитку води
окрему плитку вентилятора зволожувача
```

## Відкриті питання

```text
1. Точна форма плиток/іконок.
2. Пороги слабкого/нестабільного Wi-Fi.
3. Повний словник status_reason після імплементації firmware.
4. Чи потрібен користувацький журнал подій замість сирих логів.
5. Як відкривати developer/service mode.
```

## Журнал змін

- 2026-06-29: додано UI-фіксацію кнопки `Скинути` для калібрування вологості в панелі `Ґрунт`.
- 2026-06-28: додано ownership rule для українських user-visible labels і первинний UI vocabulary mapping без зміни module contracts.
- 2026-06-27: додано UI-only mapping `status/status_reason -> текст/колір/дія`; MQTT namespace прибрано з UI-документа.
- 2026-06-24, v3-humidifier-ui: UI синхронізовано з V3 сутністю `humidifier`; вода і вентилятор зволожувача не є окремими плитками головного екрана.
