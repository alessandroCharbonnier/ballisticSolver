# Collect mode: implementation plan (phase 1)

Audience: the developer or coding agent who implements this. Read the whole file once, then work through section 13 in order. This file is the single source of truth for phase 1. Where it conflicts with older notes or with existing code, this file wins.

## 1. Goal and scope

The BoreCore Link phone app (Flutter, Android) must let one shooter build a clean data set that links each .22LR cartridge's bench measurements to the velocity the radar measured when it was fired. The data set is exported as CSV and used on a PC to train a model (see `re/ML_PROTOTYPE_SPEC.md`).

Phase 1 delivers three things:

1. **Measure** (at home): enter weight and rim thickness for every cartridge of a box, on a grid that looks like the box.
2. **Fire** (at the range): tap cartridges to load a magazine on screen; every radar shot is attached to the top cartridge automatically, together with the weather and the barrel state.
3. **Data**: review sessions, export CSV, back up and restore the database.

The shooter must be able to fire a whole magazine without touching the phone. Loading takes one tap per cartridge.

Everything not listed here is out of scope (section 16).

## 2. Rules for working

1. Do the steps of section 13 in order. Each step ends with a "Done when" list. Meet every item on it before starting the next step.
2. When a "Done when" item cannot be met, or the plan looks wrong or incomplete, stop and ask the user. Report what you tried and what happened.
3. Build exactly what this file describes.
4. Before calling any function of a plugin, open the plugin's source and read the real signature. Plugin sources are in `%LOCALAPPDATA%\Pub\Cache\hosted\pub.dev\<package>-<version>\lib`. Plugin APIs in this Flutter version differ from older tutorials.
5. `Z:\embedded\ballisticCalc\app` is the source of truth. `C:\Users\haloa\dev\borecore_app` is a disposable build copy. Edit on Z:, build and test on C: (section 4). Delete files only inside the C: copy.
6. Z: is a network drive and a file write can fail silently. After creating files, check that each one exists with `Test-Path`.
7. Before writing tests, read `c:\Users\haloa\.copilot\instructions\testing.instructions.md` and follow it.
8. Import Material widgets from `package:material_ui/material_ui.dart`. In this Flutter version `package:flutter/material.dart` is deprecated.
9. User-visible text is English, with no emojis. Every tap target is at least 48 dp. State is always shown with text or a number as well as colour.
10. Numbers that may need tuning (limits, timeouts, capacities) are named constants in one file, `lib/domain/constants.dart`.

## 3. Vocabulary

Use these words in code, UI text and commit messages.

| Word | Meaning |
|------|---------|
| Product | An ammunition type, e.g. "SK Rifle Match". Owns the box layout. |
| Lot | A manufacturing lot number of one product. |
| Box | One physical tray of cartridges belonging to one lot. Has a short code such as `B01` that the shooter writes on the real box. |
| Slot | A position in a box, numbered from 1. Permanent: a slot number is never reused or renumbered. |
| Cartridge | The round in one slot of one box. Identified by (box, slot). |
| Magazine | The on-screen stack of cartridges loaded in the rifle magazine. Position 1 is the bottom (loaded first, fired last). |
| Top | The cartridge with the highest magazine position. It is the next one fired. |
| Session | One day at the range with one rifle. |
| Shot | One radar notification. Normally attached to a cartridge. |
| Unassigned shot | A shot that arrived while the magazine was empty. |
| Voided shot | A shot the shooter cancelled with Undo. Kept in the database, flagged. |
| Reading | One weather measurement received from the dongle. |
| Radar | The FX True Ballistic chronograph (Bluetooth). |
| Dongle | The weather beacon (`re/weather_dongle/`). It only broadcasts; the app never connects to it. |

## 4. Environment and commands

- Target device: AYN Thor (Android, landscape screen), adb serial `d3cb015a`.
- Flutter 3.50.1 at `C:\Users\haloa\flutter`. Package id `com.borecore.borecore_one`.
- Android builds fail from the Z: network drive (Gradle cannot read it). Always build from the C: copy.
- Use the JDK that ships with Android Studio. The system JDK 25 is too new for Gradle.

PowerShell commands. Check the current folder with `Get-Location` before any `flutter` command.

```powershell
$SRC     = 'Z:\embedded\ballisticCalc\app'
$BUILD   = 'C:\Users\haloa\dev\borecore_app'
$FLUTTER = 'C:\Users\haloa\flutter\bin\flutter.bat'
$ADB     = "$env:LOCALAPPDATA\Android\Sdk\platform-tools\adb.exe"
$env:JAVA_HOME = 'C:\Program Files\Android\Android Studio\jbr'

# Add a package (run on the source, then sync)
Set-Location $SRC; & $FLUTTER pub add <package_name>

# Sync source -> build copy
Remove-Item "$BUILD\lib","$BUILD\test" -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item "$SRC\lib" "$BUILD\lib" -Recurse -Force
if (Test-Path "$SRC\test") { Copy-Item "$SRC\test" "$BUILD\test" -Recurse -Force }
Copy-Item "$SRC\pubspec.yaml","$SRC\pubspec.lock" $BUILD -Force
Copy-Item "$SRC\android\app\src\main\AndroidManifest.xml" "$BUILD\android\app\src\main\AndroidManifest.xml" -Force

# Check, test, build, install, start (from the build copy)
Set-Location $BUILD
& $FLUTTER pub get
& $FLUTTER analyze
& $FLUTTER test
& $FLUTTER build apk --debug
& $ADB install -r "$BUILD\build\app\outputs\flutter-apk\app-debug.apk"
& $ADB shell am start -n com.borecore.borecore_one/.MainActivity
```

## 5. Starting point

The app already builds and runs. Existing files in `lib/` and what happens to each:

| File | Action |
|------|--------|
| `main.dart` | Rewrite in step 3: create the services (section 7), then show Home. |
| `screens/home_screen.dart` | Rewrite in step 3: three entries (section 11.1). |
| `screens/radar_screen.dart` | Delete in step 3, once Home no longer links to it. The Fire screen replaces it. |
| `radar/radar.dart` | Delete in step 4. Its logic moves to `domain/radar_packet.dart` and `sources/radar_source.dart`. |
| `models/shot.dart` | Delete in step 4, together with `radar/radar.dart`, the last file that imports it. Replaced by the domain models. |
| `ble/gatt.dart`, `ble/sentinel_device.dart`, `screens/scan_screen.dart`, `screens/device_screen.dart` | Keep unchanged. They talk to the rifle-mounted device and are needed in phase 2. No entry on Home leads to them in phase 1. |

Known defect in the existing radar code, to fix when moving it: it listens to `lastValueStream`. That stream replays the previous value whenever it is listened to again, so after a reconnect the previous shot would arrive a second time and be attached to the wrong cartridge. Listen to `onValueReceived` instead.

## 6. Packages

Already present: `flutter_blue_plus` (Bluetooth), `material_ui` (widgets).

Add these. Let `flutter pub add` choose the version.

| Package | Used for |
|---------|----------|
| `sqflite` | SQLite database on the phone. |
| `path` | Joining file paths. |
| `path_provider` | Finding the app's documents and temporary folders. |
| `share_plus` | Opening the Android share menu for an exported file. |
| `file_picker` | Choosing a backup file to import. |
| `wakelock_plus` | Keeping the screen on while the Fire screen is open. |
| `sqflite_common_ffi` (dev dependency: `flutter pub add --dev`) | Running the database tests on the PC. |

State is handled with Flutter's own `ChangeNotifier` and `ListenableBuilder`. Add no state-management package.

`flutter_blue_plus` requires a licence argument on `connect`. Use `License.nonprofit`, as the existing code does.

## 7. Code structure

```
lib/
  main.dart
  app/          app_services.dart, app_scope.dart
  domain/       pure Dart: no Flutter imports, no plugin imports
  data/         SQLite: database, repositories, export, backup
  sources/      radar and weather inputs: Bluetooth versions and simulated versions
  controllers/  one ChangeNotifier per screen
  screens/      one file per screen
  widgets/      box_grid.dart, magazine_stack.dart, weather_chip.dart, radar_status.dart
  ble/          existing Sentinel code (unchanged)
test/
  domain/       tests of pure logic
  data/         tests against a real SQLite database
```

Layering, top depends on bottom, never the reverse:

- `screens` and `widgets` show state and forward taps to a controller.
- `controllers` hold screen state and call repositories and sources.
- `data` reads and writes the database. Every operation that changes more than one row runs in one database transaction.
- `sources` deliver radar shots and weather readings as streams.
- `domain` holds the rules of section 9 as plain functions and classes, so they can be tested without a phone.

`AppServices` is created once in `main` before the first screen. It owns the database, the repositories, the settings, the radar source and the weather source, and a clock. `AppScope` is an inherited widget that makes `AppServices` available to every screen. Controllers receive what they need through their constructor.

The clock is an object with one method, "now". Production uses the system time. Tests pass a fake clock so time-based rules are deterministic.

## 8. Database

One SQLite file, `sentinel.db`, in the app's default database folder. Schema version 1. Turn foreign keys on when the database is opened. All times are stored as integer milliseconds since 1970 UTC. Booleans are stored as 0 or 1.

**products**

| Column | Type | Meaning |
|--------|------|---------|
| id | integer, key | |
| name | text, unique, required | e.g. "SK Rifle Match" |
| rows | integer, default 5 | grid rows |
| cols | integer, default 10 | grid columns |
| hinge_edge | text, default `top` | which grid edge is the lid hinge: `top`, `bottom`, `left`, `right` |
| created_at | integer | |

**lots**

| Column | Type | Meaning |
|--------|------|---------|
| id | integer, key | |
| product_id | integer, required | references products |
| lot_number | text, required | unique together with product_id |
| created_at | integer | |

**boxes**

| Column | Type | Meaning |
|--------|------|---------|
| id | integer, key | |
| lot_id | integer, required | references lots |
| code | text, unique, required | e.g. `B01` |
| hidden | boolean, default 0 | 1 = removed from lists; the data stays |
| created_at | integer | |

**cartridges**. Created for every slot when the box is created, with empty measurements.

| Column | Type | Meaning |
|--------|------|---------|
| id | integer, key | |
| box_id | integer, required | references boxes |
| slot | integer, required | 1..rows*cols, unique together with box_id |
| weight_g | real, empty until measured | grams |
| rim_thickness_mm | real, empty until measured | millimetres |
| oal_mm | real, always empty in phase 1 | reserved; no input field |
| measured_at | integer, empty until measured | |
| state | text, default `box` | `box`, `magazine` or `fired` |
| mag_position | integer | set only while state is `magazine`; 1 = bottom |
| predicted_mps | real, always empty in phase 1 | reserved for phase 2 |
| batch_id | integer, always empty in phase 1 | reserved for phase 2 |

**rifles**. One row is inserted when the database is created: "Tikka T1X".

| Column | Type | Meaning |
|--------|------|---------|
| id | integer, key | |
| name | text, required | |
| created_at | integer | |

**sessions**

| Column | Type | Meaning |
|--------|------|---------|
| id | integer, key | this is the session ID in the export |
| rifle_id | integer, required | references rifles |
| started_at | integer | |
| last_active_at | integer | updated whenever the Fire screen is left |
| barrel_cleaned | boolean | the answer given at session start |
| clean_base | integer | shots since cleaning before this session's first shot (rule S3) |
| manual_temp_c | real, usually empty | temperature typed by the shooter as a fallback |

**shots**

| Column | Type | Meaning |
|--------|------|---------|
| id | integer, key | |
| session_id | integer, required | references sessions |
| cartridge_id | integer, empty for an unassigned shot | references cartridges |
| shot_index | integer | 1, 2, 3 ... within the session, voided shots included |
| mag_position | integer, empty if unassigned | the cartridge's magazine position when fired |
| freq_hz | integer | raw doppler frequency from the radar |
| bc | real | ballistic coefficient from the radar |
| fired_at | integer | when the notification arrived |
| temp_c, pressure_hpa, humidity_pct | real, may be empty | weather |
| weather_source | text | `dongle`, `manual` or `none` |
| cold_bore | boolean | |
| shots_since_clean | integer | |
| time_since_last_shot_s | real, empty for the first shot | |
| voided | boolean, default 0 | |
| voided_at | integer, empty unless voided | |

**settings**: `key` (text, key) and `value` (text). Keys used: `radar_remote_id` (default `D0:3A:C4:71:D4:AD`), `dongle_remote_id` (empty until a dongle is first seen), `last_box_id`, `last_product_id`, `last_lot_id`.

Nothing is ever deleted from these tables by the app. Boxes are hidden; shots are voided.

## 9. Behaviour rules

These rules are the specification. Implement each one in exactly one place and name the test after the rule.

### 9.1 Slots (rules G)

- G1. Slots are numbered in reading order: left to right, then top to bottom. Slot 1 is top-left.
- G2. For a product with `cols` columns: row = (slot - 1) divided by cols, rounded down, plus 1. Column = (slot - 1) modulo cols, plus 1.
- G3. The position label is `R<row> C<column>`, e.g. slot 14 of a 5 by 10 box is `R2 C4`. With the box it is `B01 R2 C4`.
- G4. The grid is always drawn with `rows` rows and `cols` columns. The default box is wide: 5 rows of 10.

### 9.2 Measurements (rules M)

- M1. A cartridge is "measured" when both weight and rim thickness are present.
- M2. Number input accepts a comma or a dot as decimal separator (the phone is set to French). Anything else that is not a digit is rejected.
- M3. Warnings never block saving. A value triggers a warning when it is outside the plausible range (weight 2.50 to 4.50 g, rim thickness 0.80 to 1.30 mm), or when the box already has 5 or more measured cartridges and the value differs from the box median by more than 0.20 g or 0.10 mm.
- M4. When a warning applies, the first press of Save shows the warning and the button text becomes "Save anyway". The second press saves.
- M5. Any cartridge can be measured again or corrected at any time, in any state. Saving sets `measured_at` to now.

### 9.3 Magazine (rules L)

- L1. The magazine holds at most 10 cartridges.
- L2. Load: allowed when the cartridge is measured, its state is `box`, and the magazine holds fewer than 10. The cartridge becomes `magazine` with position = current count + 1.
- L3. Unload: tapping the magazine returns the top cartridge to state `box` and clears its position. With an empty magazine nothing happens.
- L4. Only the top can be unloaded. Tapping a loaded cartridge in the box grid does nothing.
- L5. Cartridges from different boxes may be in the magazine together.
- L6. The magazine lives in the database (cartridge state and position), so it is exactly the same after the app is closed, killed or restarted.

### 9.4 Shots (rules F)

- F1. A radar shot is handled the same way whatever the screen is showing or the magazine contains.
- F2. If the magazine is not empty, the shot is attached to the top. That cartridge becomes `fired` and leaves the magazine.
- F3. If the magazine is empty, the shot is saved with no cartridge (unassigned shot).
- F4. Saving the shot and changing the cartridge happen in one database transaction.
- F5. Load, unload, shot and undo are processed one at a time, in arrival order, through a single queue in the Fire controller. Two of them never run at the same moment.
- F6. Undo voids the most recent shot of the session that is not yet voided. If that shot had a cartridge, the cartridge returns to the top of the magazine. If the magazine already holds 10, undo is refused with a message. Pressing Undo again voids the shot before it.
- F7. A voided shot is ignored by every counter in section 9.5.

### 9.5 Sessions and counters (rules S)

- S1. When the Fire screen opens: if a session of the active rifle started today (local date), continue it. Otherwise ask "Barrel cleaned since last session?" (Yes / No) and create a session. Leaving the question without answering closes the Fire screen.
- S2. The radar and weather sources are started only once a session exists.
- S3. `clean_base` is fixed when the session is created: 0 if the barrel was cleaned; otherwise the `shots_since_clean` of the rifle's most recent shot in earlier sessions, or 0 if there is none.
- S4. For each new shot: `shots_since_clean` = clean_base + number of earlier shots in this session + 1.
- S5. `cold_bore` is 1 for the first shot of the session and 0 for all others.
- S6. `time_since_last_shot_s` is the time in seconds, with decimals, since the previous shot of the session. It is empty for the first shot.
- S7. `shot_index` = highest shot_index of the session + 1 (voided shots included, so indexes are never reused).
- S8. Unassigned shots count in S4 to S6, because a bullet went down the barrel.

### 9.6 Weather (rules W)

- W1. The app keeps only the latest reading and the time it was received.
- W2. The reading is "current" for 30 seconds after it was received. After that there is no current reading.
- W3. When a shot is saved: if there is a current reading, store its three values with source `dongle`. Otherwise, if the session has a manual temperature, store that temperature only, with source `manual`. Otherwise store no weather, with source `none`.
- W4. Shooting is never blocked by missing weather.
- W5. The first dongle ever seen is remembered (`dongle_remote_id`). Beacons from any other device are ignored until the shooter chooses "Forget dongle" in Settings.
- W6. Ten seconds after a session becomes active, if there is still no current reading and no manual temperature, the app offers once to type the temperature (section 11.4).
- W7. Weather can be added later to shots whose source is `none` (section 11.5). Those shots get source `manual`.

## 10. Bluetooth sources

Two small interfaces in `sources/`, each with a Bluetooth version and a simulated version:

- **Radar source**: a stream of shots (frequency, BC) and a stream of link states (`searching`, `connecting`, `connected`, `disconnected`). Has start and stop.
- **Weather source**: a stream of readings (temperature in C, pressure in hPa, humidity in %). Has start and stop.

The Fire controller only knows the interfaces.

### 10.1 One scanner

Android allows one scan at a time and limits how often a scan may be started. A single scanner object owns scanning while the Fire screen is open and passes every scan result to both Bluetooth sources.

- Start one scan with no time limit and with repeated advertisements enabled (the `continuousUpdates` option; read its documentation in the plugin source first).
- Restart the scan every 5 minutes, because Android can quietly slow down a scan that runs for a long time.
- Stop it when the Fire screen closes.

### 10.2 Radar

Protocol reference: `re/RADAR_PROTOCOL.md`.

- Recognise the radar by its remote id (setting `radar_remote_id`, compared without regard to letter case) or by the advertised name `vXfS3c6k`.
- When the radar appears in a scan result and the link state is `searching`: pause the scan, connect with the same options the existing `radar/radar.dart` uses (they are known to work), discover services, find service `00001623-88ec-688c-644b-3fa706c0bb76` and characteristic `00001624-88ec-688c-644b-3fa706c0bb76`, turn notifications on, listen to `onValueReceived`, then resume the scan.
- A shot notification is exactly 9 bytes: byte 0 is `0x0B`, byte 8 is `0x05`, bytes 2-3 are the frequency (unsigned 16-bit, big-endian), bytes 4-7 are the BC (32-bit float, big-endian). Ignore anything else, and ignore a frequency of 0.
- Velocity in m/s = frequency / 160.647. It is computed for display only. The database stores the raw frequency.
- When the link drops: state becomes `disconnected`, then `searching`, and the radar is reconnected when it is seen again. The app never invents a shot.

### 10.3 Dongle

Firmware reference: `re/weather_dongle/src/main.cpp`.

- The dongle advertises manufacturer data with company id `0xFFFF` and the name `WeatherDongle`.
- The plugin gives manufacturer data as a map keyed by company id. The value for `0xFFFF` must be exactly 20 bytes, little-endian: temperature in C (float, offset 0), pressure in hPa (float, offset 4), humidity in % (float, offset 8), sequence number (unsigned 32-bit, offset 12), uptime in ms (unsigned 32-bit, offset 16).
- Accept a reading only if temperature is between -40 and 85, pressure between 300 and 1100, humidity between 0 and 100. Ignore it otherwise.
- Risk to check in step 4: the Android manifest declares the scan permission with the `neverForLocation` flag, and Android hides some beacons from apps that use it. If the radar is found but the dongle never is, report this to the user before changing the manifest.

## 11. Screens

Design for landscape first. Screens that show the box grid use the side-by-side layout when the screen is wider than tall, and stack the two parts vertically otherwise.

### 11.1 Home

Three large entries: **Boxes**, **Fire**, **Data**. A gear icon in the app bar opens Settings.

### 11.2 Boxes

- A list of boxes that are not hidden, newest first. Each line: code, product, lot, "measured 37/50", "fired 12".
- Tapping a box opens the Measure screen for it.
- "New box" opens a form: product (choose an existing one or type a new name), lot number (choose or type), box code. Product and lot are prefilled with the last ones used. The code is prefilled with the next free code (`B01`, `B02`, ...) and can be edited; it must be unique. A new product also asks for rows, columns and hinge edge, prefilled with 5, 10 and top.
- Creating a box creates all its cartridges (empty) in the same transaction.
- A long press on a box offers "Hide box". It is refused while one of its cartridges is in the magazine.

### 11.3 Measure

- Left: the box grid (`BoxGrid` widget). A bar labelled "HINGE" is drawn along the product's hinge edge so the shooter turns the real box the same way.
- Each slot shows its state: empty (outline), measured (filled, with the weight in small text), fired (green). The next empty slot in reading order is highlighted.
- Tapping any slot opens the entry dialog for that slot. A "Measure next" button opens it for the next empty slot.
- Entry dialog: title "Slot 14 - R2 C4"; two numeric fields, weight (g) and rim thickness (mm); Save and Close. If the slot is already measured, the fields show the stored values.
- After Save, the dialog stays open and moves to the next empty slot: the title changes, the fields are cleared and the cursor is in the weight field. When no empty slot remains, the dialog closes with the message "Box complete".
- Rules M1 to M5 apply.

### 11.4 Fire

Opening follows rules S1 and S2. The screen stays on while this screen is open.

Layout in landscape:

- **Left, about two thirds of the width**: a box selector (boxes that are not hidden; preselects the last box used) and the box grid of the selected box.
- **Right column**, from top to bottom: radar status, weather line, last shot, magazine stack, a small Undo button, and the number of unassigned shots in this session when it is above zero.

Box grid slots on this screen:

| Cartridge | Looks like | Tap |
|-----------|-----------|-----|
| Not measured | dimmed | nothing |
| Measured, in the box | outlined circle | load (rule L2); when the magazine is full, show "Magazine full" |
| In the magazine | filled circle showing its magazine position number | nothing |
| Fired | green circle | nothing |

Magazine stack:

- Ten fixed rows; position 10 at the top, position 1 at the bottom.
- A filled row shows the position and where the cartridge came from, e.g. `3 - B01 R2 C4`. Empty rows are outlines.
- The top filled row is marked "NEXT".
- The whole stack is one tap target: a tap unloads the top (rule L3).

Other elements:

- **Radar status**: "Radar connected" (green), "Searching for radar" (amber), "Radar disconnected" (red).
- **Weather line**: with a current reading, `21.4 C  1013 hPa  45 %`. Without one, red text "No weather data", followed by `manual 18.0 C` if a manual temperature is set. Tapping the weather line opens the manual temperature dialog.
- **Manual temperature dialog**: "No weather dongle found. Type the temperature in C, or skip." with one numeric field, Skip and Save. Opened automatically once per session by rule W6, or by tapping the weather line. Saving stores `manual_temp_c` on the session.
- **Last shot**: velocity in m/s in large text with the BC below it, and the cartridge it was attached to (or "unassigned").
- **Undo**: applies rule F6 after a confirmation ("Cancel the last shot?").

Leaving the screen stops the scanner and the sources, updates the session's `last_active_at`, and writes an automatic backup (section 12.2). Cartridges still in the magazine stay there.

### 11.5 Data

- Top buttons: "Export all shots (CSV)", "Export full backup", "Import backup".
- A list of sessions, newest first: date and start time, number of shots, number of unassigned shots, barrel cleaned yes/no.
- Session detail: a table of the session's shots (index, time, cartridge label, velocity in m/s, BC, temperature, and the words "voided" or "unassigned" where they apply). Two actions: "Export this session (CSV)" and "Set weather for shots without data" (rule W7: temperature required, pressure and humidity optional).

### 11.6 Settings

- Rifle name (editable).
- Radar id (editable text, default as in section 8).
- Dongle: shows the remembered id or "none yet", with a "Forget dongle" button.
- Bluetooth check: starts the scanner and both sources and shows the radar link state, the last shot received and the last reading received with its age. Saves nothing. Used in step 4.
- Simulator on/off (section 12.4).
- App version.

## 12. Export, backup, import, simulator

### 12.1 CSV export

One row per shot, voided and unassigned shots included, oldest first. Comma separated, UTF-8, dot as decimal separator, one header row. A field containing a comma, a quote or a line break is wrapped in double quotes, with inner quotes doubled. Empty values are empty fields.

Columns, in this order:

| Column | Source |
|--------|--------|
| session_id | sessions.id |
| shot_index | shots.shot_index |
| mag_position | shots.mag_position |
| box_id | boxes.code |
| slot_in_box | cartridges.slot |
| weight_g | cartridges.weight_g |
| rim_thickness_mm | cartridges.rim_thickness_mm |
| oal_mm | cartridges.oal_mm |
| lot | lots.lot_number |
| freq_hz | shots.freq_hz |
| bc | shots.bc |
| temp_c, pressure_hpa, humidity_pct | shots |
| cold_bore | 0 or 1 |
| shots_since_clean | shots |
| time_since_last_shot_s | shots |
| timestamp_ms | shots.fired_at |
| fired_at_utc | shots.fired_at as `2026-11-26T14:03:22.123Z` |
| fired_at_local | shots.fired_at in phone local time as `2026-11-26 15:03:22.123` |
| weather_source | `dongle`, `manual` or `none` |
| voided | 0 or 1 |

The first eighteen columns are the ones of `re/ML_PROTOTYPE_SPEC.md` section 2 plus the session ID. The time columns were requested by the user. `weather_source` and `voided` are needed so the analysis can exclude cancelled shots and typed temperatures.

The file is written to the temporary folder as `sentinel_shots_<yyyyMMdd_HHmmss>.csv` and handed to the Android share menu, where the user picks the destination (files, Drive, Discord, WhatsApp, email).

### 12.2 Backup

- A backup is a copy of the database file named `sentinel_backup_<yyyyMMdd_HHmmss>.db`, stored in a `backups` folder inside the app's documents folder.
- To make one: close the database, copy the file, open the database again.
- Automatic backup: when the Fire screen is left, if the session received at least one shot since the previous backup. Keep the 20 newest backups and remove older ones.
- "Export full backup" makes a backup and hands it to the share menu.
- Limit to state in the README: automatic backups stay on the phone. They protect against a damaged database, not against a lost phone. Only an exported backup protects against that.

### 12.3 Import

1. Ask for confirmation: "This replaces all data in the app with the content of the backup. A backup of the current data is made first."
2. Let the user pick a file.
3. Check the file: it opens as a SQLite database, its schema version is 1, and it contains the `shots` table. If not, show "This file is not a BoreCore Link backup" and change nothing.
4. Make a backup of the current data.
5. Close the database, replace the file with the picked one, open it again.
6. Return to Home.

### 12.4 Simulator

Purpose: rehearse the whole flow at home and develop the Fire screen without a radar. Simulated data must never mix with real data.

- A marker file `simulator.flag` in the app's documents folder means the simulator is on. Settings creates or removes it, then rebuilds `AppServices` and returns to Home.
- With the simulator on, the app uses a second database file, `sentinel_demo.db`, and the simulated sources. The real database is not opened.
- When `sentinel_demo.db` is created it is filled with one product "Demo", one lot "DEMO" and one box `D01` whose 50 cartridges all have measurements (weight between 3.30 and 3.40 g, rim thickness between 1.00 and 1.08 mm).
- Every app bar shows the word "SIMULATOR" in amber.
- The Fire screen shows two extra controls: "Simulate shot" (sends a shot with a frequency between 50900 and 52400 and a BC between 0.120 and 0.140) and a "Dongle on / off" switch. While on, the simulated dongle sends a reading immediately and then every 10 seconds.

## 13. Build steps

Run `flutter analyze` (no issues) and `flutter test` (all pass) at the end of every step, in addition to that step's list.

### Step 1: domain logic

Create `lib/domain/` with: constants, the clock, the models (product, lot, box, cartridge, session, shot, reading), slot label maths (rules G), decimal parser and measurement warnings (rules M2, M3), radar packet decoder and dongle payload decoder (section 10), the weather tracker (rules W1, W2), the shot-context calculator (rules S3 to S7 as pure functions of "the earlier shots"), and the CSV builder (section 12.1, from a list of row objects to text). Leave the existing files untouched in this step.

Done when:
- Tests T1, T3, T5a, T5b and T6c of section 14 exist and pass.
- No file in `lib/domain/` imports Flutter or a plugin.

### Step 2: database and repositories

Add the packages of section 6. Create `lib/data/`: the database (section 8), and repositories for boxes and cartridges, for the magazine and shots (rules L and F1 to F4, F6, F7), for sessions (rules S1, S3) and for settings. The database takes its file path as a parameter so tests can use a temporary file.

Done when:
- The first test written opens an in-memory database through `sqflite_common_ffi` and passes. If the SQLite library fails to load on the PC, stop and report it to the user.
- Tests T2, T4 and T6a exist and pass.

### Step 3: Home, Boxes, Measure

Create `AppServices`, `AppScope`, the Home, Boxes and Measure screens and their controllers, and the `BoxGrid` widget. Rewrite `main.dart` and `screens/home_screen.dart`, then delete `screens/radar_screen.dart`. Until step 6 the Fire entry shows a placeholder page, and until step 7 so does the Data entry. `BoxGrid` receives, for each slot, a label, a colour, a state and whether it can be tapped, so the Fire screen and phase 2 can reuse it.

Done when:
- On the AYN Thor: a box can be created; 50 cartridges can be measured in sequence without touching anything but the number keys and Save; a comma works as decimal separator; an out-of-range value shows "Save anyway"; after closing and reopening the app every value is still there.
- The user has been told the app is ready for measuring, since measuring at home does not depend on the later steps.

### Step 4: sources

Create `lib/sources/`: the two interfaces, the simulated versions, the scanner, the Bluetooth radar source and the Bluetooth weather source. Move the radar logic out of `lib/radar/radar.dart` (fixing the defect described in section 5), then delete that file and `lib/models/shot.dart`. Add the Settings screen with the Bluetooth check.

Done when:
- On the AYN Thor, with the real dongle powered: the Bluetooth check shows a reading whose age returns to a few seconds at least every 10 seconds.
- With the real radar on: the check shows "connected". The user fires or triggers one shot and it appears once. After the radar is switched off and on again it reconnects by itself and the old shot does not appear a second time.
- If either device cannot be seen, stop and report to the user.

### Step 5: simulator

Implement section 12.4.

Done when:
- Switching the simulator on shows the "SIMULATOR" label and the demo box; switching it off shows the real boxes again, unchanged.
- The real database file has the same size and modification time before and after a simulator session.

### Step 6: Fire screen

Create the Fire screen, its controller (with the single queue of rule F5), the `MagazineStack`, weather line and radar status widgets, the session start question and the manual temperature dialog. Keep the screen on with `wakelock_plus`.

Done when, in the simulator on the AYN Thor:
- Tapping cartridges fills the magazine from the bottom, with position numbers on the grid and origin labels on the stack.
- Tapping the stack returns the top cartridge to its slot.
- Each simulated shot removes the top cartridge, turns its slot green and shows the velocity. A full magazine of 10 can be fired with no tap between shots.
- A shot with an empty magazine raises the unassigned counter.
- Undo returns the last cartridge to the top.
- With the simulated dongle off for more than 30 seconds the weather line is red, and shots are still recorded.
- Killing the app with 4 cartridges loaded and reopening it shows the same 4 in the same order.
- The first opening of the day asks the cleaning question; the second does not.

### Step 7: Data, export, backup, import

Create the Data screens and the export, backup and import services (sections 11.5, 12.1 to 12.3). The backup service takes folder paths as parameters so tests can use a temporary folder.

Done when:
- Test T6b exists and passes.
- On the AYN Thor: "Export all shots" opens the share menu and the saved file opens in a spreadsheet with the columns of section 12.1 in order.
- A backup file appears in the backups folder after leaving the Fire screen with new shots.
- Importing an exported backup restores the data it contained.

### Step 8: finish

Confirm that nothing on Home leads to the Sentinel screens and that their files are unchanged. Update `app/README.md` (what the app does, how to build from the C: copy, the backup limit of section 12.2) and add an entry to the repository `CHANGELOG.md`. Set the version in `pubspec.yaml` to `0.2.0+2`.

Done when:
- The checks of section 15 have been done by the user with the real radar and dongle, and the user confirms them.

## 14. Automated tests

These are the behaviours the user confirmed as critical. Name each test after the rule it proves.

| Id | Proves | Where |
|----|--------|-------|
| T1 | Radar decoding: a valid 9-byte packet gives the right frequency, BC and velocity (use rows of the table in `re/RADAR_PROTOCOL.md`); wrong length, wrong first byte, wrong last byte and zero frequency are rejected. | domain |
| T2 | Shot assignment, rules L2, L3, F2, F3, F6, F7: a shot goes to the top; unloading returns the top to its slot; a shot with an empty magazine is unassigned; undo returns the cartridge to the top; undo is refused with a full magazine; a load is refused for an unmeasured cartridge and for an 11th cartridge. | data |
| T3 | Slot numbering, rules G1 to G3: for 5 by 10, slot 1 is `R1 C1`, slot 10 is `R1 C10`, slot 11 is `R2 C1`, slot 14 is `R2 C4`, slot 50 is `R5 C10`; also one check with a 10 by 5 layout. | domain |
| T4 | Session counters, rules S3 to S8: cleaned session starts at 1; uncleaned session continues from the previous session; cold bore only on the first shot; time since last shot from a fake clock; a voided shot is skipped by all counters; shot_index is never reused. | data |
| T5a | Dongle decoding: a valid 20-byte payload gives the right values; wrong length and out-of-range values are rejected. | domain |
| T5b | Weather tracker, rules W1 to W3: a shot gets the latest reading; at 30 seconds it is still current, just after it is not; with no reading and a manual temperature the source is `manual`; with neither it is `none`. | domain |
| T6a | Nothing lost on restart, rule L6: load three cartridges into a database file, close it, open it again; the magazine has the same three in the same order. | data |
| T6b | Backup round trip: make a backup, change the data, import the backup; the data equals the state at backup time. Importing a file that is not a backup changes nothing. | data |
| T6c | CSV: the header matches section 12.1 exactly; a voided row and an unassigned row are written correctly; a value containing a comma is quoted; decimals use a dot. | domain |

The real Bluetooth links and the visual design are checked by hand (section 15).

## 15. Check on the real device

Done by the user at the end, with the real radar and dongle, simulator off:

1. Measure five cartridges of a real box; values survive an app restart.
2. Open Fire: the cleaning question appears; the radar connects without any tap; the weather line shows the dongle values.
3. Load three cartridges; the stack and the grid agree with the real magazine and box.
4. Fire three shots without touching the phone; each one is attached to the right cartridge.
5. Switch the dongle off; within about 30 seconds the weather line turns red; a shot is still recorded.
6. Switch the radar off and on; it reconnects; no extra shot appears.
7. Export the CSV and open it on the PC; the three shots are there with measurements, frequency, BC, weather and times.

## 16. Out of scope and known issues

Not part of phase 1. Leave them alone unless the user asks:

- Predicting velocity, forming groups, colouring the grid by group, expected mean velocity and BC of a group. The two reserved cartridge columns and the reusable `BoxGrid` are the only preparation.
- The "send to rifle" button and any change to the Sentinel screens or firmware.
- Sending data to a server, accounts, payment, protection against copying.
- Attaching an unassigned shot to a cartridge afterwards.
- Entering overall length (`oal_mm`).
- iOS.

Known issues for later phases, recorded here so they are not forgotten:

- Device firmware: writing only velocity and BC to the Sentinel currently clears its dongle binding, because a missing `dongle_mac` field is treated as "unbind" (`src/ble_server.cpp`, `configFromJson`).
- The drag model behind the radar's BC value is not confirmed. It must match the drag table set on the device before the BC is sent to it.
- A paid release needs the commercial licence of `flutter_blue_plus`, and depends on the radar's undocumented protocol.
