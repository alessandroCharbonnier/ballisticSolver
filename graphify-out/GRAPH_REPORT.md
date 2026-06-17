# Graph Report - .  (2026-06-17)

## Corpus Check
- 65 files · ~88,593 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1311 nodes · 4187 edges · 81 communities (53 shown, 28 thin omitted)
- Extraction: 52% EXTRACTED · 48% INFERRED · 0% AMBIGUOUS · INFERRED: 2017 edges (avg confidence: 0.51)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- [[_COMMUNITY_Integration Engines|Integration Engines]]
- [[_COMMUNITY_Conditions & Munition Models|Conditions & Munition Models]]
- [[_COMMUNITY_Firmware Main Logic|Firmware Main Logic]]
- [[_COMMUNITY_UI & Display Systems|UI & Display Systems]]
- [[_COMMUNITY_Persistent Storage (NVS)|Persistent Storage (NVS)]]
- [[_COMMUNITY_Atmospheric Models (CIPM-2007)|Atmospheric Models (CIPM-2007)]]
- [[_COMMUNITY_Interpolation (PCHIP)|Interpolation (PCHIP)]]
- [[_COMMUNITY_Hardware Sensors (I2C)|Hardware Sensors (I2C)]]
- [[_COMMUNITY_Community 8|Community 8]]
- [[_COMMUNITY_Community 9|Community 9]]
- [[_COMMUNITY_Community 10|Community 10]]
- [[_COMMUNITY_Community 11|Community 11]]
- [[_COMMUNITY_Community 12|Community 12]]
- [[_COMMUNITY_Community 13|Community 13]]
- [[_COMMUNITY_Community 14|Community 14]]
- [[_COMMUNITY_Community 15|Community 15]]
- [[_COMMUNITY_Community 16|Community 16]]
- [[_COMMUNITY_Community 17|Community 17]]
- [[_COMMUNITY_Community 18|Community 18]]
- [[_COMMUNITY_Community 19|Community 19]]
- [[_COMMUNITY_Community 20|Community 20]]
- [[_COMMUNITY_Community 21|Community 21]]
- [[_COMMUNITY_Community 22|Community 22]]
- [[_COMMUNITY_Community 23|Community 23]]
- [[_COMMUNITY_Community 24|Community 24]]
- [[_COMMUNITY_Community 25|Community 25]]
- [[_COMMUNITY_Community 26|Community 26]]
- [[_COMMUNITY_Community 27|Community 27]]
- [[_COMMUNITY_Community 28|Community 28]]
- [[_COMMUNITY_Community 29|Community 29]]
- [[_COMMUNITY_Community 30|Community 30]]
- [[_COMMUNITY_Community 31|Community 31]]
- [[_COMMUNITY_Community 32|Community 32]]
- [[_COMMUNITY_Community 33|Community 33]]
- [[_COMMUNITY_Community 34|Community 34]]
- [[_COMMUNITY_Community 35|Community 35]]
- [[_COMMUNITY_Community 36|Community 36]]
- [[_COMMUNITY_Community 37|Community 37]]
- [[_COMMUNITY_Community 38|Community 38]]
- [[_COMMUNITY_Community 39|Community 39]]
- [[_COMMUNITY_Community 40|Community 40]]
- [[_COMMUNITY_Community 41|Community 41]]
- [[_COMMUNITY_Community 42|Community 42]]
- [[_COMMUNITY_Community 43|Community 43]]
- [[_COMMUNITY_Community 44|Community 44]]
- [[_COMMUNITY_Community 45|Community 45]]
- [[_COMMUNITY_Community 46|Community 46]]
- [[_COMMUNITY_Community 47|Community 47]]
- [[_COMMUNITY_Community 48|Community 48]]
- [[_COMMUNITY_Community 49|Community 49]]
- [[_COMMUNITY_Community 50|Community 50]]
- [[_COMMUNITY_Community 51|Community 51]]
- [[_COMMUNITY_Community 52|Community 52]]
- [[_COMMUNITY_Community 53|Community 53]]
- [[_COMMUNITY_Community 54|Community 54]]
- [[_COMMUNITY_Community 55|Community 55]]
- [[_COMMUNITY_Community 56|Community 56]]
- [[_COMMUNITY_Community 57|Community 57]]
- [[_COMMUNITY_Community 58|Community 58]]
- [[_COMMUNITY_Community 59|Community 59]]
- [[_COMMUNITY_Community 60|Community 60]]
- [[_COMMUNITY_Community 61|Community 61]]
- [[_COMMUNITY_Community 62|Community 62]]
- [[_COMMUNITY_Community 63|Community 63]]
- [[_COMMUNITY_Community 64|Community 64]]
- [[_COMMUNITY_Community 65|Community 65]]
- [[_COMMUNITY_Community 66|Community 66]]
- [[_COMMUNITY_Community 67|Community 67]]
- [[_COMMUNITY_Community 68|Community 68]]
- [[_COMMUNITY_Community 69|Community 69]]
- [[_COMMUNITY_Community 70|Community 70]]
- [[_COMMUNITY_Community 71|Community 71]]
- [[_COMMUNITY_Community 74|Community 74]]
- [[_COMMUNITY_Community 75|Community 75]]
- [[_COMMUNITY_Community 76|Community 76]]
- [[_COMMUNITY_Community 77|Community 77]]
- [[_COMMUNITY_Community 78|Community 78]]
- [[_COMMUNITY_Community 79|Community 79]]
- [[_COMMUNITY_Community 80|Community 80]]

## God Nodes (most connected - your core abstractions)
1. `Distance` - 174 edges
2. `Angular` - 160 edges
3. `HitResult` - 143 edges
4. `TrajFlag` - 139 edges
5. `TrajectoryData` - 133 edges
6. `Vector` - 128 edges
7. `ShotProps` - 115 edges
8. `PreferredUnits` - 103 edges
9. `Shot` - 96 edges
10. `RangeError` - 83 edges

## Surprising Connections (you probably didn't know these)
- `Live Mode` --implemented_by--> `Mode Manager`  [INFERRED]
  README.md → src/modes.cpp
- `Mode Manager` --manages--> `Staged Mode`  [INFERRED]
  src/modes.cpp → README.md
- `compute_multi_bc_trajectory()` --calls--> `DragModelMultiBC()`  [INFERRED]
  test/test_compare.py → bin/py_ballisticcalc/drag_model.py
- `PreferredUnitsMeta` --inherits--> `type`  [EXTRACTED]
  bin/py_ballisticcalc/unit.py → src/web_ui.h
- `Firmware Entry Point` --runs_on--> `ESP32-WROOM-32`  [EXTRACTED]
  src/main.cpp → README.md

## Import Cycles
- None detected.

## Communities (81 total, 28 thin omitted)

### Community 0 - "Integration Engines"
Cohesion: 0.13
Nodes (55): BaseEngineConfigDict, bool, float, HitResult, int, ShotProps, TrajFlag, BaseEngineConfigDict (+47 more)

### Community 1 - "Conditions & Munition Models"
Cohesion: 0.07
Nodes (45): BaseTrajDataAttribute, Angular, bool, str, Vector, Velocity, str, Velocity (+37 more)

### Community 2 - "Firmware Main Logic"
Cohesion: 0.07
Nodes (27): bool, float, Self, NotImplementedType, Protocol, Comparable, GenericDimension, In-place division by a number: `this /= other`.          Returns: (+19 more)

### Community 3 - "UI & Display Systems"
Cohesion: 0.05
Nodes (27): Distance, float, GenericDimension, ShotProps, str, int, Get the human-readable name for a trajectory flag value.          Converts a n, Synonym for .distance. (+19 more)

### Community 4 - "Persistent Storage (NVS)"
Cohesion: 0.11
Nodes (40): Angular, BaseTrajData, bool, Distance, float, HitResult, int, Shot (+32 more)

### Community 5 - "Atmospheric Models (CIPM-2007)"
Cohesion: 0.11
Nodes (40): bool, str, Unit, Any, GenericDimension, GenericDimensionT, BCPoint, Ballistic coefficient point for multi-BC drag models.      Represents a single (+32 more)

### Community 6 - "Interpolation (PCHIP)"
Cohesion: 0.07
Nodes (27): cfg(), namespace, Display(), class, applyConfig(), loop(), updateDisplay(), begin() (+19 more)

### Community 7 - "Hardware Sensors (I2C)"
Cohesion: 0.07
Nodes (16): ASSERT_NEAR(), test_atmosphere_standard(), test_atmosphere_unit_conversions(), test_constants_conversions(), test_correction_clicks(), test_correction_units_consistency(), test_drag_model_single_bc(), test_pchip_basic() (+8 more)

### Community 8 - "Community 8"
Cohesion: 0.07
Nodes (16): CorrectionResult, CorrectionUnit, drawArrowDown(), drawArrowLeft(), drawArrowRight(), drawArrowUp(), drawCantSlider(), drawCorrections() (+8 more)

### Community 9 - "Community 9"
Cohesion: 0.07
Nodes (20): float, Atmo, Altitude relative to sea level., Powder temperature (falls back to ambient when unspecified)., Local speed of sound (Mach 1)., Ratio of local density to standard density (dimensionless)., Relative humidity as fraction [0..1]., Set relative humidity.          Accepts either a fraction [0..1] or percent [0 (+12 more)

### Community 10 - "Community 10"
Cohesion: 0.09
Nodes (20): str, Euler integration engine for ballistic trajectory calculations.  The Euler met, Integration engines for ballistic trajectory calculations.  This package provi, Runge-Kutta 4th order integration engine for ballistic trajectory calculations., Velocity Verlet integration engine for ballistic trajectory calculations.  The, Environmental conditions used by ballistic engines.  What this module provides, Global physical and atmospheric constants for ballistic calculations.  This mo, Ballistics calculator interface and engine loading system.  This module provid (+12 more)

### Community 11 - "Community 11"
Cohesion: 0.09
Nodes (28): bool, DataFrame, HitResult, Angular, Axes, bool, HitResult, int (+20 more)

### Community 12 - "Community 12"
Cohesion: 0.09
Nodes (25): Distance, float, str, DragTableDataType, DragModel, DragModelMultiBC(), linear_interpolation(), make_data_points() (+17 more)

### Community 13 - "Community 13"
Cohesion: 0.13
Nodes (24): Ammo, Atmo, Angular, Vector, Wind, DragDataPoint, PchipPrepared, Coriolis (+16 more)

### Community 14 - "Community 14"
Cohesion: 0.12
Nodes (27): Any, float, HitResult, int, TrajectoryData, Unit, bisect_for_monotonic_condition(), BisectWrapper (+19 more)

### Community 15 - "Community 15"
Cohesion: 0.13
Nodes (24): Angular, Any, bool, ConfigT, Distance, float, HitResult, int (+16 more)

### Community 16 - "Community 16"
Cohesion: 0.09
Nodes (21): ABC, Vector, create_base_engine_config(), Base integration engine for ballistic trajectory calculations.  The module ser, Create BaseEngineConfig from optional dictionary configuration.      This fact, Winds in effect down range.      Currently this class assumes that requests fo, Initialize the _WindSock class.          Args:             winds: A sequence, Return the current cached wind vector.          Raises:             RuntimeEr (+13 more)

### Community 17 - "Community 17"
Cohesion: 0.12
Nodes (18): Angular, Distance, float, ColoredFormatter, Weapon and ammunition configuration for ballistic calculations.  This module p, Initialize a Sight instance with given parameters.          Args:, Calculate SFP reticle steps for target distance and magnification.          Fo, Calculate sight adjustment for target distance and magnification.          Thi (+10 more)

### Community 18 - "Community 18"
Cohesion: 0.16
Nodes (24): Path, compare_rows(), compile_cpp_binary(), compute_multi_bc_trajectory(), compute_powder_sens_trajectory(), compute_python_trajectory(), get_drag_table(), main() (+16 more)

### Community 19 - "Community 19"
Cohesion: 0.16
Nodes (20): BaseException, Angular, Any, bool, ConfigT, Distance, float, HitResult (+12 more)

### Community 20 - "Community 20"
Cohesion: 0.16
Nodes (19): Angular, bool, Distance, float, HitResult, ShotProps, TrajFlag, BaseEngineConfig (+11 more)

### Community 21 - "Community 21"
Cohesion: 0.15
Nodes (18): SensorData, adjustDigit(), begin(), compute(), configureCaclulator(), ButtonState, RifleConfig, StageConfig (+10 more)

### Community 22 - "Community 22"
Cohesion: 0.13
Nodes (18): bool, Distance, Shot, EngineProtocol, Self, Calculator, must_fire(), Wrapper function to resolve RangeError and get HitResult. (+10 more)

### Community 23 - "Community 23"
Cohesion: 0.17
Nodes (20): async, AsyncWebServerRequest, function, id, $(), clickUnitChanged(), disableWifi(), type (+12 more)

### Community 24 - "Community 24"
Cohesion: 0.15
Nodes (19): float, int, _ensure_strictly_increasing(), _hermite_eval(), interpolate_2_pt(), interpolate_3_pt(), pchip_eval(), pchip_prepare() (+11 more)

### Community 25 - "Community 25"
Cohesion: 0.10
Nodes (9): Number, Shortcut for `>> Temperature.Fahrenheit`., In-place numeric addition; clamp at absolute zero., In-place numeric subtraction; clamp at absolute zero., Shortcut for `>> Time.Second`., Shortcut for `>> Velocity.FPS`., Get the numeric value of this measurement in specified units.          Args:, Get the numeric value in the defined units.          Returns:             Num (+1 more)

### Community 26 - "Community 26"
Cohesion: 0.10
Nodes (20): MultiBCScenario, bc_count, bc_pts, diameter_in, humidity, length_in, max_range_yd, mv_fps (+12 more)

### Community 27 - "Community 27"
Cohesion: 0.10
Nodes (20): PowderSensScenario, bc, diameter_in, fps_per_degf, humidity, length_in, max_range_yd, mv_fps (+12 more)

### Community 28 - "Community 28"
Cohesion: 0.11
Nodes (19): DragTableId, Scenario, bc, diameter_in, humidity, length_in, max_range_yd, mv_fps (+11 more)

### Community 29 - "Community 29"
Cohesion: 0.12
Nodes (8): str, Set preferred units from keyword arguments.          Allows bulk configuration, Readable name of the unit of measure., Short symbol of the unit of measure., Find a unit type by searching through a dictionary that maps strings to Units., Parse a unit type from a string representation.          Attempts to parse a s, Human-readable string representation of the unit measurement.          Returns, Detailed string representation for debugging.          Returns:             S

### Community 30 - "Community 30"
Cohesion: 0.21
Nodes (9): Distance, Temperature, Pressure, Initialize an `Atmo` instance.          Args:             altitude: Altitude, Station barometric pressure (not altitude adjusted)., ICAO standard temperature for altitude (valid to ~36,000 ft)., ICAO standard pressure for altitude (valid to ~36,000 ft)., Create a standard ICAO atmosphere at altitude.          Args:             alt (+1 more)

### Community 31 - "Community 31"
Cohesion: 0.15
Nodes (10): int, str, Wind, Optimized wind vector calculator using binary search for O(log n) lookups., Initialize WindSock with optimized data structures for fast lookups., String representation for debugging., Return number of wind zones., ScipyWindSock (+2 more)

### Community 32 - "Community 32"
Cohesion: 0.18
Nodes (9): float, int, Multiply vector by a scalar constant.          Args:             a: Scalar mu, Calculate the dot product (scalar product) of two vectors.          Computes t, Create a unit vector pointing in the same direction.          Returns:, Multiplication operator supporting both scalar and vector multiplication., Right multiplication operator for vector operations.          Enables multipli, In-place multiplication operator for vector operations.          Provides *= o (+1 more)

### Community 33 - "Community 33"
Cohesion: 0.19
Nodes (7): Hashable, IntEnum, Measurable, Enumeration of all supported unit types.      - Angular: Radian, Degree, MOA,, Unit, SupportsFloat, SupportsInt

### Community 34 - "Community 34"
Cohesion: 0.21
Nodes (14): ballistic(), EngineRK4(), evalZero(), recordPoint(), spinDrift(), stabilityCoefficient(), traceToDistance(), traceToPoint() (+6 more)

### Community 35 - "Community 35"
Cohesion: 0.13
Nodes (6): float, Azimuth of the shooting direction in degrees [0, 360).          Should be *geo, Latitude of the shooting location in degrees [-90, 90]., Get the air density and Mach number for a given altitude.          Args:, Litz spin-drift approximation.          Args:             time: Time of fligh, Calculate the Miller stability coefficient.          Returns:             flo

### Community 36 - "Community 36"
Cohesion: 0.24
Nodes (12): atAltitude(), ballistic(), cipmDensity(), cToF(), fToC(), fToK(), hpaToInhg(), inhgToPa() (+4 more)

### Community 37 - "Community 37"
Cohesion: 0.23
Nodes (3): BME280, BME280/BMP280 MicroPython Driver (I2C) Supports both BME280 (temp + humidity +, Returns (temperature_C, pressure_hPa, humidity_%) as a tuple.         Humidity

### Community 38 - "Community 38"
Cohesion: 0.21
Nodes (9): Angular, BaseTrajData, Distance, float, int, str, TrajectoryData, Parameters:         - zero_finding_error: The error magnitude         - iterat (+1 more)

### Community 39 - "Community 39"
Cohesion: 0.21
Nodes (10): BaseEngineConfigDict, Vector, create_scipy_engine_config(), SciPy-based integration engine for high-performance ballistic trajectory calcula, Configuration dataclass for the SciPy integration engine.      This configurat, TypedDict for flexible SciPy integration engine configuration.      This Typed, Initialize the SciPy integration engine with configuration.          Sets up t, # TODO: If _cMinimumVelocity<=0 then: either don't add this event, or always ret (+2 more)

### Community 40 - "Community 40"
Cohesion: 0.27
Nodes (6): EngineFactoryProtocolType, EntryPoint, _EngineLoader, Called by pickle for deserialization.         We manually set the fields and ca, Iterate all available engines in the entry points., Iterate over all available engines in the entry points.

### Community 41 - "Community 41"
Cohesion: 0.22
Nodes (6): ballistic(), ballistic(), ballistic(), namespace, namespace, namespace

### Community 42 - "Community 42"
Cohesion: 0.22
Nodes (4): ballistic(), ballistic(), namespace, namespace

### Community 43 - "Community 43"
Cohesion: 0.22
Nodes (4): Validate that units are compatible with this dimension.          Args:, Create a new instance from a raw value in base units.          Args:, Convert a raw value to the specified units.          Args:             raw_va, Convert a value in specified units to the raw unit.          Args:

### Community 44 - "Community 44"
Cohesion: 0.29
Nodes (4): ballistic(), buildMultiBCSpline(), BCPoint, namespace

### Community 45 - "Community 45"
Cohesion: 0.29
Nodes (4): Add two vectors component-wise.          Args:             b: The other Vecto, Addition operator for vector addition.          Provides intuitive syntax for, Right addition operator for vector addition.          Enables vector addition, In-place addition operator for vector addition.          Provides += operator

### Community 46 - "Community 46"
Cohesion: 0.33
Nodes (5): Any, Event object for SciPy solve_ivp integration with trajectory detection.      T, Call the wrapped event function with time and state parameters.          Args:, SciPyEvent, floating

### Community 47 - "Community 47"
Cohesion: 0.33
Nodes (6): ESP32-WROOM-32, Ballistic Calculator API, Live Mode, Staged Mode, Firmware Entry Point, Mode Manager

### Community 48 - "Community 48"
Cohesion: 0.33
Nodes (3): Subtract one vector from another component-wise.          Args:             b, Subtraction operator for vector subtraction.          Provides intuitive synta, In-place subtraction operator for vector subtraction.          Provides -= ope

### Community 50 - "Community 50"
Cohesion: 0.40
Nodes (4): ballistic(), kZeroVector(), namespace, Vector3

### Community 53 - "Community 53"
Cohesion: 0.70
Nodes (4): main(), runMultiBCScenario(), runPowderSensScenario(), runScenario()

## Knowledge Gaps
- **94 isolated node(s):** `str`, `int`, `ANSIColorCodes`, `str`, `namespace` (+89 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **28 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `PreferredUnitsMeta` connect `Atmospheric Models (CIPM-2007)` to `Community 10`, `Community 23`?**
  _High betweenness centrality (0.085) - this node is a cross-community bridge._
- **Why does `type` connect `Community 23` to `Atmospheric Models (CIPM-2007)`?**
  _High betweenness centrality (0.083) - this node is a cross-community bridge._
- **Why does `Distance` connect `Community 22` to `Integration Engines`, `Conditions & Munition Models`, `Firmware Main Logic`, `UI & Display Systems`, `Persistent Storage (NVS)`, `Atmospheric Models (CIPM-2007)`, `Community 9`, `Community 10`, `Community 11`, `Community 12`, `Community 13`, `Community 14`, `Community 15`, `Community 16`, `Community 17`, `Community 19`, `Community 20`, `Community 30`, `Community 31`, `Community 35`, `Community 38`, `Community 39`, `Community 40`, `Community 46`?**
  _High betweenness centrality (0.077) - this node is a cross-community bridge._
- **Are the 167 inferred relationships involving `Distance` (e.g. with `Ammo` and `Atmo`) actually correct?**
  _`Distance` has 167 INFERRED edges - model-reasoned connections that need verification._
- **Are the 153 inferred relationships involving `Angular` (e.g. with `Ammo` and `Atmo`) actually correct?**
  _`Angular` has 153 INFERRED edges - model-reasoned connections that need verification._
- **Are the 126 inferred relationships involving `HitResult` (e.g. with `BaseException` and `Angular`) actually correct?**
  _`HitResult` has 126 INFERRED edges - model-reasoned connections that need verification._
- **Are the 128 inferred relationships involving `TrajFlag` (e.g. with `Ammo` and `Atmo`) actually correct?**
  _`TrajFlag` has 128 INFERRED edges - model-reasoned connections that need verification._