# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Fixed
- **WiFi cycling crash** — replaced `WiFi.mode(WIFI_OFF)` with `esp_wifi_stop()` in `WebServer_::stop()` to preserve the ESP32 netif layer. `WiFi.mode(WIFI_OFF)` destroys the default AP netif but doesn't fully deregister the netstack callback, causing `netstack cb reg failed with 12308` (`ESP_ERR_INVALID_STATE`) on the next `WiFi.mode(WIFI_AP)`.
- **Web UI `/api/wifi/off` state desync** — `handleWifiOff` now sets a pending flag instead of directly calling `WiFi.softAPdisconnect()`/`WiFi.mode(WIFI_OFF)` from the async handler. Shutdown is deferred to `processDNS()` on the main loop, which calls `stop()` properly. This prevents `active_` from staying stale and avoids blocking `delay()` inside the async web server task.
- **WiFi LED/menu sync** — main loop now syncs `Modes::wifi_on_` and LED state with `WebServer_::isActive()` after each `processDNS()` call, so the menu and LED stay correct when WiFi is shut down from the web UI.

### Changed
- **Performance: float inner loop (ESP32 hardware FPU)** — all three integration methods (`traceToDistance`, `traceToPoint`, `trajectory`) now run velocity, acceleration, and RK45 stages in single-precision `float` (`Vector3f`), leveraging the ESP32 Xtensa LX6 hardware FPU. Position accumulation stays in `double` to prevent systematic drift. Each double multiply/divide on ESP32 was ~10-50× slower than float due to software emulation; with ~50+ float vector ops per RK45 step, this yields a major speedup. `sqrtf(sqrtf(x))` replaces `pow(x, 0.25)` for step rejection scaling (two hardware sqrtf vs software double pow). Float's 7 significant digits give sub-0.01 MOA accuracy per step — well below BC/atmosphere model uncertainty.
- **Performance: adaptive RK45 Dormand-Prince integrator** — replaced fixed-step RK4 (dt=0.0025s, ~1400 steps at 2000m) with adaptive Dormand-Prince RK4(5) using embedded error estimation. Automatically uses large steps (~0.01-0.02s) in smooth supersonic flight and small steps near transonic transition. Reduces step count by ~3-5× for long-range shots while maintaining accuracy via per-step error control (tolerance 1e-6 ft)
- **Performance: atmosphere amortization** — atmospheric density and speed of sound recalculated every 8 steps instead of every step, reducing CIPM density + `sqrt()` overhead by ~87% with negligible error (<0.001% density change over 8 steps)
- **Performance: reciprocal Mach pre-compute** — replaced 4× `speed / mach_local` divisions per step with 4× multiplications by pre-computed `1.0 / mach_local` (updated at atmosphere refresh). Eliminates ~6000 cycles/step on ESP32 software double FPU
- **Performance: fast magnitude** — added `Vector3::fastMagnitude()` using hardware `float` `sqrtf()` + one Newton-Raphson refinement step (~14 digits precision). Replaces `std::sqrt(double)` in inner loop (4× per step), leveraging ESP32 hardware single-precision FPU
- **Performance: polynomial barometric formula** — replaced `std::pow()` in `AtmoPrecomp::atAltitude()` with 5th-order Horner's polynomial (exact to double precision for trajectory altitude ranges), eliminating the most expensive per-step call on ESP32
- **Performance: dry-air density fast path** — `cipmDensity()` skips `std::exp()` and all water vapor calculations when humidity is zero (the default and most common setting)
- **Performance: hinted spline evaluation** — added `PchipSpline::evalHinted()` with O(1) amortized lookups instead of O(log n) binary search, used in all integration loops where Mach changes monotonically
- **Performance: pre-computed loop invariants** — drag coefficient factor, stability coefficient, and atmospheric base values computed once before integration instead of per-step
- **Performance: magnitudeSquared termination** — replaced `magnitude() < threshold` with `magnitudeSquared() < threshold²` in termination checks, eliminating one `sqrt()` per integration step
- **Performance: compiler flags** — added `-fno-math-errno` for ESP32 build, `-O2` for native test builds

### Added
- `Vector3f` single-precision 3D vector type (`vector3d.h`) with hardware `sqrtf()` magnitude, used by float inner loops
- `CoriolisParams::accelerationF()` float-precision overload for inner-loop Coriolis computation
- **87 comparison test scenarios** (up from 75) with 1850 trajectory points
- **6 multi-BC scenarios** — velocity-stepped ballistic coefficients validated across C++ and Python:
  6.5 CM Berger 140 HT, 6.5 CM 147 ELD-M (with wind), .338 LM Berger 300, .300 PRC Berger 230 (altitude),
  6mm CM A-Tip 110, .308 Win Berger 185 (with wind)
- **6 powder sensitivity scenarios** — MV-adjusted-for-temperature validated across C++ and Python:
  6.5 CM 140 ELD-M (cold 20°F + hot 110°F), .308 175 SMK (extreme cold 0°F), .338 LM 300gr (hot desert 115°F),
  6.5 CM 147 ELD-M (cold 15°F + crosswind), .300 PRC 230gr (cold high-altitude 25°F)
- `MultiBCScenario` dataclass and `compute_multi_bc_trajectory()` in Python test suite
- `PowderSensScenario` dataclass and `compute_powder_sens_trajectory()` in Python test suite
- `MultiBCScenario` / `PowderSensScenario` structs and `runMultiBCScenario()` / `runPowderSensScenario()` in C++ test suite
- `.22 LR competition bullet profiles`: SK Rifle Match, SK Long Range, Eley Tenex, Eley ELR (Contact), Lapua Center-X, Lapua Midas+, Lapua Long Range, Lapua Super Long Range, Lapua X-Act, RWS R50, CCI Standard Velocity, Federal Gold Medal Target
- Centerfire competition bullets per caliber: Berger Hybrid Target, Sierra MatchKing, Hornady ELD-M / A-Tip, Cutting Edge MTAC, and others
- Handloaded profiles for 6.5 CM (Berger 153.5 LRHT, 140 ELD-M hot, 147 ELD-M pushed), 6mm CM (Berger 115 DTAC), .308 Win (Berger 200 HT), .338 LM (Berger 300 OTM hot), .375 CT (350gr pushed), .300 PRC (Berger 230 HT)
- Weather condition matrix: standard ICAO, hot (100°F), cold (10°F), altitude (5000 ft), desert (110°F), tropical (90°F/90% RH), mountain (8500 ft cold), very high altitude (7500 ft)
- Wind variation tests: 5/10/15/20 mph crosswind, quartering 45°/135°, headwind, tailwind
- Competition distance ranges: King of 22LR (300 yd), PRS (1200 yd), King of 1 Mile (1760 yd), King of 2 Miles (3520 yd)
- Combined condition scenarios simulating real match days (altitude + wind + temperature)

### Changed
- **Refactored C++ multi-BC drag model** (`buildMultiBCSpline` in `drag_model.h`):
  replaced average-BC normalization with per-Mach form-factor folding (matching
  Python's `DragModelMultiBC`).  At runtime `drag(m) = Cd_ref(m) / BC(m)` —
  exact per-Mach BC instead of an averaged approximation.  Eliminates the
  previous 7-15 fps velocity divergence; multi-BC scenarios now pass at the
  same tight tolerances as single-BC.
- Removed multi-BC tolerance multiplier from test harness; all 1850 points use
  uniform tolerances (drop 0.65", velocity 5 fps, energy 2%).
- **Fixed C++ molar mass constant** (`atmosphere.h` CIPM-2007 density model):
  corrected `Ma` from `28.9635e-3` to `28.96546e-3` kg/mol to match the
  published CIPM-2007 standard and the Python reference.  Eliminates a ~0.007%
  systematic error in air density calculations.
- **Added Coriolis to C++ zero-finder** (`traceToDistance` in `engine_rk4.h`):
  the zero-finding integrator now includes Coriolis acceleration, matching the
  Python engine which uses its full `_integrate()` (with Coriolis) for zeroing.
  Previously the C++ zero-finder omitted Coriolis, causing a small systematic
  bias in the barrel elevation when Coriolis was enabled.
- `run_cpp_binary()` now uses per-scenario step sizes instead of hardcoded 100 yd snap, supporting 25 yd (`.22 LR`), 50 yd (`PRS`), and 100 yd step intervals
- Increased C++ binary execution timeout from 60s to 300s to accommodate 75 scenarios
- Updated README trajectory point count from 173 to ~1500
