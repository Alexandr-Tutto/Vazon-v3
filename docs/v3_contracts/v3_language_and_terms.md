# V3 language and terminology rules

Document status: rule
Code status: documentation rule
Repository scope: clean Vazon V3 workspace
Status note: active V3 documentation rule

## 1. Purpose

This document fixes language rules for Vazon V3 documentation, firmware naming, UI text and chat explanations.

## 2. Main rule

Use English technical names where they are standard in software development, firmware, MQTT and code architecture.

Project documentation should stay concise and should not include educational explanations for every standard software term.

Short Ukrainian explanations of standard software terms are requested in chat discussion, not inside normal project documents.

## 3. Documentation language

Project documentation may use mixed Ukrainian + English technical terms.

Preferred documentation style:

```text
Український робочий текст + англійський технічний термін, якщо він є стандартним для коду, MQTT або архітектури.
```

Example:

```text
Створити AppState як центральний стан PWA.
```

Do not artificially translate stable code/architecture names such as:

```text
AppState
Reducer
Event
Effect
MQTT topic
payload
retained message
pending_command
status_reason
```

Documentation should not become a glossary unless the document itself is specifically a terminology/glossary document.

## 4. Chat explanation rule

In chat, when a standard software term appears and may be unclear, add a very short explanation.

Format:

```text
Term — коротке пояснення.
```

Examples:

```text
Backlog — список задач.
Bring-up — первинний запуск і перевірка нового заліза/софту.
Reducer — функція, що з поточного стану і події формує новий стан.
Event — подія, яку обробляє логіка.
Effect — зовнішня дія після зміни стану: MQTT, таймер, команда залізу.
Payload — дані всередині MQTT-повідомлення.
Retained message — останнє повідомлення, збережене брокером.
```

If the user explicitly marks a term as understood, do not keep explaining it repeatedly in chat.

## 5. Firmware/code naming

Firmware, source code, MQTT topics, internal state names and configuration keys use English technical names.

Examples:

```text
system/status
climate/air_temp0
humidifier/water/status
fan/manual/duration/set
AppState
Reducer
Event
Effect
pending_command
status_reason
```

Do not translate code identifiers to Ukrainian.

Reason:

```text
English identifiers are easier to maintain, search, document and use with existing libraries/tools.
```

## 6. UI language

All user-visible UI output must be Ukrainian.

This includes:

```text
screen titles
buttons
status messages
warnings
errors
settings labels
help text
validation messages
confirmation messages
```

Examples:

```text
Аварія — Зволоження
Перевірити зволожувач
Світло
Вентиляція
Зберегти
Скасувати
Налаштування
Розширені налаштування
```

Do not show raw internal English identifiers in normal UI.

Allowed exception:

```text
advanced/debug/service screens may show raw MQTT topics or internal names when explicitly marked as diagnostics
```

## 7. Known terms

Terms marked by the user as already clear should be treated as known and normally should not be re-explained in chat.

Current known-term list:

```text
not filled yet
```
