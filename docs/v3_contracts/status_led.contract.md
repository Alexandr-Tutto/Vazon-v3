# Status LED Contract

Document status: draft
Code status: architecture contract
Scope: Vazon V3 Status LED

## 1. Purpose

```text
Status LED показує базовий стан пристрою фізично на корпусі.
```

## 2. Role

```text
Status LED є System Service Module.

Його роль — фізично відобразити вже визначений стан системи.
```

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
connection state
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
Status LED Module володіє тільки LED outputs.
```

## 7. Priority

```text
LED indication priority:

1. provisioning active
2. system.error
3. local warning / attention
4. mqtt offline
5. wi-fi offline
6. normal ok
```

## 8. LED States

```text
green short blink, long pause — normal ok

green 50/50 blink             — provisioning AP active

red short blink, long pause   — wi-fi offline

red 50/50 blink               — mqtt offline

red steady                    — system error

red/green alternate blink     — local warning / attention
```

## 9. Forbidden

```text
Status LED не рахує system.status.
Status LED не змінює GlobalContext.
Status LED не змінює module state.
Status LED не керує climate actuators.
```
