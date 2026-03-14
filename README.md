# Ballistic Sentinel

A live ballistic calculator for long-range shooting, built on the ESP32-WROOM-32 platform.
The complete C++ ballistic engine is a from-scratch rewrite of [py_ballisticcalc](../bin/py_ballisticcalc),
validated to within 0.25% of the Python reference across 8 scenarios and 173 trajectory points.

## Features

- **Pure C++ ballistic engine** — header-only library with RK4 integration, PCHIP interpolation,
  CIPM-2007 atmosphere model, spin drift (Litz formula), Miller stability, Coriolis-ready (disabled pending magnetometer calibration)
- **All 9 standard drag tables**: G1, G7, G2, G5, G6, G8, GI, GS, RA4
- **Multi-BC support** — velocity-stepped ballistic coefficients
- **Four operating modes**:
  - **Live** — dial distance with a combination-lock style 4-digit display (yards)
  - **Staged** — pre-programmed target stages (PRS-style matches, up to 20)
  - **Sensor View** — real-time display of all sensor readings
  - **Digital Level** — full-screen cant readout with graphical bubble level, "LEVEL" indicator + display inversion when within sensitivity threshold, directional tilt hints when off-level
- **Real-time sensors** — BME280 (temperature/pressure/humidity), QMC5883P (compass heading), MPU6050 (cant/roll detection)
- **Calypso ultrasonic anemometer** — NMEA 0183 wind input (prepared, UART2)
- **Cant detection** — MPU6050 accelerometer with auto-calibration, visual cant slider, display auto-invert when level
- **SH1106 1.3" OLED** — 128×64 display with header, distance, corrections, sensor bar, scrolling menu with arrow indicators
- **5-way navigation button** — debounced with double-press, long-press (deep sleep), and auto-repeat
- **WiFi Access Point** — configure rifle/stages via a dark-themed web UI with captive portal
- **Persistent storage** — NVS for rifle config, stage definitions, and cant calibration
- **Power management** — auto-dim after 2 min inactivity (~80% brightness reduction), accelerometer-based auto deep-sleep after 10 min with no motion, ESP32 light sleep between sensor intervals, event-driven ballistic recalculation (on distance change only), BME280 forced mode, compass ODR reduced to 10 Hz, WiFi modem sleep when AP active
- **Battery monitoring** — LiPo voltage via ADC1 (GPIO36) with voltage divider, piecewise-linear discharge curve, battery icon in OLED header on all screens
- **Unit preferences** — configurable imperial/metric units for distance, velocity, weight, length, temperature, and pressure

## Hardware

### Bill of Materials

| Component | Part | Notes |
|-----------|------|-------|
| MCU | ESP32-WROOM-32 (classic) | 520KB SRAM, 4MB Flash |
| Display | 1.3" OLED SH1106 I2C | 128×64, addr 0x3C |
| Env sensor | BME280 | Temp/press/humidity, addr 0x76 |
| Compass | QMC5883P (GY-271 module) | 3-axis magnetometer, addr 0x2C |
| Accelerometer | MPU6050 | Cant/roll detection, addr 0x68 |
| Button | 5-way digital navigation | One GPIO per direction + center |
| Wind sensor | Calypso ultrasonic (optional) | NMEA 0183 at 4800 baud |
| LED | On-board ESP32 LED | GPIO 2 |
| Battery | 3.7V LiPo (e.g. 2000mAh) | Requires 2× 100kΩ voltage divider |
| Resistors | 2 × 100kΩ | Voltage divider for battery ADC |

### Pin Map

| GPIO | Function | Direction | Notes |
|------|----------|-----------|-------|
| **21** | I2C SDA | Bidirectional | Shared bus: OLED + BME280 + QMC5883P + MPU6050 |
| **22** | I2C SCL | Output | 400 kHz Fast-mode |
| **32** | Button UP | Input (pull-up) | Active-low |
| **33** | Button DOWN | Input (pull-up) | Active-low |
| **25** | Button LEFT | Input (pull-up) | Active-low |
| **26** | Button RIGHT | Input (pull-up) | Active-low |
| **27** | Button CENTER | Input (pull-up) | Active-low, long-press (5s) = deep sleep, double-press = back |
| **16** | Wind UART2 RX | Input | ← Calypso TX (prepared) |
| **17** | Wind UART2 TX | Output | → Calypso RX (prepared) |
| **2** | Status LED | Output | Solid = WiFi AP active |
| **36** | Battery ADC (VP) | Input | Via 100kΩ+100kΩ voltage divider, ADC1_CH0 |

### I2C Bus Addresses

| Address | Device | Protocol |
|---------|--------|----------|
| 0x3C | SH1106 OLED | U8g2 HW I2C |
| 0x76 | BME280 | Adafruit BME280 |
| 0x2C | QMC5883P | 3-axis magnetometer |
| 0x68 | MPU6050 | Adafruit MPU6050 |

### Wiring Diagram

```
ESP32-WROOM-32
├── GPIO 21 (SDA) ──┬── OLED SDA
│                    ├── BME280 SDA
│                    ├── QMC5883P SDA
│                    └── MPU6050 SDA
├── GPIO 22 (SCL) ──┬── OLED SCL
│                    ├── BME280 SCL
│                    ├── QMC5883P SCL
│                    └── MPU6050 SCL
├── GPIO 32 ──── BTN UP
├── GPIO 33 ──── BTN DOWN
├── GPIO 25 ──── BTN LEFT
├── GPIO 26 ──── BTN RIGHT
├── GPIO 27 ──── BTN CENTER
├── GPIO 16 ──── Calypso TX (RX2)
├── GPIO 17 ──── Calypso RX (TX2)
├── GPIO 36 (VP) ── Battery voltage divider mid-point
│                    (Batt+ ─[100kΩ]─ GPIO36 ─[100kΩ]─ GND)
└── GPIO  2 ──── On-board LED
```

## Software Architecture

```
Ballistic Sentinel/
├── lib/ballistic/          # Header-only C++ ballistic library
│   ├── ballistic.h         # Umbrella include
│   ├── constants.h         # Physical constants, unit conversions
│   ├── vector3d.h          # 3D vector math
│   ├── drag_tables.h       # 9 drag tables (G1/G7/G2/G5/G6/G8/GI/GS/RA4)
│   ├── interpolation.h     # PCHIP Fritsch-Carlson spline
│   ├── atmosphere.h        # CIPM-2007 air density model
│   ├── drag_model.h        # Single & multi-BC drag models
│   ├── trajectory.h        # TrajectoryPoint, CorrectionResult
│   ├── engine_rk4.h        # RK4 integration engine, Ridder zero-finding
│   └── calculator.h        # High-level API
├── src/                    # ESP32 firmware
│   ├── config.h            # Pin assignments, timing constants
│   ├── display.h/.cpp      # U8g2 SH1106 OLED driver
│   ├── input.h/.cpp        # 5-way button with debounce/repeat
│   ├── sensors.h/.cpp      # BME280 + QMC5883P + MPU6050 polling
│   ├── storage.h/.cpp      # NVS persistent config
│   ├── web_ui.h            # PROGMEM HTML/CSS/JS
│   ├── webserver.h/.cpp    # ESPAsyncWebServer REST API
│   ├── wind.h/.cpp         # Calypso NMEA parser
│   ├── modes.h/.cpp        # Live/Staged/Sensor mode manager
│   └── main.cpp            # Firmware entry point
├── test/
│   ├── test_native/        # Unity C++ unit tests (37 cases)
│   └── test_compare/       # C++ output for Python comparison
└── platformio.ini          # Build configuration
```

### Ballistic Library API

```cpp
#include <ballistic.h>
using namespace ballistic;

Calculator calc;
calc.configure(0.315, DragTableId::G7,  // BC, drag table
               140, 0.264, 1.35,        // weight_gr, diameter_in, length_in
               2710, 1.5, 8.0, 100);    // mv_fps, sight_in, twist_in, zero_yd
calc.setAtmosphere(59.0, 29.92, 0.0);   // temp_F, pressure_inHg, humidity 0-1
calc.setWind(10.0, 90.0);               // speed_fps, direction_deg
calc.solve();                            // find zero elevation

auto corr = calc.correction(800);        // at 800 yards
// corr.vertical, corr.horizontal, corr.velocity_fps, etc.

auto traj = calc.trajectory(2000, 100);  // max range, step (yards)
for (auto& pt : traj) {
    // pt.distance_ft, pt.height_ft, pt.velocity_fps, pt.mach, ...
}
```

## Building

### Prerequisites

- [PlatformIO CLI](https://platformio.org/install/cli)
- ESP32 board support (auto-installed by PlatformIO)

### Flash Firmware

```bash
cd "Ballistic Sentinel"
pio run -e upesy_wroom -t upload
```

### Run Native Tests

```bash
cd "Ballistic Sentinel"
pio test -e native
```

Expected: **37/37 PASS**

### Run Comparison Tests

Requires `py_ballisticcalc` installed in a Python environment and `g++` in PATH:

```bash
cd <project_root>
python tests/test_compare.py
```

Expected: **173/173 PASS** (8 scenarios × up to 31 points each)

## Web Interface

1. Select **WiFi Toggle** from the main menu to start WiFi AP
2. Connect to **BallisticSentinel** (password: `longrange`)
3. Open `http://192.168.4.1` in any browser (captive portal auto-redirects)
4. Configure rifle parameters, unit preferences, and stage targets
5. Select **WiFi Toggle** again to turn off WiFi

The web UI uses a dark theme (`#0d1117` background) with green accents (`#16c79a`)
and a Courier New monospace font. Includes unit conversion (imperial/metric),
multi-BC configuration, powder sensitivity calculator, cant calibration, and
correction unit selection (MOA, SMOA, MRAD, CM, CLICKS).

### API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Web configuration UI |
| GET | `/api/config` | Current rifle + stage config (JSON) |
| POST | `/api/save` | Save rifle + stage config |
| POST | `/api/wifi/off` | Shut down WiFi AP |
| POST | `/api/cant/calibrate` | Start cant auto-calibration |

## Test Coverage

### Native C++ Tests (37 cases)

| Group | Tests | Description |
|-------|-------|-------------|
| Constants | 3 | Gravity, std atmosphere, unit conversions |
| Vector3 | 5 | Addition, magnitude, normalize, scalar, dot |
| PCHIP | 2 | Basic interpolation, G7 table accuracy |
| Atmosphere | 5 | Standard, hot, altitude, humidity, unit conversion |
| Drag Model | 2 | Single BC, all 9 tables exist |
| Zero-Finding | 2 | 100 yd, 200 yd zero |
| Trajectory | 3 | Drop increases, velocity decreases, reasonable values |
| Calibers | 4 | .308 G7, .338LM G7, .50BMG G1, 6mm CM G7 |
| Weather | 2 | Hot vs cold, altitude effect |
| Wind | 2 | Crosswind deflection, no-wind spin drift |
| Coriolis | 1 | Effect at 2000 yd |
| Corrections | 2 | MOA/MRAD consistency, click calculations |
| Advanced | 2 | Powder sensitivity, multi-BC |
| Edge Cases | 2 | Zero-distance correction, very close range |

### Python Comparison (173 points, 8 scenarios)

| Scenario | Drag | Range | Conditions |
|----------|------|-------|------------|
| 6.5 Creedmoor | G7 | 2000 yd | Standard atmosphere |
| .308 Winchester | G7 | 1500 yd | 10 mph crosswind |
| .338 Lapua Magnum | G7 | 2500 yd | Hot (100°F), humid |
| .50 BMG | G1 | 2500 yd | Standard atmosphere |
| 6mm Creedmoor | G7 | 1500 yd | Cold (20°F) |
| .300 PRC | G7 | 2000 yd | High altitude (≈5000 ft) |
| .308 Winchester | G1 | 1500 yd | G1 reference |
| .375 CheyTac | G7 | 3000 yd | Extreme long range |

Tolerance: max(0.5"/500yd × distance, 0.25% of absolute value)

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2025-01-XX | Initial release — complete C++ ballistic engine, ESP32 firmware, web UI, 37 native tests, 173-point Python comparison |
| 1.1.0 | 2026-03-13 | Power optimization: event-driven ballistic calc, BME280 forced mode, compass 10 Hz ODR, display 4 fps, split sensor intervals (cant 5 Hz / env 0.5 Hz), ESP32 light sleep, WiFi modem sleep, auto-dim (2 min / 80%), accelerometer-based auto deep-sleep (10 min / 0.43g threshold). Bug fixes: deep-sleep GPIO wakeup loop, wake-from-sleep phantom button press, display inversion persisting on shutdown/wake screens |
| 1.2.0 | 2026-03-13 | New Digital Level mode with full-screen cant readout, graphical bubble level, sensitivity zone marks, "LEVEL" indicator with display inversion, and directional tilt hints. Scrolling main menu with up/down arrow indicators (supports 5+ items without shrinking text). WiFi toggle moved to last menu position |

## License

Internal project — not for distribution.
