# ballistic_sentinel (phone app)

The Ballistic Sentinel phone app (Flutter, Android) lets one shooter build a
clean data set that links each .22LR cartridge's bench measurements (weight,
rim thickness) to the velocity the FX True Ballistic radar measured when it
was fired. The data set is exported as CSV and used on a PC to train a model.

Phase 1 delivers:

- **Measure** (at home): enter weight and rim thickness for every cartridge of
  a box, on a grid that looks like the box.
- **Fire** (at the range): tap cartridges to load a magazine on screen; every
  radar shot is attached to the top cartridge automatically, together with the
  weather (from the BLE weather dongle) and the barrel state.
- **Data**: review sessions, export CSV, back up and restore the database.

See `docs/COLLECT_MODE_PLAN.md` for the full specification.

## Layout

```
lib/
  main.dart
  app/          app_services.dart, app_scope.dart
  domain/       pure Dart: models, rules, decoders, CSV (no Flutter imports)
  data/         SQLite: database, repositories, export, backup, demo seed
  sources/      radar and weather inputs: Bluetooth and simulated versions
  controllers/  one ChangeNotifier per screen
  screens/      one file per screen
  widgets/      box_grid.dart, magazine_stack.dart, weather_chip.dart, ...
  ble/          existing Sentinel device code (unchanged, used in phase 2)
```

## Building

The source of truth is `Z:\embedded\ballisticCalc\app`, but Android Gradle
builds fail from the Z: network drive. Always build from a local copy:

```powershell
$SRC     = 'Z:\embedded\ballisticCalc\app'
$BUILD   = 'C:\Users\haloa\dev\ballistic_sentinel_app'
$FLUTTER = 'C:\Users\haloa\flutter\bin\flutter.bat'
$ADB     = "$env:LOCALAPPDATA\Android\Sdk\platform-tools\adb.exe"
$env:JAVA_HOME = 'C:\Program Files\Android\Android Studio\jbr'

Remove-Item "$BUILD\lib","$BUILD\test" -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item "$SRC\lib" "$BUILD\lib" -Recurse -Force
if (Test-Path "$SRC\test") { Copy-Item "$SRC\test" "$BUILD\test" -Recurse -Force }
Copy-Item "$SRC\pubspec.yaml","$SRC\pubspec.lock" $BUILD -Force
Copy-Item "$SRC\android\app\src\main\AndroidManifest.xml" "$BUILD\android\app\src\main\AndroidManifest.xml" -Force

Set-Location $BUILD
& $FLUTTER pub get
& $FLUTTER analyze
& $FLUTTER test
& $FLUTTER build apk --debug
& $ADB install -r "$BUILD\build\app\outputs\flutter-apk\app-debug.apk"
& $ADB shell am start -n com.ballisticsentinel.ballistic_sentinel/.MainActivity
```

Target device: AYN Thor (Android, landscape), adb serial `d3cb015a`.

## Notes

- Material widgets are imported from `package:material_ui/material_ui.dart`
  (`package:flutter/material.dart` is deprecated in Flutter 3.50).
- `flutter_blue_plus` requires a licence argument on `connect`; the app uses
  `License.nonprofit`.
- The simulator (Settings → Simulator) uses a second database
  (`sentinel_demo.db`) and simulated radar/dongle sources. Simulated data never
  mixes with real data.

## Backups

- A backup is a copy of the database file in a `backups` folder inside the
  app's documents folder.
- An automatic backup is made when the Fire screen is left and the session
  received at least one shot since the previous backup; the 20 newest backups
  are kept.
- **Automatic backups stay on the phone.** They protect against a damaged
  database, not against a lost phone. Only an exported backup (Data → "Export
  full backup") protects against that.

