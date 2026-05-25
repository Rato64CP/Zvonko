# ZVONKO v. 1.0 - Technical firmware documentation

This document describes how `ZVONKO v. 1.0` currently behaves in real tower-clock operation: mechanical movement of the hands, rotating plate, bells, and hammers, time synchronization, and safety mechanisms. The focus is on system behavior and design intent rather than copying the implementation line by line.

---

## 1. System overview

### What the system controls
The firmware manages four main tower-clock subsystems:
- **Clock hands**: minute steps through the `EVEN` and `ODD` relays.
- **Rotating plate**: discrete positions representing the mechanical event schedule.
- **Bells**: longer relay activation, including manual switches and automatic plate inputs.
- **Hammers / striking**: short impulses for full hour, half hour, celebration, and funeral mode.

### Physical hardware controlled by the firmware
The field-side execution layer of the tower clock consists of:
- two three-phase `Koncar 0.55 kW / 380 V` motors, one per bell
- microswitches on the rear shaft of each bell motor, used in phase-reversal logic and bell run transitions
- two `310 VDC` electromagnetic hammers, one per bell, with very short impulses of about `0.01 s`
- a clock-hand drive motor with a gear mechanism moved through `EVEN/ODD` impulses lasting about `6 s`
- an electrical cabinet with bell phase-reversal contactors, hammer contactors, fuses, and other protective hardware

The firmware in `main/zvonjenje.*`, `main/otkucavanje.*`, and `main/kazaljke_sata.*` controls the relay and logic layer of the tower clock, while the actual power switching of motors and electromagnetic hammers is handled through contactors and protection inside the electrical cabinet.

### Main runtime loop concept
The main `loop()` is organized as a cooperative non-blocking scheduler:
1. refresh the watchdog
2. process communications and UI (`ESP`, menu, buttons)
3. process mechanics (bells, striking, hands, plate)
4. process additional synchronization (`NTP`)
5. periodically save critical state
6. refresh the watchdog again

This ensures that no subsystem starves and that all of them operate cyclically in small steps.

### High-level architecture
- **Time and synchronization**: [main/time_glob.cpp](../main/time_glob.cpp), [main/esp_serial.cpp](../main/esp_serial.cpp)
- **Mechanical movement**: [main/kazaljke_sata.cpp](../main/kazaljke_sata.cpp), [main/okretna_ploca.cpp](../main/okretna_ploca.cpp), [main/unified_motion_state.cpp](../main/unified_motion_state.cpp)
- **Strikes and bells**: [main/otkucavanje.cpp](../main/otkucavanje.cpp), [main/zvonjenje.cpp](../main/zvonjenje.cpp), [main/slavljenje_mrtvacko.cpp](../main/slavljenje_mrtvacko.cpp)
- **UI and settings**: [main/tipke.cpp](../main/tipke.cpp), [main/menu_system.cpp](../main/menu_system.cpp), [main/postavke.cpp](../main/postavke.cpp)
- **Reliability and recovery**: [main/watchdog.cpp](../main/watchdog.cpp), [main/power_recovery.cpp](../main/power_recovery.cpp), [main/wear_leveling.cpp](../main/wear_leveling.cpp), [main/i2c_eeprom.cpp](../main/i2c_eeprom.cpp)

### Pinout and service connections
The active `Arduino Mega 2560` pin mapping for the tower clock is documented in:
- [Arduino Mega tower clock pinout](arduino_mega_tower_clock_pinout.md)

Critical groups are:

| Group | Pins | Role |
|---|---|---|
| `RTC SQW` | `2` | Main 1 Hz timing reference for hands, rotating plate, and regular striking |
| `433 MHz` | `3` | `SRX882` interrupt input for the remote receiver |
| Local menu buttons | `7-12` | `UP`, `DOWN`, `LEFT`, `RIGHT`, `YES`, `NO` |
| `ESP32` bridge | `14-15` | `Serial3` toward the external network bridge |
| `RS485` | `18-19`, `35` | Active `RS485` transport and DE/RE direction control |
| `I2C` | `20-21` | `DS3231 RTC`, LCD, and external EEPROM/FRAM |
| Hand / plate / bell / hammer relays | `22-29` | Main relay outputs for tower-clock mechanics |
| Plate inputs | `30-34` | Mechanical program plate pins |
| Lamps and mode inputs | `36-47` | Bell lamps, special-mode lamps, silent mode, mains monitor, switches, and night lighting |
| Funeral thumbwheel | `A0`, `A2-A8` | `BCD 1-2-4-8` timer input |
| Solar service buttons and lamps | `A9-A14` | Local `MORNING`, `NOON`, `EVENING` controls and indicators |

---

## 2. Unified state model

### Role of `UnifiedMotionState`
`UnifiedMotionState` is the shared state record for clock hands and the rotating plate. The idea is that both mechanisms share the same model and persistence, so after a restart the system knows:
- where the mechanism stopped
- whether an impulse was active
- which motion phase was in progress

### Field meanings
- `hand_position`: logical hand position in the range `0-719`
- `hand_active`: whether a hand impulse is currently active
- `hand_relay`: which relay is driving the hand impulse (`none`, `EVEN`, `ODD`)
- `plate_position`: current rotating plate position `0-63`
- `plate_phase`: plate phase (`stable`, `first relay`, `second relay`)

### Cache vs EEPROM
The `UnifiedMotionStateStore` layer uses two levels:
- a RAM cache for fast reads without unnecessary `I2C` traffic
- EEPROM for persistence across power loss

Notes:
- the current tower-clock firmware revision intentionally no longer reads older `UnifiedMotionState` layouts, old periodic recovery backups, or earlier user EEPROM containers
- after upgrading to this revision, settings, recovery backups, and helper containers are treated as new and may need to be rewritten through the local menu or web interface

The current `UnifiedMotionState` revision uses **48 rotating slots** in its own EEPROM block. On read, all configured slots are scanned and the newest valid record is selected through an internal sequence number, without shared wear-leveling metadata.

Read flow:
1. try the RAM cache first
2. if the cache is not initialized, read from EEPROM
3. if EEPROM is invalid, reconstruct the initial state for this revision and save it immediately

### When the state is saved
State is saved only when something changes:
- at the start and end of a hand impulse
- when the plate phase changes
- when a movement step completes and position changes
- when positions are adjusted manually

This reduces EEPROM wear while keeping the mechanics consistent.

---

## 3. Clock hands

### Minute step
The hands do not jump directly to the target. Each physical step works like this:
- activate the appropriate relay
- keep it active for about `6 s`
- close the step and increment `hand_position` by `1`

### Even/odd relay model
The relay is chosen from the parity of the current logical position:
- even position -> `EVEN` relay
- odd position -> `ODD` relay

### Finishing an active phase
`RTC SQW` is the primary authority for movement rhythm and phase completion. If `RTC SQW` temporarily disappears, the tower clock immediately drops the relay without applying an extra movement step so an active phase cannot remain stuck.

### How RTC mismatch is corrected
The target is always:
`RTC time -> (hour % 12) * 60 + minute`

If `hand_position != target`:
1. start one step
2. wait for the step to complete
3. compute the target again
4. repeat if still needed

If the hands are slightly ahead, the system does not force a full wraparound. It simply waits for real time to catch up.

---

## 4. Rotating plate

### 15-minute step logic
The target plate position is calculated in `15`-minute blocks. The active daily window is configurable, and the default logic window is:
- from `04:59` to `20:44` as the position range
- pin reading happens one minute later, at `HH:MM:25 + 1 min`

### Two-phase model
A plate step is not instantaneous. It uses two phases:
1. phase 1: first relay active for `6 s`
2. phase 2: second relay active for `6 s`
3. finish: `plate_position = (plate_position + 1) % 64`, phase returns to stable

### Position mapping `0-63`
- `0` corresponds to the start of the daily window
- each following position represents `+15 min`
- `63` is the final or night reference

### Plate pins and bell schedule
The current firmware supports:
- `5` plate-pin positions
- `2` bells
- a dedicated pin for `CELEBRATION`
- no separate active `FUNERAL` plate-pin model in settings anymore

Stored separately for weekdays and Sundays:
- pin schedule for `BELL 1`
- pin schedule for `BELL 2`
- pin for `CELEBRATION`

Value `0` means that the given bell or celebration has no assigned pin.

### When pins are read
Pins are not read while the plate is moving. Reading is allowed only when:
- time is confirmed
- the plate is configured
- the plate is in `STABLE_PHASE`
- the plate is at the target position for the current time slot

Plate inputs use a slower and more robust `75 ms` debounce, because the mechanical pin position changes rarely but must be resilient to contact bounce and noise.

The actual reading moment is shifted to **one minute after the logical slot**, at second `:25`.

Example with the default start:
- slot `04:59` is read at `05:00:25`
- slot `05:14` is read at `05:15:25`
- slot `05:29` is read at `05:30:25`
- slot `05:44` is read at `05:45:25`

The fifth pin is not an extra bell. It is a special trigger for `CELEBRATION`. When active it:
- does not switch bell relays directly
- schedules automatic celebration start
- waits for active bells, inertia, and any ongoing striking to finish before actual start
- may cancel a not-yet-started automatic celebration if the pin is removed before start

### Plate synchronization after power loss
At boot, the system loads the last known state from EEPROM. At runtime:
- if the plate is already at the target, no movement happens
- otherwise, it is corrected step by step until it reaches the target

This avoids aggressive rewinding of the plate mechanism.

---

## 5. Hammer striking

### Full hour and half hour
- full hour (`minute == 00`): `1-12` strikes, male hammer
- half hour (`minute == 30`): one strike, female hammer

### Timing
Regular striking uses:
- hammer impulse duration from settings, limited by the safety cap
- an `RTC SQW` strike schedule every second second (`0, 2, 4, 6...`) while the sequence is running

The start of the hour and half-hour sequence is aligned to the true second boundary through `RTC SQW`, and all following strikes in the same sequence also follow the `SQW` schedule. If `SQW` is not available, regular striking is not performed.

### Special hammer modes
- `celebration 1`, `funeral 1`, and regular striking still use the shared impulse duration from settings
- `celebration 2` uses the fixed sequence: `C1 110 ms -> pause 90 ms -> C2 110 ms -> pause 190 ms`
- `funeral 2` uses the fixed sequence: `C1 300 ms -> pause 700 ms -> C2 300 ms -> pause 3700 ms`

### Local celebration and funeral controls
- `celebration` uses a physical maintained toggle switch
- input `LOW` means celebration should be on
- returning the switch to `HIGH` stops celebration
- if the celebration switch remains continuously active for more than `30 min`, the firmware forces it off and ignores it until the switch is physically returned to `OFF`
- `funeral` remains a separate button and works as a `toggle` on press

### Funeral thumbwheel
Funeral mode uses a two-digit `BCD` thumbwheel `00-99`:
- `00` means run continuously until manual stop
- `01-99` means auto-stop after that many minutes
- the value is stabilized in the background
- if it changes while funeral mode is already active, the new value becomes authoritative immediately and restarts the local countdown

### BAT and quiet hours
`BAT from/to` defines the time range in which regular striking is allowed. Outside that range, the tower-clock firmware blocks only regular striking. It does not block:
- solar automation
- plate-pin-triggered bell ringing

Example:
- `BAT from 6`, `BAT to 22` means striking is allowed from `06:00` to `22:00`
- outside that range, the clock still keeps time, reads the plate pins, and may ring through solar or manual logic, but does not perform regular striking
- for an overnight range such as `22-6`, the transition is intentionally gentle, so `22:00` may still strike and quiet time begins after that

If the morning solar bell event has already run, it may open regular striking even before the normal BAT range begins.

### Silent mode
The unified silent mode blocks:
- bells
- hammers
- celebration
- funeral mode

Hands and the rotating plate remain active.

---

## 6. Bells

### Difference between bells and hammers
- bells: longer relay activation tied to plate inputs, manual switches, and timed automation
- hammers: short impulse strikes for regular striking and special modes

### Manual switch control
There are physical switches for `BELL 1` and `BELL 2`. When a manual override is active, it has priority over automation.

If an individual switch remains continuously on for more than `30 min`, the firmware performs a safety shutoff of that bell and ignores the switch until it is physically returned to `OFF`. This protects the tower clock against a stuck contact or forgotten manual activation.

### Inertia
After turning a bell on or off, inertia becomes active. The value is configured separately for `Bell 1` and `Bell 2` through the `System` menu, and during that period hammer strikes are blocked so mechanical movement does not overlap.

### Bell brake setting `K:0/1`
The `System` menu includes `K:0/1`:
- `K=1` means the bell brake exists and behavior matches earlier firmware generations
- `K=0` means operation without the brake, so the firmware intentionally separates the shutdown of two bells

For synchronized no-brake ending, the firmware uses the real `INR1` and `INR2` settings from [main/postavke.cpp](../main/postavke.cpp) plus a shared `20 s` runtime threshold:
- if both bells have run for less than `20 s`, shutdown remains immediate because the bells may not yet have built their full mechanical inertia
- if both bells have run for at least `20 s`, the relay is released earlier on the bell with the longer inertia so the mechanical ending of both bells is as close as possible to a synchronized stop

When `K=0` and both bells are started by the rotating plate or another time-limited automation:
- the base duration remains the same scheduled duration
- only the bell with the longer inertia has its relay duration shortened, by `abs(INR1 - INR2)`
- a minimum relay runtime of `20 s` is preserved so short or borderline events do not get unrealistically short activation

When `K=0` and the external API sends a shared `GASI_SVE` command while both bells are running:
- if both bells have run for at least `20 s`, the bell with the longer inertia is switched off immediately
- the other bell may remain active for the inertia difference so both bells stop mechanically at nearly the same time
- if the `20 s` threshold has not been reached, both bells are shut down immediately without extension

When `K=0` and the operator turns bells off with individual `OFF` toggles, physical switches, or `433 MHz` buttons:
- the first `OFF` does not have to drop the relay immediately if both bells are active and have already run for at least `20 s`
- the firmware then waits up to `2 s` for the second `OFF` to compensate for the normal human delay between two separate controls
- during that short waiting period, the first requested bell lamp blinks with the same rhythm used for inertia, even though the relay is not yet physically off
- if the second `OFF` arrives within that window, both requests are treated as a coordinated shutdown and the same inertia-difference math is applied
- if the second `OFF` does not arrive in time, the first requested bell is shut down as a normal single-bell stop

Single-bell web/API commands `ZVONO1_OFF` and `ZVONO2_OFF` remain immediate, while coordinated shutdown is still handled through `GASI_SVE`.

### Silent mode and bells
When silent mode is active:
- automatic bell ringing does not run
- manual switches cannot turn bells on
- plate pins may still be read, but cannot start a bell event

---

## 7. NTP / RTC synchronization

### RTC as the primary source
The system continuously reads the `DS3231`, which remains the local time authority during normal operation.

### NTP as a controlled correction
The `ESP` no longer pushes `NTP` on its own schedule. The Mega requests `NTP` only in a safe window when:
- the mechanics are idle
- no sensitive striking or correction moment is active
- the network layer is ready

Whenever possible, accepted `NTP` synchronization is aligned to the next `RTC SQW` second tick before being applied.

### Protection against suspicious time jumps
If new time would create an excessive jump, the system does not accept it immediately and requests extra confirmation. This avoids writing clearly wrong time into the `RTC`.

### DST
The firmware manages `CET/CEST` state itself, stores it in EEPROM, and applies the transition automatically.

---

## 8. Menu and settings system

### Clear separation of responsibilities
- [main/tipke.cpp](../main/tipke.cpp): physical button scanning and conversion into `KeyEvent`
- [main/menu_system.cpp](../main/menu_system.cpp): UI state, screens, navigation, and calls into business logic
- [main/postavke.cpp](../main/postavke.cpp): persistent storage, validation, fallback to defaults, and EEPROM writes

Local buttons no longer use a matrix keypad. The active tower-clock firmware now uses 6 direct `INPUT_PULLUP` inputs:
- `UP`
- `DOWN`
- `LEFT`
- `RIGHT`
- `YES`
- `NO`

### How settings are changed and saved
1. the operator changes a value through the menu
2. `menu_system` calls an API from `postavke`
3. `postavke` validates it, prepares integrity data, and writes it
4. the change is saved to EEPROM

Button behavior is unified:
- arrows are used for navigation and value changes
- `Ent` saves changes for the active menu branch
- `Esc` exits without saving and goes one level back
- numeric keys are no longer part of the active menu flow; corrections of hand position and other values are done through `UP/DOWN`

The `Sun` menu currently edits:
- morning event
- noon event
- evening event
- night lighting as the fourth page after `Morning`, `Noon`, and `Evening`

Night lighting is separate from bells, but uses the same sunrise/sunset calculations:
- it turns on `PIN_RELEJ_NOCNE_RASVJETE` at the evening event
- it turns it off at the morning event
- it is `OFF` by day and `ON` by night
- in the `Sun` menu, `Up/Down` on the night-light page changes `AUTO`, while `Left/Right` moves to a neighboring page

The `Feasts` menu edits automatic celebration after solar events:
- `SLAVI J0 P0 V0` defines whether celebration is allowed after the morning Hail Mary, noon bell, and evening Hail Mary
- `A:0 P:0 VG:0` enables the seasonal ranges for St. Anthony, St. Peter, and Assumption
- St. Anthony applies from `6 June` through `13 June`
- St. Peter applies from `22 June` through `28 June`
- Assumption applies from `8 August` through `15 August`
- celebration is scheduled only after a solar bell event actually starts
- before celebration starts, the system waits for bells, striking, and inertia to finish
- celebration duration and delay use the existing `Stapici` settings

The second `Feasts` page edits the All Saints / All Souls schedule:
- `SVI SVETI 0/1` enables or disables the special funeral schedule
- `P:15` is the starting hour for funeral mode on `1 Nov`
- `Z:8` is the ending hour for funeral mode on `2 Nov`
- when enabled, funeral mode runs on `1 Nov` from `P:00` until `21:00`
- after that there is silence until `2 Nov` at `06:00`
- funeral mode runs again on `2 Nov` from `06:00` until `Z:00`
- evening Hail Mary is skipped on `1 Nov`
- morning Hail Mary is skipped on `2 Nov`
- the All Saints schedule intentionally ignores the thumbwheel auto-stop, because duration is defined by the calendar schedule

The `System` menu is condensed into two LCD pages to reduce navigation overhead:
- page `Misc` shows `LCD`, `LOG`, `RS` on the first row and `UPS` and `BRAKE` on the second
- page `Impulse / inertia` shows `IN1`, `IN2`, and `IMPULSE`

Within that layout:
- `LEFT/RIGHT` moves the active field through all 8 system settings
- `UP/DOWN` changes the active value
- `YES` saves all changes
- `NO` exits without saving

Covered settings:
- `LCD light`
- `Logging`
- `RS485`
- `UPS mode`
- `K` - bell brake usage
- `INR1` - rundown time for `Bell 1`
- `INR2` - rundown time for `Bell 2`
- `Hammer impulse`

`UPS mode` uses a dedicated mains-monitoring input. When enabled and the `Mega` reports mains loss, the tower-clock firmware:
- blocks bells through [main/zvonjenje.cpp](../main/zvonjenje.cpp)
- blocks striking and other hammer sequences through [main/otkucavanje.cpp](../main/otkucavanje.cpp) and [main/slavljenje_mrtvacko.cpp](../main/slavljenje_mrtvacko.cpp)
- blocks automatic hand motion through [main/kazaljke_sata.cpp](../main/kazaljke_sata.cpp)
- keeps the rotating plate active, but disables plate-triggered bell ringing and special modes in [main/okretna_ploca.cpp](../main/okretna_ploca.cpp)
- turns on the silent-mode lamp and displays `NEMA STRUJE!` instead of the date on the main LCD

When mains power returns, these blocks are removed and the hands resume normal automatic alignment to real time.

### Main LCD details
The main LCD presentation is aligned with current firmware behavior:
- the time-source marker shows `---` instead of the old `RTC`
- labels `NTP`, `MAN`, `ERR`, and `---` occupy fields `11-13` of the first row
- the last two fields of the first row show the `DS3231` temperature in short two-digit form
- the activity `*`, `R/N`, and WiFi `W` markers are no longer displayed
- the time colon blinks in `1/2 SQW` rhythm only when [main/zvonjenje.cpp](../main/zvonjenje.cpp), [main/otkucavanje.cpp](../main/otkucavanje.cpp), [main/kazaljke_sata.cpp](../main/kazaljke_sata.cpp), or [main/okretna_ploca.cpp](../main/okretna_ploca.cpp) is currently active, or when WiFi is disconnected
- if WiFi is connected and the tower-clock mechanics are idle, the colon stays steadily lit
- until time is confirmed, the first row remains in safe `ERR` presentation and does not display unverified `RTC` time as if it were valid
- during `UPS mode` without mains power, the lower row displays `NEMA STRUJE!` instead of the date
- day names are shown in uppercase
- Monday is abbreviated as `PON.` so the date still fits on a `16x2` display
- Thursday uses a custom LCD character for `Č`, and the same character is used in the `MRTVAČKO` message

### EEPROM versioning and validation
Settings use triple protection:
- signature
- layout version
- full-structure checksum

If validation fails, the system returns to defaults and writes them again.

The newer `UPS mode` and `K:0/1` flags do not require a new EEPROM layout. They reuse free bits in the existing settings field, so older systems keep a compatible boot path and default behavior:
- an older record without the new flag still means `K=1`
- `UPS mode` remains explicitly opt-in

---

## 9. Boot sequence

### Initialization order
`setup()` initializes in this order:
1. LCD and PC serial
2. external EEPROM
3. RTC and settings load
4. buttons, ESP bridge, and menu
5. bells, striking, hands, and plate
6. watchdog
7. power-recovery markers and boot recovery

### Watchdog initialization
The watchdog is configured with an `8 s` timeout and immediately records the previous reset reason (`WDT`, `BOR`, `POR`, `EXTRF`).

### Recovery after power loss
Power recovery reads the rotating set of backup slots and searches for the newest valid record. If found, it restores:
- hand position
- plate position
- any interrupted active step is returned to an inactive state at the same position, so the physical step will be executed again after restart

The plate `offset` is no longer part of the active recovery model.

### Restoring state from EEPROM
Critical state is periodically saved every `60 s` into rotating slots. This limits state loss to at most the last minute before a failure.

### Initial synchronization behavior
After boot, the system does not perform a hard jump of the mechanics. Instead, mechanisms enter the normal step-by-step mode and converge toward the correct time and plate position.

---

## 10. Error handling and safety

### Watchdog
`wdt_reset()` is called twice during the main loop. If the loop stalls, the `MCU` resets and the reset reason remains available for recovery diagnostics.

### EEPROM validation
- settings: signature + version + checksum
- critical-state backup: checksum
- unified motion state: field ranges + version + sequence

### Handling corrupted settings
If stored data is invalid:
- safe defaults are restored
- string fields are sanitized and `null`-terminated
- the corrected structure is written again

### Preventing invalid states
- values are clamped to valid ranges
- mutually exclusive modes are enforced (`celebration` vs `funeral`)
- parallel striking sequences are not allowed
- relays are safely shut down during interruption and initialization
- `UPS mode` can keep tower-clock logic alive on backup power, but without outputs to hands, bells, or hammers while mains is absent

---

## 11. Known design decisions

### Why correction is step by step
Tower-clock mechanics have mass and inertia. Gradual correction reduces impact load, relay heating, and the risk of mechanical overshoot.

### Why unified state is used
A single source of truth for hands and plate reduces race-condition scenarios and simplifies recovery because both drives recover from the same state concept.

### Why blocking boot recovery is allowed
A short deterministic delay during boot is acceptable because it is more important to return the mechanics to a consistent state before entering normal cyclic operation.

### Why the Mega chooses the NTP moment
This avoids unnecessary micro-correction in the middle of an active mechanical cycle and preserves the rhythm of the tower clock.

---

## Developer notes

- [main/wear_leveling.cpp](../main/wear_leveling.cpp) still exists for slower EEPROM segments, but the main hand and plate state is now managed by [main/unified_motion_state.cpp](../main/unified_motion_state.cpp)
- [main/okretna_ploca.cpp](../main/okretna_ploca.cpp) expects the active daily window to remain aligned to `15`-minute blocks
- when changing layout, always review [main/eeprom_konstante.h](../main/eeprom_konstante.h), [main/unified_motion_state.cpp](../main/unified_motion_state.cpp), and [main/power_recovery.cpp](../main/power_recovery.cpp) together
- manual bell override has priority over automation; when diagnosing why a bell does not stop, first check physical switches and silent mode
- the latest SRAM tuning moved most fixed log strings to `snprintf_P(..., PSTR(...))`, reduced large buffers in `main/esp_serial.cpp`, and lowered global SRAM usage to about `41%`, leaving about `4815 B` of free reserve for local buffers and stack
