# Vazon V3 Board Pinout

Document status: active draft
Code status: hardware reference
Scope: Vazon V3 new PCB pinout

## 1. Purpose

This is the active V3 board pinout.

It contains only pins used by the V3 board.

## 2. Board

```text
MCU/module: ESP32-WROVER
SPI flash: 16 MB
Board revision: PCB v1.0
Physical pin numbers: recorded in this file
```

## 3. Pinout

| Pin No. | GPIO | Function | Direction | Electrical / active level | Firmware name |
|---:|---:|---|---|---|---|
| 25 | GPIO0 | Provisioning / factory reset button | input | pull-up, pressed = low; boot strap pin | `PROVISIONING_BUTTON_GPIO` |
| 34 | GPIO1 | UART0 TXD | output | UART0 programming/log TX | `UART0_TXD_GPIO` |
| 35 | GPIO3 | UART0 RXD | input | UART0 programming/log RX | `UART0_RXD_GPIO` |
| 26 | GPIO4 | Light output | output | active-high switched output; `0` = OFF, `1` = ON; safe state = LOW | `LIGHT_GPIO` |
| 29 | GPIO5 | Status LED green | output | active-low LED; `0` = ON, `1` = OFF | `STATUS_LED_GREEN_GPIO` |
| 16 | GPIO13 | Main fan output | output | active-high switched output; `0` = OFF, `1` = ON; safe state = LOW | `MAIN_FAN_GPIO` |
| 23 | GPIO15 | Status LED red | output | active-low LED; `0` = ON, `1` = OFF; boot strap pin | `STATUS_LED_RED_GPIO` |
| 31 | GPIO19 | Humidifier local fan output | output | active-high switched output; `0` = OFF, `1` = ON; safe state = LOW | `HUMIDIFIER_FAN_GPIO` |
| 33 | GPIO21 | Climate I2C SDA | bidirectional | I2C SDA | `CLIMATE_I2C_SDA_GPIO` |
| 36 | GPIO22 | Climate I2C SCL | bidirectional | I2C SCL | `CLIMATE_I2C_SCL_GPIO` |
| 11 | GPIO26 | Humidifier mist control | output | active-high switched output; `0` = OFF, `1` = ON; safe state = LOW | `HUMIDIFIER_MIST_GPIO` |
| 8 | GPIO32 | Pot temperature OneWire | bidirectional | OneWire data; external 4.7 kOhm pull-up required | `POT_TEMP_ONEWIRE_GPIO` |
| 9 | GPIO33 | Door reed input | input | pull-up input; active-low door signal | `DOOR_REED_GPIO` |
| 7 | GPIO35 | Humidifier water present sensor | input-only | external pull-up; reed closed = low = water empty; reed open = high = water present; GPIO35 has no internal pull-up | `HUMIDIFIER_WATER_PRESENT_GPIO` |
| 4 | GPIO36 | Pot 0 soil moisture ADC | analog input | ADC1_CH0, input-only | `POT0_SOIL_MOISTURE_ADC_GPIO` |
| 5 | GPIO39 | Pot 1 soil moisture ADC | analog input | ADC1_CH3, input-only | `POT1_SOIL_MOISTURE_ADC_GPIO` |

## 4. UART0

UART0 is reserved for programming and serial logs.

| Pin No. | GPIO | Signal | Direction |
|---:|---:|---|---|
| 34 | GPIO1 | TXD0 | output |
| 35 | GPIO3 | RXD0 | input |

## 5. Bus Notes

| Bus | Pins | Notes |
|---|---|---|
| Climate I2C | GPIO21 SDA, GPIO22 SCL | Two SHT31 sensors. Address handling belongs to `climate.contract.md`. |
| Pot temperature OneWire | GPIO32 | External 4.7 kOhm pull-up required. Current prototype uses DS18B20 enumeration index; pot binding belongs to `pot.contract.md`. |
| Pot soil moisture ADC | GPIO36, GPIO39 | ADC1 inputs. Pot mapping belongs to `pot.contract.md`. |

## 6. Output Safe State

All actuator outputs are active-high and must initialize LOW (OFF):

```text
GPIO4  LIGHT_GPIO            OFF = 0
GPIO13 MAIN_FAN_GPIO         OFF = 0
GPIO19 HUMIDIFIER_FAN_GPIO   OFF = 0
GPIO26 HUMIDIFIER_MIST_GPIO  OFF = 0
```

## 7. Remaining Hardware Confirmations

```text
None.
```
