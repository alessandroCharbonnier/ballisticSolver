# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Changed
- **Performance: polynomial barometric formula** — replaced `std::pow()` in `AtmoPrecomp::atAltitude()` with 5th-order Horner's polynomial (exact to double precision for trajectory altitude ranges), eliminating the most expensive per-step call on ESP32
- **Performance: dry-air density fast path** — `cipmDensity()` skips `std::exp()` and all water vapor calculations when humidity is zero (the default and most common setting)
- **Performance: hinted spline evaluation** — added `PchipSpline::evalHinted()` with O(1) amortized lookups instead of O(log n) binary search, used in all integration loops where Mach changes monotonically
- **Performance: pre-computed loop invariants** — drag coefficient factor, stability coefficient, and atmospheric base values computed once before integration instead of per-step
- **Performance: magnitudeSquared termination** — replaced `magnitude() < threshold` with `magnitudeSquared() < threshold²` in termination checks, eliminating one `sqrt()` per integration step
- **Performance: compiler flags** — added `-fno-math-errno` for ESP32 build, `-O2` for native test builds

### Added
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
