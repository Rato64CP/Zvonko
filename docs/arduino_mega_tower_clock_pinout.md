# ZVONKO v. 1.0 - Arduino Mega tower clock pinout

This document is a readable overview of all active pins and connections for the `Arduino Mega 2560` used in `ZVONKO v. 1.0`. The single source of truth remains [podesavanja_piny.h](../main/podesavanja_piny.h), while this file is meant as a service and installation reference.

## Quick overview of all active connections

This table gives a service-level overview of all currently active `Arduino Mega 2560` connections, not only outputs.

| Pin | Function | Direction | Type / subsystem | Active state | Note |
|---:|---|---|---|---|---|
| `2` | `RTC SQW` | Input | Time synchronization | `1 Hz` pulse | `DS3231 SQW` reference clock |
| `3` | `433 MHz DATA` | Input | Remote receiver | pulse signal | `SRX882` data output |
| `7` | `UP` button | Input | Local menu | `LOW` | `INPUT_PULLUP` |
| `8` | `DOWN` button | Input | Local menu | `LOW` | `INPUT_PULLUP` |
| `9` | `LEFT` button | Input | Local menu | `LOW` | `INPUT_PULLUP` |
| `10` | `RIGHT` button | Input | Local menu | `LOW` | `INPUT_PULLUP` |
| `11` | `YES` button | Input | Local menu | `LOW` | `INPUT_PULLUP` |
| `12` | `NO` button | Input | Local menu | `LOW` | `INPUT_PULLUP` |
| `14` | `TX3` | Bidirectional | `ESP32` serial bridge | UART | `Serial3` toward `ESP32` |
| `15` | `RX3` | Bidirectional | `ESP32` serial bridge | UART | `Serial3` toward `ESP32` |
| `18` | `TX1` | Bidirectional | `RS485` | UART | `Serial1` for the `RS485` layer |
| `19` | `RX1` | Bidirectional | `RS485` | UART | `Serial1` for the `RS485` layer |
| `20` | `SDA` | Bidirectional | `I2C` | bus | `LCD`, `DS3231`, EEPROM/FRAM |
| `21` | `SCL` | Bidirectional | `I2C` | bus | `LCD`, `DS3231`, EEPROM/FRAM |
| `22` | Even hand relay | Output | Relay | `HIGH` | Clock-hand impulse, even step |
| `23` | Odd hand relay | Output | Relay | `HIGH` | Clock-hand impulse, odd step |
| `24` | Even plate relay | Output | Relay | `HIGH` | First phase of plate movement |
| `25` | Odd plate relay | Output | Relay | `HIGH` | Second phase of plate movement |
| `26` | Bell 1 | Output | Relay | `HIGH` | Bell 1, automatic or manual |
| `27` | Bell 2 | Output | Relay | `HIGH` | Bell 2, automatic or manual |
| `28` | Hammer 1 - male | Output | Relay / hammer | `HIGH` | Full hour, celebration, and funeral mode |
| `29` | Hammer 2 - female | Output | Relay / hammer | `HIGH` | Half hour, quarter-hour, and special modes |
| `30` | Plate pin 1 | Input | Rotating plate | via contact | Plate input |
| `31` | Plate pin 2 | Input | Rotating plate | via contact | Plate input |
| `32` | Plate pin 3 | Input | Rotating plate | via contact | Plate input |
| `33` | Plate pin 4 | Input | Rotating plate | via contact | Plate input |
| `34` | Plate pin 5 | Input | Rotating plate | via contact | Plate input |
| `35` | `RS485 DE/RE` | Output | `RS485` control | `HIGH=TX` | Transceiver direction |
| `36` | Bell 1 indicator | Output | LED | `HIGH` | Activity or signaled rundown |
| `37` | Bell 2 indicator | Output | LED | `HIGH` | Activity or signaled rundown |
| `38` | Celebration indicator | Output | LED | `HIGH` / blinking | Celebration or thermal pause |
| `39` | Funeral indicator | Output | LED | `HIGH` / blinking | Funeral mode or waiting state |
| `40` | Mains monitor | Input | `UPS` | `LOW=mains` | `HIGH` means UPS-only operation |
| `41` | Silent-mode switch | Input | Silent mode | `LOW=ON` | `INPUT_PULLUP` |
| `42` | Funeral button | Input | Special mode | `LOW` | Momentary `toggle` input |
| `43` | Celebration switch | Input | Special mode | `LOW=ON` | `INPUT_PULLUP` |
| `44` | Manual bell 1 switch | Input | Manual override | `LOW=ON` | `INPUT_PULLUP` |
| `45` | Manual bell 2 switch | Input | Manual override | `LOW=ON` | `INPUT_PULLUP` |
| `46` | Silent-mode indicator | Output | LED | `HIGH` | Final effective silent mode is active |
| `47` | Night-light relay | Output | Relay | `HIGH` | Enabled at night by solar automation |
| `A0` | Thumbwheel tens `1` | Input | Funeral timer | `LOW` to `GND` | `BCD 1` |
| `A2` | Thumbwheel tens `2` | Input | Funeral timer | `LOW` to `GND` | `BCD 2` |
| `A3` | Thumbwheel tens `4` | Input | Funeral timer | `LOW` to `GND` | `BCD 4` |
| `A4` | Thumbwheel tens `8` | Input | Funeral timer | `LOW` to `GND` | `BCD 8` |
| `A5` | Thumbwheel units `8` | Input | Funeral timer | `LOW` to `GND` | `BCD 8` |
| `A6` | Thumbwheel units `4` | Input | Funeral timer | `LOW` to `GND` | `BCD 4` |
| `A7` | Thumbwheel units `2` | Input | Funeral timer | `LOW` to `GND` | `BCD 2` |
| `A8` | Thumbwheel units `1` | Input | Funeral timer | `LOW` to `GND` | `BCD 1` |
| `A9` | Evening sun button | Input | Solar automation | `LOW` | Momentary service input |
| `A10` | Evening sun indicator | Output | LED | `HIGH` | Evening automation state |
| `A11` | Morning sun button | Input | Solar automation | `LOW` | Momentary service input |
| `A12` | Morning sun indicator | Output | LED | `HIGH` | Morning automation state |
| `A13` | Noon sun button | Input | Solar automation | `LOW` | Momentary service input |
| `A14` | Noon sun indicator | Output | LED | `HIGH` | Noon automation state |

Note:
- `A1` is currently not part of the active tower-clock mapping
- pins `5`, `6`, `16`, and `17` are currently unused by the active firmware

## Hand relays

| Function | Pin | Note |
|---|---:|---|
| Even hand relay | `22` | First phase of the hand impulse |
| Odd hand relay | `23` | Second phase of the hand impulse |

## Rotating plate relays

| Function | Pin | Note |
|---|---:|---|
| Even plate relay | `24` | First phase of rotating plate movement |
| Odd plate relay | `25` | Second phase of rotating plate movement |

## Bells and hammers

| Function | Pin | Note |
|---|---:|---|
| Bell 1 | `26` | Bell 1 relay |
| Bell 2 | `27` | Bell 2 relay |
| Hammer 1 - male | `28` | Hour striking and special modes |
| Hammer 2 - female | `29` | Half-hour, quarter-hour, and special modes |

## Night lighting

| Function | Pin | Note |
|---|---:|---|
| Night-light relay | `47` | `HIGH = night lighting ON` |

## Rotating plate inputs

| Function | Pin | Note |
|---|---:|---|
| Plate pin 1 | `30` | Plate input |
| Plate pin 2 | `31` | Plate input |
| Plate pin 3 | `32` | Plate input |
| Plate pin 4 | `33` | Plate input |
| Plate pin 5 | `34` | Plate input |

## Time synchronization

| Function | Pin | Note |
|---|---:|---|
| RTC SQW 1 Hz | `2` | `DS3231 SQW` timing pulse |

## I2C bus

| Function | Pin | Note |
|---|---:|---|
| SDA | `20` | `DS3231 RTC`, external EEPROM/FRAM, and LCD |
| SCL | `21` | `DS3231 RTC`, external EEPROM/FRAM, and LCD |

## Local menu buttons

All local menu buttons use `INPUT_PULLUP` and become active when connected to `GND`.

| Function | Pin | Note |
|---|---:|---|
| UP | `7` | Up navigation |
| DOWN | `8` | Down navigation |
| LEFT | `9` | Left navigation |
| RIGHT | `10` | Right navigation |
| YES | `11` | Confirm / `SELECT` |
| NO | `12` | Back / `BACK` |

Note:
- the old matrix keypad is no longer part of the active tower-clock firmware path
- the `433 MHz` `SRX882` receiver uses pin `3` as its only data input
- pins `5`, `16`, and `17` remain free for future expansion

## Solar service buttons and indicators

Local service buttons `MORNING`, `NOON`, and `EVENING` are momentary inputs and do not use a maintained toggle logic.

| Function | Pin | Note |
|---|---:|---|
| Evening sun button | `A9` | Momentary input for evening automation |
| Evening sun indicator | `A10` | Evening automation state |
| Morning sun button | `A11` | Momentary input for morning automation |
| Morning sun indicator | `A12` | Morning automation state |
| Noon sun button | `A13` | Momentary input for noon automation |
| Noon sun indicator | `A14` | Noon automation state |

## 433 MHz remote receiver

The design expects a raw `433 MHz` `SRX882` receiver with a single data output.

| Function | Pin | Note |
|---|---:|---|
| 433 `DATA` receiver output | `3` | `SRX882` data signal toward the `Mega 2560` interrupt input |

Note:
- learned button codes are processed in [main/daljinski_433.cpp](../main/daljinski_433.cpp)
- buttons `A`, `B`, `C`, and `D` can trigger bells and celebration according to the active firmware logic

## Special buttons and switches

| Function | Pin | Note |
|---|---:|---|
| Celebration toggle switch | `43` | `LOW = celebration ON` |
| Funeral button | `42` | Press = `toggle` |
| Silent-mode toggle switch | `41` | `LOW = silent mode ON` |
| Manual bell 1 switch | `44` | `LOW = ON` |
| Manual bell 2 switch | `45` | `LOW = ON` |

## Mains monitoring for UPS mode

| Function | Pin | Note |
|---|---:|---|
| Mains voltage monitor | `40` | `INPUT_PULLUP`, `LOW = mains present`, `HIGH = UPS only` |

Note:
- the input is intended for an optocoupler with an open-collector output toward `GND`
- while `UPS mode` is enabled and this pin reports loss of mains power, the firmware blocks bells, hammers, and hands
- the rotating plate remains active so the tower clock can still track the mechanical schedule

## Indicator lamps

| Function | Pin | Note |
|---|---:|---|
| Bell 1 indicator | `36` | `HIGH = on`, blinks during inertia or synchronized rundown |
| Bell 2 indicator | `37` | `HIGH = on`, blinks during inertia or synchronized rundown |
| Celebration indicator | `38` | `HIGH = on`, blinks during thermal pause |
| Funeral indicator | `39` | `HIGH = on`, blinks while waiting for completion |
| Silent-mode indicator | `46` | `HIGH = on` |

## Funeral thumbwheel timer

The firmware uses two `BCD 1-2-4-8` digits with `INPUT_PULLUP` logic.

### Tens

| BCD bit | Pin | Note |
|---|---:|---|
| `1` | `A0` | Tens bit 0 |
| `2` | `A2` | Tens bit 1 |
| `4` | `A3` | Tens bit 2 |
| `8` | `A4` | Tens bit 3 |

### Units

| BCD bit | Pin | Note |
|---|---:|---|
| `1` | `A8` | Units bit 0 |
| `2` | `A7` | Units bit 1 |
| `4` | `A6` | Units bit 2 |
| `8` | `A5` | Units bit 3 |

Note:
- the thumbwheel closes toward `GND`
- each digit is read as `BCD 1-2-4-8`
- `A1` is currently not used by the active thumbwheel mapping

## Serial communication

| Port | Pins | Role |
|---|---|---|
| `Serial` | USB | PC logging and diagnostics (`115200`) |
| `Serial1` | `RX1=19`, `TX1=18` | Active tower-clock `RS485` transport (`9600`) |
| `Serial3` | `RX3=15`, `TX3=14` | External `ESP32` network bridge (`9600`) |

Current firmware assignments:
- `ESP_SERIJSKI_PORT = Serial3`
- `RS485_SERIJSKI_PORT = Serial1`

## Short range summary

- `2` -> `RTC SQW`
- `3` -> `433 MHz SRX882 DATA`
- `7-12` -> 6 direct local menu buttons
- `14-15` -> `Serial3` to the external `ESP32`
- `16-17` -> free
- `18-19` -> `Serial1` for active `RS485`
- `20-21` -> `I2C`
- `22-29` -> hand, plate, bell, and hammer relays
- `30-34` -> plate inputs
- `35-39` -> `RS485` direction and indicator lamps
- `40-45` -> `UPS`, silent mode, and physical switches
- `46-47` -> silent-mode lamp and night lighting
- `A0`, `A2-A8` -> funeral thumbwheel
- `A9-A14` -> solar service buttons and indicators

## Development note

If the pin mapping ever changes, update [podesavanja_piny.h](../main/podesavanja_piny.h) first, and only then update this documentation and the related service notes.
