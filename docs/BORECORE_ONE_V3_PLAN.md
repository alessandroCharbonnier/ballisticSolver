# BoreCore One v3: design decisions and build plan

Status: plan only. No code has been written for v3. This file records the outcome of a design review held on 2026-11-27 and is the source of truth for v3 of the rifle-mounted unit. Where it conflicts with `README.md` or existing firmware, this file describes the target and the others describe the present.

Audience: the developer or coding agent who will build v3. Read sections 1 to 6 before touching anything, then work through section 9 in order.

## 1. Purpose

The BoreCore One exists to stop a shooter from getting a wrong firing solution through a setup mistake: wrong rifle or ammunition selected, wrong muzzle velocity, stale or imprecise weather, forgotten Coriolis inputs, forgotten shot angle, a zero that no longer matches the conditions.

Three principles follow, and every rule below serves one of them:

1. **Measure instead of asking.** Whatever a sensor can supply is never typed.
2. **Works with or without a phone.** The phone (BoreCore Link) makes things faster. It is never required for a correct solution.
3. **Nothing is silently stale.** Any value that was typed or pushed, and could be out of date, shows where it came from and how old it is.

Range envelope: .22LR at short range up to ELR at 2 miles.

Intended users of v1: the author and a few trusted testers. The foundations must not need redoing if it later becomes a product (section 7.6).

## 2. Decisions

| # | Topic | Decision |
|---|-------|----------|
| 1 | Rifle identity | One unit is permanently bound to one rifle. Rifle parameters are set once at install. There is no rifle profile selection. |
| 2 | Phone | Optional. The One must give a correct solution with no phone present. |
| 3 | Range envelope | Everything up to ELR (2 miles). |
| 4 | Ammunition | A named load list on the One (up to 8), one active. Loads are created in Link and pushed; they then work phone-less. The active load name is always on the HUD. |
| 5 | MV and BC | Editable on the One at any time, or pushed from Link (ammunition grouped by predicted MV). |
| 6 | MV/BC guard | Source and age tag on the HUD only: `PHONE` when the value came from the phone, `MAN` once the user edits it. Plus an "expected MV" reference line. |
| 7 | MV and temperature | Each MV is stored with its reference temperature. One slope per load (m/s per degree C), default 0 until measured data gives a number. |
| 8 | Pressure | Barometer on the One. Always live. |
| 9 | Temperature, humidity | From the BoreCore Air dongle. A warning is shown when the Air is not connected. Manual values can be edited at any time and are replaced by the next Air reading. |
| 10 | Latitude and clock | Power-gated GNSS on the One: one fix per wake from Off, then powered down. |
| 11 | Azimuth | Manual on the One, or pushed from the phone. Source and age tag only. |
| 12 | Wind | An abstract wind source. Manual speed + clock direction now; a BLE anemometer (Calypso, model undecided) later. The anemometer's reference mark is pointed at the target, so direction is relative to the bore. |
| 13 | Shot angle | Button capture: aim at the target, press. Valid on the assumption that the turrets are at 0 clicks. |
| 14 | Zero | A stored snapshot per load, taken with a "Set zero" action. |
| 15 | Distance | Live dial and staged targets. Stages are editable on the One. No rangefinder link in v1. |
| 16 | Display | Sharp Memory LCD 2.7" 400x240 (LS027B7DH01 class). |
| 17 | MCU | ESP32-S3-WROOM-1-N16R8 (16 MB flash, 8 MB PSRAM), subject to the solver benchmark gate (section 9, step A2). |
| 18 | Battery | LiPo pouch, about 2000 mAh, USB-C charging. |
| 19 | Wake and sleep | Nap (automatic, wakes on motion) and Off (deliberate, wakes on button), with a safety net. |
| 20 | Layout | Two pieces joined by one short cable: display body on the rifle, 5-way input on the thumb rest. |
| 21 | Input device | Metal-dome D-pad (5 snap domes on a thin PCB under a silicone cap). |
| 22 | Phone link | BLE bonding with a passkey shown on the One. Advertising is automatic while awake. Config requires an encrypted bonded link. |
| 23 | Build order | S3 breadboard with all new parts first, then custom PCB and enclosure. No intermediate step on the old hardware. |

### Considered and rejected

Do not reintroduce these without a new decision.

| Idea | Why rejected |
|------|--------------|
| NFC tags on ammunition boxes | Too much hardware. |
| Radar link on the One | Radar stays phone-only. MV reaches the One by push or manual entry. |
| Plausibility clamp on manual MV/BC | Not wanted. The tag and the expected-MV line are the guard. |
| Confirmation prompt on wake for old manual values | Not wanted. |
| Live compass azimuth | Fails under covered metal firing lines and needs calibration on the rifle. |
| Azimuth remembered per site, or invalidated when GNSS shows a new site | Not wanted. Tag only. |
| Wired Calypso (UART NMEA) on the One | Replaced by BLE. |
| 18650 cell | Pouch cell preferred. |
| nRF52840, STM32H7/RP2350 + radio, ESP32-P4 + C6 | Full port or two-chip design. Reconsider only if the benchmark gate fails. |
| Rangefinder over BLE | Proprietary per brand. Not v1. |
| Truing MV/BC from observed impacts | Not v1. |

## 3. Vocabulary

| Word | Meaning |
|------|---------|
| One | The rifle-mounted unit (this firmware). |
| Air | The BLE weather beacon (`re/weather_dongle/`). Broadcast only. |
| Link | The phone app (`app/`). |
| Load | One ammunition definition: name, drag model, BC (single or stepped), bullet weight, diameter, length, nominal MV, temperature slope. |
| Active load | The load the solver is using. Exactly one. |
| MV record | A muzzle velocity together with its reference temperature, its source and the time it was set. |
| Nominal MV | The MV stored in the load definition. |
| Expected MV | The nominal MV corrected to the current temperature with the load's slope. |
| Source tag | A short word next to a value saying where it came from: `PHONE`, `MAN`, `AIR`, `GPS`. |
| Age | Time since a tagged value was set, shown after the tag (`MAN 3d`). |
| Zero snapshot | The conditions recorded when the shooter confirmed zero for a load. |
| Nap | Automatic sleep that ends when the rifle moves. |
| Off | Deliberate sleep that ends only with the button. |
| Capture | Latching the shot angle with a button press while aiming. |

## 4. Inputs: where each value comes from

| Value | Sources, best first | When the best source is missing | Shown as |
|-------|---------------------|---------------------------------|----------|
| Rifle parameters (sight height, twist, click value, units) | First-boot wizard or Link; editable in a deep settings menu | n/a | Settings |
| Active load | Chosen from the list on the One, or set by Link | Stays as last chosen | Load name on the HUD |
| MV | Link push (`PHONE`), manual edit (`MAN`) | Stays as last set | `MV` line with tag and age, plus `EXP` line |
| BC | Link push (`PHONE`), manual edit (`MAN`) | Stays as last set | Tag and age |
| Pressure | Barometer on the One | Sensor fault warning | Info screen |
| Temperature | Air | Last value (latest of Air reading or manual edit) + "Air not connected" warning | HUD with source |
| Humidity | Air | Last value + the same warning | Info screen |
| Latitude | GNSS (`GPS`), Link (`PHONE`), manual (`MAN`) | Stored value with age | Info screen |
| Clock | GNSS, Link | Ages show `?` until a time source is seen | n/a |
| Azimuth | Link (`PHONE`), manual (`MAN`) | Stays as last set | Info screen with tag and age |
| Wind | BLE anemometer (later), manual speed + clock direction | Manual | HUD, with a link indicator for the sensor |
| Shot angle | Button capture | 0 | HUD when captured |
| Distance | Live dial, or current stage | n/a | Largest item on the HUD |
| Cant | IMU | Sensor fault warning | Level indicator (warning only; the solution is never corrected for cant) |

## 5. Behaviour rules

Implement each rule in exactly one place.

### 5.1 Loads (rules L)

- L1. The One stores up to 8 loads. Each has a name of its own.
- L2. Exactly one load is active. The choice survives sleep and power loss.
- L3. The active load's name is always visible on the main HUD.
- L4. A load can be selected on the One without a phone.
- L5. Every field of a load can be edited on the One in a deep settings menu. MV and BC of the active load have a short path.
- L6. A load pushed by Link with a name that already exists replaces that load's definition. Its zero snapshot is kept.

### 5.2 Muzzle velocity and BC (rules V)

- V1. An MV record holds: speed, reference temperature, source, time set.
- V2. A push from Link sets source `PHONE` and uses the reference temperature supplied by Link.
- V3. A manual edit on the One sets source `MAN` and takes the current temperature as the reference temperature.
- V4. The solver uses the MV corrected to the current temperature: speed + slope x (current temperature - reference temperature). With slope 0 the stored speed is used unchanged.
- V5. The HUD shows the MV in use, its source and its age.
- V6. The HUD shows the expected MV: the load's nominal MV corrected to the current temperature with the same slope.
- V7. No value is clamped or refused. No confirmation is asked on wake.
- V8. BC carries a source and age tag in the same way as MV.

### 5.3 Weather (rules W)

- W1. Pressure always comes from the One's barometer.
- W2. Temperature and humidity come from the Air. A reading is current for 30 seconds after reception (existing `WEATHER_STALE_MS`).
- W3. The Air may appear or reappear at any time. Its latest reading replaces whatever temperature and humidity are in use, including manual values.
- W4. Temperature and humidity can be edited manually at any time.
- W5. When there is no current Air reading, the last values stay in use and the HUD shows a warning that the Air is not connected.
- W6. The HUD shows whether the Air is connected.
- W7. The Air's own pressure value is a cross-check only. A difference from the One's barometer above a threshold (proposed 3 hPa, to tune) raises a sensor warning.

### 5.4 Coriolis (rules C)

- C1. Latitude source priority: GNSS, then Link, then manual. The value in use is tagged with source and age.
- C2. Azimuth comes from Link or manual entry. It is tagged with source and age. There is no site logic.
- C3. Coriolis is on whenever latitude and azimuth are both set. It is no longer disabled in firmware.
- C4. GNSS altitude is not used.

Open point: behaviour before any azimuth has ever been set (section 12).

### 5.5 Wind (rules N)

- N1. The solver takes wind from a wind source interface with two implementations: manual, and BLE anemometer.
- N2. Manual wind is a speed and a clock direction relative to the bore (12 = headwind, 3 = from the right).
- N3. Anemometer wind is relative to the anemometer's reference mark, which the shooter points at the target. No azimuth or compass is involved.
- N4. When the anemometer link is absent, manual wind is used. The HUD shows which one is in use and whether the sensor is connected.
- N5. Sensor wind is smoothed over a few seconds before use.

### 5.6 Shot angle (rules I)

- I1. The angle is latched by a button press while the shooter aims at the target.
- I2. The capture screen states the assumption: turrets at 0 clicks.
- I3. The captured angle stays in the solution and on the HUD until the next capture or until it is cleared.
- I4. Pitch zero is set once at install, with the bore level, next to the cant calibration.

### 5.7 Zero (rules Z)

- Z1. "Set zero" is a menu action used when the shooter has just confirmed zero. It records, for the active load: zero distance, MV record, temperature, pressure, humidity, date.
- Z2. The sight angle is computed once from the snapshot and kept fixed. Current MV and weather change only the trajectory.
- Z3. Each load has its own snapshot.
- Z4. A load with no snapshot is solved as today (zero assumed in current conditions) and the info screen shows `ZERO ?`.
- Z5. The info screen shows the snapshot: distance, MV, temperature, age.

This replaces the current behaviour in `ModeManager::compute()`, which re-solves the zero with the current weather and MV whenever either changes.

### 5.8 Distance (rules D)

- D1. Live mode keeps digit entry and gains a coarse step on a held direction.
- D2. Staged mode shows the previous, current and next target with their solutions together.
- D3. Stages can be added and changed on the One. Stages created on the One get generic names.

### 5.9 Power (rules P)

- P1. Nap: after 10 minutes without motion the One sleeps. Motion wakes it. Distance, stage, captured angle and wind are restored exactly.
- P2. Off: a long press on CENTER. Only the button wakes it.
- P3. Safety net: if motion keeps waking the One but no button is pressed for 30 minutes, it goes to Off.
- P4. Waking from Off starts a GNSS fix. The One never waits for it: it starts with stored values and adopts the fix when it arrives. The GNSS is powered down after the fix or a timeout.
- P5. Waking from Nap does not start a GNSS fix.

### 5.10 Phone link (rules B)

- B1. Pairing uses LE Secure Connections with a 6-digit passkey shown on the One's display. Keys are stored (bonding).
- B2. Pairing can only be started from a menu action on the One and stays open for 60 seconds.
- B3. The One advertises whenever it is awake. A bonded phone reconnects without user action.
- B4. Every config, command and telemetry characteristic requires an encrypted, bonded link.
- B5. The app-layer PIN (`ble_pin`, default 1234) and the manual "BT" menu toggle are removed.
- B6. "Forget phones" clears all bonds.
- B7. The HUD shows whether a phone is connected.

### 5.11 Input safety (rules K)

- K1. Button input is ignored for about 0.5 seconds after the IMU detects a recoil shock (the thumb rests on the D-pad when the shot breaks).
- K2. Stored values (MV, BC, azimuth, temperature, load fields) can only change inside an edit mode that must be entered first. A stray click on the main HUD can change only distance or wind.

## 6. Display content

Main HUD, always visible: distance, elevation, windage, wind in use, active load name, MV line (value, source, age), expected MV, temperature with source, link indicators (phone, Air, wind sensor), shot angle when captured, cant indicator, battery.

Info screen, one press away: latitude, azimuth and zero snapshot with their tags; pressure; humidity; GNSS status.

State is always shown with text or a symbol as well as inversion or position, so it survives glare and low light. The panel has no light of its own: a button-activated frontlight is required (section 7.1).

The exact layout is a work item (step A4), to be reviewed with the user as a mock-up before implementation.

## 7. Hardware

Target MCU and board variant for v3: **ESP32-S3-WROOM-1-N16R8** on a custom PCB. The breadboard step uses an S3 dev board with the same module.

Part numbers below are candidates named during the review. **Sizes, prices, currents and availability were not verified and must be checked against datasheets at selection time.**

### 7.1 Parts

| Function | v2 (today) | v3 | Bus | Notes |
|----------|-----------|----|-----|-------|
| MCU | ESP32-WROOM-32 dev board | ESP32-S3-WROOM-1-N16R8 | n/a | Native USB replaces the USB-UART bridge. Pre-certified module, no custom antenna. |
| Display | SH1106 OLED 128x64 | Sharp Memory LCD 2.7" 400x240 (LS027B7DH01 class) | SPI | Supported by U8g2. Frame buffer 12 kB. |
| Frontlight | none | Small LED front or side light, button-activated | GPIO | For dusk and night. |
| Barometer | none (pressure from Air) | BMP390 / BMP581 class | I2C | Needs a vent with a membrane in the enclosure. |
| GNSS | none | Small module, power-gated by a load switch. Candidates: ATGM336H or Quectel L76K class with separate antenna; u-blox MAX-M10S; module with built-in antenna (u-blox SAM-M10Q, Quectel L80/L86 class) | UART | Antenna on the top face with sky view. |
| IMU | MPU6050 (end of life) | Current 6-axis part with a wake-on-motion interrupt (LSM6DSO / BMI270 class) | I2C | Must sit on the rigid display body. Interrupt wired to a wake-capable pin. |
| Magnetometer | QMC5883P | removed | n/a | |
| Input | 5-way switch, one GPIO per direction | Metal-dome D-pad on the thumb rest, same 5 lines + ground | GPIO | 6-wire cable to the body. |
| Body buttons | none | Proposed: one or two buttons on the body (wake, back) | GPIO | So the unit works without the thumb piece. To confirm. |
| Battery | LiPo, 2000 mAh | LiPo pouch, about 2000 mAh | n/a | |
| Charging | external | USB-C charger IC on the PCB | n/a | Same connector as firmware update. |
| Battery level | Resistor divider on ADC + voltage curve | Proposed: fuel-gauge IC (MAX17048 class) | I2C | More trustworthy in the cold. To confirm. |
| Wind | UART2 pins reserved for wired Calypso | none on the PCB (BLE) | n/a | |
| Status LED | GPIO 2 | Optional | GPIO | |

### 7.2 Radio roles

The One runs three BLE roles at once: peripheral (Link, bonded), observer (Air beacon), central (wind sensor, when one is chosen). NimBLE supports this. WiFi is not used.

### 7.3 Layout

- Display body: PCB, battery, GNSS, barometer, IMU, LCD. Mounted rigidly to the receiver or scope (rail or ring side mount), screen toward the shooter, GNSS antenna toward the sky.
- Thumb piece: D-pad on a small thin PCB under a silicone cap, on the thumb rest (existing CAD: `CAD/Tikka T1X thumb grip*`, `CAD/5Axis button cap*`).
- One short cable between them with a locking connector and strain relief.

### 7.4 Constraints to respect in the pin map

- Keep the module's strapping pins and the pins used internally by the octal PSRAM free (check the module datasheet for the N16R8 variant).
- Wake sources (CENTER button, IMU interrupt) must be on pins that can wake the chip from deep sleep.
- All pins are named constants in `src/config.h`. No pin numbers inline.

### 7.5 Power budget (estimates, to be measured in step A9)

- Awake with BLE scanning and one central link, no light sleep: on the order of 40 to 60 mA, giving roughly 30 hours on 2000 mAh.
- Display: negligible.
- GNSS: tens of mA for seconds per wake from Off.
- Off and Nap: tens of microamps on a custom PCB; Nap slightly higher because the IMU stays in motion-detect mode.

Light sleep is not required for v1. If measured awake time is below one full day, revisit.

### 7.6 Product-ready foundations

- Firmware update over USB-C in v1. Flash laid out with two firmware slots from the start so wireless update can be added later.
- No personal values in firmware (device addresses, names). Everything through pairing or config.
- First-boot wizard: units, scope click value, sight height, twist, cant zero, pitch zero, optional phone pairing.
- Not in v1: enclosure tooling, sealing tests, whole-device regulatory testing, paid BLE library licence for the app, iOS.

## 8. Removed from the One

- Magnetometer, its calibration flow and the heading readout.
- Wired Calypso path (`PIN_WIND_RX`, `PIN_WIND_TX`, NMEA parser in `src/wind.*`), replaced by the wind source interface.
- App-layer PIN and the "BT" menu toggle.
- Single `RifleConfig` that mixes rifle, load, weather fallback and BLE settings: to be split into rifle parameters, load list, and device settings.
- Live re-solving of the zero.
- Battery ADC divider, if the fuel gauge is confirmed.

## 9. Build plan

Two steps. Step A is the S3 breadboard; step B is the PCB and enclosure. Finish every "Done when" item of a work item before starting the next. When an item cannot be met or the plan looks wrong, stop and ask the user.

Tests: before writing any test, agree with the user which behaviours and functions are critical (see `testing.instructions.md`). Candidates are the pure rules of section 5: MV correction and tags (V), weather replacement (W), zero snapshot (Z), load store (L), wake/sleep state machine (P). These must be testable on the host (`env:native`) without hardware.

### Step A: S3 breadboard

**A1. Bring-up.** New PlatformIO environment for the S3 dev board (N16R8), PSRAM enabled, two-slot partition table, native USB serial. Library versions pinned. Debug output behind a compile-time flag.
Done when: the existing firmware logic builds for the S3 target; native tests still pass; the board boots and logs over USB.

**A2. Solver benchmark gate.** Measure the worst case on the S3: 2-mile solve, multi-BC, Coriolis on, float inner loop. Report the time for a zero solve and for one correction.
Done when: numbers are reported to the user. Pass (proposed): a new solution appears in under 150 ms after a distance change. On fail, stop and review the MCU decision with the user before any further hardware work.

**A3. Display driver.** Sharp LCD over SPI through U8g2; VCOM handling; frontlight on a button.
Done when: a test screen is readable in direct sun and with the frontlight in the dark; refresh is fast enough for the distance dial.

**A4. HUD and info screen.** Mock-up reviewed by the user first, then implemented (section 6).
Done when: every item of section 6 is visible with real or simulated data, and the user has approved the layout on the panel.

**A5. Storage model.** Rifle parameters, load list with active load, MV records, zero snapshots, device settings. Migration from the v2 NVS layout is not required (new hardware).
Done when: rules L1 to L6 hold across power loss.

**A6. MV, BC and tags.** Rules V1 to V8, including manual edit path and age display. Age shows `?` without a clock.
Done when: a pushed value shows `PHONE`, an edited one `MAN`, ages advance, and the expected MV follows temperature with a non-zero slope.

**A7. Barometer and weather rules.** Rules W1 to W7. Align the Air's advertised-name constant with the BoreCore naming (firmware currently looks for `WeatherDongle`).
Done when: pressure is live with nothing else present; the Air replaces manual values on arrival; the warning appears 30 seconds after the Air stops.

**A8. GNSS.** Power gating, one fix per wake from Off, latitude and clock, timeout, tags (rules C1, P4, P5).
Done when: latitude and time are set from a fix outdoors; indoors the One starts without delay and shows the stored value with its age.

**A9. IMU, Nap and Off.** New IMU driver for cant and pitch; wake-on-motion; rules P1 to P3. Measure current in awake, Nap and Off.
Done when: the three states behave as specified and measured currents are reported to the user.

**A10. Shot angle capture.** Rules I1 to I4. Wire the engine's existing look-angle input, which the firmware does not use today.
Done when: a captured angle changes the solution, is shown on the HUD, and can be cleared.

**A11. Zero snapshot.** Rules Z1 to Z5.
Done when: after "Set zero", changing MV or temperature shifts the solution at the zero distance instead of hiding the shift; a load without a snapshot shows `ZERO ?`.

**A12. Coriolis.** Rules C1 to C4, azimuth entry on the One.
Done when: the correction changes with latitude and azimuth and matches the reference comparison suite for the Coriolis scenarios.

**A13. Wind source interface.** Rules N1, N2, N4 with the manual implementation only. Remove the UART path.
Done when: manual wind drives windage; the interface accepts a second implementation without changes to the solver or the HUD.

**A14. Phone link.** Rules B1 to B7. Extend the config schema for loads, MV records, latitude, azimuth and time.
Done when: an unbonded phone cannot read or write config; a bonded phone reconnects by itself after the One wakes.

**A15. D-pad and input safety.** Dome D-pad prototype on the thumb rest; rules K1 and K2; body buttons if confirmed.
Done when: no input is registered during recoil in live fire, and no stored value can change outside edit mode.

**A16. Distance.** Rules D1 to D3.
Done when: a three-target stage can be shot without pressing a button between reading solutions.

**A17. First-boot wizard and USB update.**
Done when: a person other than the author can take an erased unit to a working solution using only the display, and can update firmware over USB.

**A18. Wind sensor over BLE (when a model is chosen).** First task: obtain and read the manufacturer's BLE developer documentation. Confirm notification rate options, the direction reference, whether a second central is allowed, and reconnect behaviour. Then implement rules N3 and N5.
Done when: wind from the sensor drives windage, and the manual fallback takes over within a few seconds of link loss.

### Step B: PCB and enclosure

Start only when step A works on the breadboard and the pin map has stopped changing.

**B1. Schematic and PCB** (`PCB/ballisticCalc.eprj`): module, LCD connector, barometer, GNSS with load switch and antenna, IMU with interrupt, charger, fuel gauge if confirmed, USB-C, D-pad connector, body buttons, frontlight driver, test points.
**B2. Thumb piece PCB** with five domes and the cable connector.
**B3. Enclosure**: rigid mount, screen bezel protecting the glass, barometer vent with membrane, antenna on top with nothing metallic above it, cable strain relief.
**B4. Validation on the rifle**: recoil (no resets, no false inputs, connector holds), sun readability, cold start, measured battery life, GNSS fix time when mounted.

## 10. Link app changes that follow

- Load editor; push loads to the One.
- Push the active batch MV with its reference temperature (from the ammunition grouping work).
- Push latitude, azimuth (map bearing to the target) and time on every connection.
- Bonding flow with passkey entry; remove PIN entry.
- Stage list editor stays.

These are listed for awareness. They need their own plan.

## 11. Data plan addition

Open question raised in the review: does every cartridge react to temperature the same way, or does a common rule hold for a lot? This cannot be answered without data.

- Starting assumption only: temperature shifts a whole lot together, and per-cartridge effects are smaller.
- To test it: fire the same lot in at least three sessions spread over about 15 degrees C. Estimate the common slope, then check whether the per-cartridge prediction error depends on temperature combined with cartridge measurements.
- Until then the slope on the One stays 0.
- If per-cartridge effects prove significant, Link does the richer prediction and pushes the result with the current temperature as reference. The One's design (rules V1 to V4) does not change.

## 12. Open points

| # | Point | Proposed | Status |
|---|-------|----------|--------|
| 1 | Coriolis before any azimuth has been set | Horizontal term only, info screen shows `AZ ?` | To confirm |
| 2 | Fuel-gauge IC instead of ADC divider | Yes | Proposed, not objected |
| 3 | Buttons on the display body | One or two | Proposed, not objected |
| 4 | Which button gesture captures the shot angle | Decide with the HUD mock-up (A4) | Open |
| 5 | Anemometer model | Calypso BLE portable family | Undecided |
| 6 | Benchmark pass threshold | 150 ms | Proposed |
| 7 | Air pressure cross-check threshold | 3 hPa | Proposed |
| 8 | Exact parts for GNSS, barometer, IMU, domes, charger | Section 7.1 candidates | To verify against datasheets |

## 13. Estimates used in the review

These numbers guided decisions. They are order-of-magnitude estimates made during the discussion, not measurements.

| Effect | Estimate |
|--------|----------|
| Horizontal Coriolis | about 0.05 mil (.22LR at 300 yd), 0.08 mil (centerfire at 1000 yd), 0.35 mil (2 miles, 45 N) |
| Latitude error of 5 degrees | about 0.03 mil at 2 miles |
| Vertical Coriolis | up to about 0.3 mil at 2 miles; 10 degrees of azimuth error costs up to about 0.05 mil |
| MV error of 10 fps, .22LR at 300 yd | about 0.3 mil |
| Zero shift between batches, .22LR zeroed at 50 m | about 0.07 mil per 5 m/s |
| Shot angle of 10 degrees ignored at 1000 yd | about 0.2 mil |
