# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- **75 comparison test scenarios** (up from 8) across `.22 LR`, `6.5 CM`, `6mm CM`, `.308 Win`, `.338 LM`, `.300 PRC`, `.375 CheyTac`, `.50 BMG`
- `.22 LR competition bullet profiles`: SK Rifle Match, SK Long Range, Eley Tenex, Eley ELR (Contact), Lapua Center-X, Lapua Midas+, Lapua Long Range, Lapua Super Long Range, Lapua X-Act, RWS R50, CCI Standard Velocity, Federal Gold Medal Target
- Centerfire competition bullets per caliber: Berger Hybrid Target, Sierra MatchKing, Hornady ELD-M / A-Tip, Cutting Edge MTAC, and others
- Handloaded profiles for 6.5 CM (Berger 153.5 LRHT, 140 ELD-M hot, 147 ELD-M pushed), 6mm CM (Berger 115 DTAC), .308 Win (Berger 200 HT), .338 LM (Berger 300 OTM hot), .375 CT (350gr pushed), .300 PRC (Berger 230 HT)
- Weather condition matrix: standard ICAO, hot (100°F), cold (10°F), altitude (5000 ft), desert (110°F), tropical (90°F/90% RH), mountain (8500 ft cold), very high altitude (7500 ft)
- Wind variation tests: 5/10/15/20 mph crosswind, quartering 45°/135°, headwind, tailwind
- Competition distance ranges: King of 22LR (300 yd), PRS (1200 yd), King of 1 Mile (1760 yd), King of 2 Miles (3520 yd)
- Combined condition scenarios simulating real match days (altitude + wind + temperature)

### Changed
- `run_cpp_binary()` now uses per-scenario step sizes instead of hardcoded 100 yd snap, supporting 25 yd (`.22 LR`), 50 yd (`PRS`), and 100 yd step intervals
- Increased C++ binary execution timeout from 60s to 300s to accommodate 75 scenarios
- Updated README trajectory point count from 173 to ~1500
