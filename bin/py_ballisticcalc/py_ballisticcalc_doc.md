# py_ballisticcalc Documentation (v2.2.8)

Ballistic trajectory calculator library for Python.  
`pip install py-ballisticcalc`

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Units System](#units-system)
3. [Core Classes](#core-classes)
   - [DragModel](#dragmodel)
   - [DragModelMultiBC / BCPoint](#dragmodelmultibc--bcpoint)
   - [Ammo](#ammo)
   - [Weapon](#weapon)
   - [Sight](#sight)
   - [Atmo](#atmo)
   - [Wind](#wind)
   - [Shot](#shot)
   - [Calculator](#calculator)
4. [Results](#results)
   - [HitResult](#hitresult)
   - [TrajectoryData](#trajectorydata)
   - [DangerSpace](#dangerspace)
   - [TrajFlag](#trajflag)
5. [Unit Reference](#unit-reference)
   - [Distance](#distance)
   - [Velocity](#velocity)
   - [Angular](#angular)
   - [Weight](#weight)
   - [Temperature](#temperature)
   - [Pressure](#pressure)
   - [Energy](#energy)
6. [PreferredUnits](#preferredunits)
7. [Integration Engines](#integration-engines)
8. [Utility Functions](#utility-functions)

---

## Quick Start

```python
from py_ballisticcalc import *
from py_ballisticcalc.unit import *

# 1. Define drag model
dm = DragModel(0.223, TableG7, Weight(175, Weight.Grain), Distance(0.308, Distance.Inch))

# 2. Define ammo
ammo = Ammo(dm, Velocity(2700, Velocity.FPS))

# 3. Define atmosphere
atmo = Atmo(
    altitude=Distance(0, Distance.Foot),
    pressure=Pressure(29.92, Pressure.InHg),
    temperature=Temperature(59, Temperature.Fahrenheit),
    humidity=0.5
)

# 4. Define weapon
weapon = Weapon(sight_height=Distance(1.5, Distance.Inch), twist=Distance(10, Distance.Inch))

# 5. Define wind
wind = [Wind(Velocity(5, Velocity.MPS), Angular(90, Angular.Degree))]

# 6. Build shot and calculate
shot = Shot(weapon=weapon, ammo=ammo, atmo=atmo, winds=wind)
calc = Calculator()
calc.set_weapon_zero(shot, Distance(100, Distance.Yard))

results = calc.fire(shot,
                    trajectory_range=Distance(1000, Distance.Yard),
                    trajectory_step=Distance(100, Distance.Yard))

# 7. Read results
for p in results:
    print(p.distance >> Distance.Yard,
          p.height >> Distance.Inch,
          p.drop_angle >> Angular.MRad,
          p.windage >> Distance.Inch,
          p.velocity >> Velocity.FPS)
```

---

## Units System

All measurement values are wrapped in typed dimension classes. Each class stores values internally in a **raw unit** and converts on demand.

### Creating a value

```python
d = Distance(100, Distance.Meter)      # constructor
d = Distance.Meter(100)                # not available for all — use constructor form
```

### Reading a value

```python
# >> operator (get_in shorthand)
meters = d >> Distance.Meter           # returns float

# .get_in() method
yards = d.get_in(Distance.Yard)        # returns float

# .convert() — returns new object in different units
d2 = d.convert(Distance.Yard)          # Distance object in yards

# << operator — returns new object without mutating
d3 = d << Distance.Foot                # new Distance in feet, d unchanged
```

### Properties

| Property     | Description                                          |
|-------------|------------------------------------------------------|
| `.raw_value` | Internal value in the dimension's base unit          |
| `.unit_value`| Value in the units it was created with               |
| `.units`     | The Unit enum it was created with                    |

### Arithmetic

Unit objects support `+`, `-`, `*`, `/` with numbers and same-dimension objects.  
Dividing two same-dimension objects returns a `float` ratio.  
**Temperature** only supports `+` and `-` (clamped at absolute zero).

---

## Core Classes

### DragModel

Aerodynamic drag model for a projectile.

```python
DragModel(
    bc: float,                    # Ballistic coefficient
    drag_table: list,             # Standard table (TableG1, TableG7, TableRA4, etc.)
    weight: float | Weight = 0,   # Bullet weight (grains) — needed for spin drift
    diameter: float | Distance = 0, # Bullet diameter (inches) — needed for spin drift
    length: float | Distance = 0   # Bullet length (inches) — needed for spin drift
)
```

**Available drag tables:** `TableG1`, `TableG2`, `TableG5`, `TableG6`, `TableG7`, `TableG8`, `TableGI`, `TableGS`, `TableRA4`

Use `get_drag_tables_names()` to list them all at runtime.

**Attributes:**
- `BC` — Ballistic coefficient
- `drag_table` — List of `DragDataPoint(Mach, CD)` pairs
- `weight`, `diameter`, `length` — Projectile dimensions
- `sectional_density` — Calculated lb/in²
- `form_factor` — Calculated dimensionless form factor

**Custom drag table:**
```python
dm = DragModel(1, [DragDataPoint(1, 0.3)])  # Constant Cd = 0.3
```

---

### DragModelMultiBC / BCPoint

Create a drag model from multiple BCs at different velocities. The library interpolates between them.

```python
DragModelMultiBC(
    bc_points: list[BCPoint],
    drag_table: list,
    weight: float | Weight = 0,
    diameter: float | Distance = 0,
    length: float | Distance = 0
) -> DragModel
```

**BCPoint:**
```python
BCPoint(BC: float, Mach: float | None = None, V: float | Velocity | None = None)
```
Specify **either** `Mach` or `V`, not both. Points are auto-sorted by Mach number.

**Example:**
```python
dm = DragModelMultiBC(
    bc_points=[
        BCPoint(BC=0.22, V=Velocity(2500, Velocity.FPS)),
        BCPoint(BC=0.21, V=Velocity(1500, Velocity.FPS)),
    ],
    drag_table=TableG7,
    weight=Weight(175, Weight.Grain),
    diameter=Distance(0.308, Distance.Inch),
)
```

---

### Ammo

Ammunition configuration.

```python
Ammo(
    dm: DragModel,
    mv: float | Velocity,                     # Muzzle velocity
    powder_temp: float | Temperature = None,   # Baseline powder temp (default 15°C)
    temp_modifier: float = 0,                  # Velocity change % per 15°C
    use_powder_sensitivity: bool = False        # Enable auto velocity adjustment
)
```

**Methods:**

| Method | Description |
|--------|-------------|
| `calc_powder_sens(other_velocity, other_temperature)` | Calculate temp sensitivity from two data points. Sets `temp_modifier`. Returns the modifier. |
| `get_velocity_for_temp(current_temp)` | Get temperature-adjusted muzzle velocity. Returns baseline velocity if `use_powder_sensitivity=False`. |

**Powder sensitivity example:**
```python
ammo = Ammo(dm, Velocity(815, Velocity.MPS),
            powder_temp=Temperature(15, Temperature.Celsius),
            use_powder_sensitivity=True)
ammo.calc_powder_sens(Velocity(830, Velocity.MPS), Temperature(30, Temperature.Celsius))
cold_vel = ammo.get_velocity_for_temp(Temperature(-10, Temperature.Celsius))
```

---

### Weapon

Weapon/sight configuration.

```python
Weapon(
    sight_height: float | Distance = None,     # Sight above bore (at muzzle)
    twist: float | Distance = None,            # Barrel twist (+ = right-hand)
    zero_elevation: float | Angular = None,    # Set by Calculator.set_weapon_zero()
    sight: Sight = None                        # Optional sight configuration
)
```

---

### Sight

Optical sight configuration for click calculations.

```python
Sight(
    focal_plane: Literal['FFP', 'SFP', 'LWIR'] = 'FFP',
    scale_factor: float | Distance = None,     # Required for SFP
    h_click_size: float | Angular = None,      # Horizontal click size
    v_click_size: float | Angular = None       # Vertical click size
)
```

**Methods:**

| Method | Description |
|--------|-------------|
| `get_adjustment(target_distance, drop_angle, windage_angle, magnification)` | Returns `SightClicks(vertical, horizontal)` |
| `get_trajectory_adjustment(trajectory_point, magnification)` | Convenience wrapper using a `TrajectoryData` point |

**SightClicks** is a named tuple: `SightClicks(vertical: float, horizontal: float)`

**Example:**
```python
weapon = Weapon(
    sight_height=Distance(1.5, Distance.Inch),
    twist=Distance(10, Distance.Inch),
    sight=Sight('FFP', h_click_size=Angular(0.1, Angular.MRad),
                       v_click_size=Angular(0.1, Angular.MRad))
)
# After calculating trajectory:
clicks = weapon.sight.get_trajectory_adjustment(results[5], magnification=10)
print(f"Up: {clicks.vertical}, Right: {clicks.horizontal}")
```

---

### Atmo

Atmospheric conditions.

```python
Atmo(
    altitude: float | Distance = None,         # Default: 0
    pressure: float | Pressure = None,         # Station pressure (default: std for altitude)
    temperature: float | Temperature = None,   # Default: std for altitude
    humidity: float = 0.0,                     # 0–1 fraction or 0–100 percent
    powder_t: float | Temperature = None       # Powder temp (default: ambient)
)
```

**Properties:** `altitude`, `pressure`, `temperature`, `humidity`, `powder_temp`, `density_ratio`, `mach`, `density_metric` (kg/m³), `density_imperial` (lb/ft³)

**Static methods:**

| Method | Description |
|--------|-------------|
| `Atmo.icao(altitude, temperature, humidity)` | Standard ICAO atmosphere at altitude |
| `Atmo.standard(...)` | Alias for `icao()` |
| `Atmo.standard_pressure(altitude)` | ICAO standard pressure at altitude |
| `Atmo.standard_temperature(altitude)` | ICAO standard temperature at altitude |
| `Atmo.machC(celsius)` | Speed of sound (m/s) at given °C |
| `Atmo.machF(fahrenheit)` | Speed of sound (fps) at given °F |
| `Atmo.machK(kelvin)` | Speed of sound (m/s) at given K |
| `Atmo.calculate_air_density(t, p_hpa, humidity)` | Air density in kg/m³ (CIPM-2007) |

**Instance methods:**

| Method | Description |
|--------|-------------|
| `get_density_and_mach_for_altitude(altitude_ft)` | Returns `(density_ratio, mach_fps)` at altitude |
| `pressure_at_altitude(altitude_ft)` | Pressure (hPa) at altitude |
| `temperature_at_altitude(altitude_ft)` | Temperature (°C) at altitude |

---

### Wind

Wind conditions for a section of the trajectory.

```python
Wind(
    velocity: float | Velocity = None,
    direction_from: float | Angular = None,   # 0=tail, 90=left-to-right
    until_distance: float | Distance = None,  # Range this wind applies to
    *, max_distance_feet: float = 1e8
)
```

**Multiple wind zones** — list sorted by until_distance:
```python
winds = [
    Wind(Velocity(3, Velocity.MPS), Angular(90, Angular.Degree),
         until_distance=Distance(300, Distance.Meter)),
    Wind(Velocity(5, Velocity.MPS), Angular(45, Angular.Degree)),  # rest of range
]
```

---

### Shot

Combines everything into a single calculation input.

```python
Shot(
    *, 
    ammo: Ammo,
    atmo: Atmo = None,                        # Default: standard ICAO
    weapon: Weapon = None,                     # Default: zero sight height
    winds: Sequence[Wind] = None,              # Default: no wind
    look_angle: float | Angular = None,        # Sight-line angle from horizontal
    relative_angle: float | Angular = None,    # Hold-over adjustment added to zero_elevation
    cant_angle: float | Angular = None,        # Gun tilt from vertical
    azimuth: float = None,                     # Bearing 0=N, 90=E (for Coriolis)
    latitude: float = None                     # Latitude in degrees (for Coriolis)
)
```

**Properties:**
- `barrel_elevation` = `look_angle + cos(cant_angle) * zero_elevation + relative_angle`
- `barrel_azimuth` — horizontal angle from cant
- `slant_angle` — synonym for `look_angle`
- `winds` — sorted by `until_distance`

---

### Calculator

Main interface. Thread-safe; creates isolated engine instances per method call.

```python
Calculator(
    *, 
    config: Any = None,         # Engine configuration dict
    engine: str | Engine = None  # Integration engine (default: RK4)
)
```

**Methods:**

| Method | Signature | Description |
|--------|-----------|-------------|
| `set_weapon_zero` | `(shot, zero_distance) -> Angular` | Sets `shot.weapon.zero_elevation` to hit at `zero_distance`. Returns the elevation angle. |
| `barrel_elevation_for_target` | `(shot, target_distance) -> Angular` | Calculate barrel elevation to hit target (does not modify weapon). |
| `fire` | `(shot, trajectory_range, trajectory_step=None, ...) -> HitResult` | Compute full trajectory. |

**`fire()` parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `shot` | Shot | required | Shot configuration |
| `trajectory_range` | Distance | required | Maximum range to compute |
| `trajectory_step` | Distance | `trajectory_range` | Step between recorded points |
| `dense_output` | bool | False | Store base data for arbitrary interpolation |
| `time_step` | float | 0.0 | Min time between recorded points (seconds) |
| `flags` | TrajFlag | NONE | Request special trajectory points |
| `raise_range_error` | bool | True | Raise error if out of range |

**Context manager support:**
```python
with Calculator(engine=RK4IntegrationEngine) as calc:
    result = calc.fire(shot, Distance(1000, Distance.Meter))
```

---

## Results

### HitResult

Returned by `Calculator.fire()`. Iterable and indexable.

```python
results = calc.fire(shot, Distance(1000, Distance.Meter), Distance(100, Distance.Meter))
len(results)        # number of trajectory points
results[5]          # 6th TrajectoryData point
for p in results:   # iterate all points
    ...
```

**Methods:**

| Method | Description |
|--------|-------------|
| `get_at(key_attribute, value)` | Interpolate trajectory at any attribute value. E.g. `get_at('distance', Distance(550, Distance.Meter))` |
| `flag(TrajFlag)` | Get first point with specific flag |
| `zeros()` | Get all zero-crossing points (requires `dense_output=True` or `flags=TrajFlag.ZERO`) |
| `danger_space(at_range, target_height)` | Calculate danger space at distance |
| `dataframe(formatted=False)` | Return `pandas.DataFrame` (requires pandas) |
| `plot(look_angle=None)` | Plot trajectory (requires matplotlib) |

---

### TrajectoryData

Named tuple for one trajectory point.

| Field | Type | Description |
|-------|------|-------------|
| `time` | float | Flight time (seconds) |
| `distance` | Distance | Down-range (x-axis) |
| `velocity` | Velocity | Scalar velocity (magnitude) |
| `mach` | float | Velocity in Mach |
| `height` | Distance | Vertical position (y-axis) relative to sight line |
| `slant_height` | Distance | Distance orthogonal to sight line |
| `drop_angle` | Angular | Slant height as angular correction |
| `windage` | Distance | Lateral deflection (z-axis) |
| `windage_angle` | Angular | Windage as angular correction |
| `slant_distance` | Distance | Distance along sight line closest to this point |
| `angle` | Angular | Angle of velocity vector relative to x-axis |
| `density_ratio` | float | Local air density / standard density |
| `drag` | float | Standard drag factor at this point |
| `energy` | Energy | Kinetic energy |
| `ogw` | Weight | Optimal game weight |
| `flag` | TrajFlag | Row type flag |

**Aliases:**
- `x` → `distance`, `y` → `height`, `z` → `windage`
- `target_drop` → `slant_height`
- `drop_adj` (deprecated) → `drop_angle`
- `windage_adj` (deprecated) → `windage_angle`
- `look_distance()` → `slant_distance`

**Extracting values:**
```python
p = results[5]
drop_cm  = p.height >> Distance.Centimeter
drop_mrad = p.drop_angle >> Angular.MRad
wind_cm  = p.windage >> Distance.Centimeter
wind_mrad = p.windage_angle >> Angular.MRad
vel_mps  = p.velocity >> Velocity.MPS
energy_j = p.energy >> Energy.Joule
```

**Static methods:**
- `calculate_energy(bullet_weight_gr, velocity_fps)` → ft·lbf
- `calculate_ogw(bullet_weight_gr, velocity_fps)` → pounds
- `get_correction(distance_ft, offset_ft)` → radians
- `interpolate(key_attribute, value, p0, p1, p2, flag, method)` → interpolated point

---

### DangerSpace

How much ranging error you can tolerate while still hitting a target of given height.

```python
ds = results.danger_space(
    at_range=Distance(500, Distance.Meter),
    target_height=Distance(50, Distance.Centimeter)
)
# ds.begin   → TrajectoryData at start of danger space
# ds.end     → TrajectoryData at end of danger space
# ds.at_range → TrajectoryData at target distance
# ds.target_height → Distance
# ds.look_angle → Angular
```

---

### TrajFlag

Bitwise flags for marking special trajectory points.

| Flag | Value | Description |
|------|-------|-------------|
| `NONE` | 0 | Standard point |
| `ZERO_UP` | 1 | Upward zero crossing |
| `ZERO_DOWN` | 2 | Downward zero crossing |
| `ZERO` | 3 | Any zero crossing (UP \| DOWN) |
| `MACH` | 4 | Mach 1 transition |
| `RANGE` | 8 | User-requested step point |
| `APEX` | 16 | Maximum height |
| `ALL` | 31 | All of the above |
| `MRT` | 32 | Mid-range trajectory / max ordinate |

**Usage:**
```python
results = calc.fire(shot, Distance(1000, Distance.Meter),
                    flags=TrajFlag.ZERO | TrajFlag.APEX | TrajFlag.MACH)
apex = results.flag(TrajFlag.APEX)
```

---

## Unit Reference

### Distance

Raw unit: **inches**

| Constant | Name |
|----------|------|
| `Distance.Inch` | inch |
| `Distance.Foot` / `Distance.Feet` | foot |
| `Distance.Yard` | yard |
| `Distance.Mile` | mile |
| `Distance.NauticalMile` | nautical mile |
| `Distance.Millimeter` | millimeter |
| `Distance.Centimeter` | centimeter |
| `Distance.Meter` | meter |
| `Distance.Kilometer` | kilometer |
| `Distance.Line` | line |

### Velocity

Raw unit: **meters per second**

| Constant | Name |
|----------|------|
| `Velocity.MPS` | m/s |
| `Velocity.FPS` | ft/s |
| `Velocity.KMH` | km/h |
| `Velocity.MPH` | mph |
| `Velocity.KT` | knot |

### Angular

Raw unit: **radians**  
Angles are normalized to (-π, π].

| Constant | Name |
|----------|------|
| `Angular.Radian` | radian |
| `Angular.Degree` | degree |
| `Angular.MOA` | minute of angle |
| `Angular.MRad` | milliradian |
| `Angular.Mil` | NATO mil |
| `Angular.Thousandth` | thousandth |
| `Angular.InchesPer100Yd` | inch/100yd |
| `Angular.CmPer100m` | cm/100m |
| `Angular.OClock` | hour (clock position) |

### Weight

Raw unit: **grains**

| Constant | Name |
|----------|------|
| `Weight.Grain` | grain |
| `Weight.Gram` | gram |
| `Weight.Kilogram` | kilogram |
| `Weight.Ounce` | ounce |
| `Weight.Pound` | pound |
| `Weight.Newton` | newton |

### Temperature

Raw unit: **Fahrenheit**  
Only supports `+` and `-`. Clamped at absolute zero.

| Constant | Name |
|----------|------|
| `Temperature.Fahrenheit` | fahrenheit |
| `Temperature.Celsius` | celsius |
| `Temperature.Kelvin` | kelvin |
| `Temperature.Rankin` | rankin |

### Pressure

Raw unit: **mmHg**

| Constant | Name |
|----------|------|
| `Pressure.MmHg` | mmHg |
| `Pressure.InHg` | inHg |
| `Pressure.Bar` | bar |
| `Pressure.hPa` | hPa (mbar) |
| `Pressure.PSI` | psi |

### Energy

Raw unit: **foot-pounds**

| Constant | Name |
|----------|------|
| `Energy.FootPound` | foot-pound |
| `Energy.Joule` | joule |

---

## PreferredUnits

Global defaults for display/creation when units aren't specified explicitly.

```python
PreferredUnits.set(
    distance='meter',
    velocity='mps',
    temperature='celsius',
    angular='degree',
    adjustment='mil',
    pressure='hPa',
    drop='centimeter',
    weight='gram'
)
```

**All configurable fields:**
`angular`, `distance`, `velocity`, `pressure`, `temperature`, `diameter`, `length`, `weight`, `adjustment`, `drop`, `energy`, `ogw`, `sight_height`, `target_height`, `twist`, `time`

**Reset:** `PreferredUnits.restore_defaults()`

**Convenience loaders:**
```python
from py_ballisticcalc import loadMetricUnits, loadImperialUnits, loadMixedUnits
loadMetricUnits()     # Sets all to metric
loadImperialUnits()   # Sets all to imperial (default)
loadMixedUnits()      # Mix of both
```

---

## Integration Engines

The `Calculator` accepts different numerical integration engines.

| Engine | Description |
|--------|-------------|
| `RK4IntegrationEngine` | 4th-order Runge-Kutta **(default)** |
| `EulerIntegrationEngine` | Simple Euler method (faster, less accurate) |
| `VelocityVerletIntegrationEngine` | Velocity Verlet (symplectic) |
| `SciPyIntegrationEngine` | SciPy-based (requires `scipy`) |

**Usage:**
```python
calc = Calculator(engine=EulerIntegrationEngine)
```

**Engine methods** (also accessible via Calculator delegation):

| Method | Description |
|--------|-------------|
| `integrate(shot, max_range, ...)` | Low-level trajectory integration |
| `zero_angle(shot, distance)` | Find barrel elevation for zero |
| `find_zero_angle(shot, distance, lofted=False)` | Find zero angle (supports lofted trajectories) |
| `find_apex(shot)` | Find trajectory apex point |
| `find_max_range(shot, angle_bracket_deg)` | Find maximum range and optimal launch angle |
| `get_calc_step()` | Get integration step size |

---

## Utility Functions

| Function | Description |
|----------|-------------|
| `get_drag_tables_names()` | List all available drag table names |
| `loadMetricUnits()` | Set PreferredUnits to metric |
| `loadImperialUnits()` | Set PreferredUnits to imperial |
| `loadMixedUnits()` | Set PreferredUnits to mixed |
| `enable_file_logging(filename='debug.log')` | Enable DEBUG-level file logging |
| `disable_file_logging()` | Disable file logging |

---

## Exceptions

| Exception | Description |
|-----------|-------------|
| `ZeroFindingError` | Failed to find zero elevation |
| `RangeError` | Trajectory didn't reach requested range |
| `OutOfRangeError` | Value out of valid range |
| `InterceptionError` | Interception calculation failed |
| `SolverRuntimeError` | Engine runtime error |
| `UnitConversionError` | Incompatible unit conversion |
| `UnitTypeError` | Wrong unit type |
| `UnitAliasError` | Invalid unit alias string |
