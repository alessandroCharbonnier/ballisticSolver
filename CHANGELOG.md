# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

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
- `run_cpp_binary()` now uses per-scenario step sizes instead of hardcoded 100 yd snap, supporting 25 yd (`.22 LR`), 50 yd (`PRS`), and 100 yd step intervals
- Increased C++ binary execution timeout from 60s to 300s to accommodate 75 scenarios
- Updated README trajectory point count from 173 to ~1500
