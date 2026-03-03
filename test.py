from py_ballisticcalc import *
from py_ballisticcalc.unit import *

# ── Projectile / Drag ────────────────────────────────────────
BC          = 0.114            # Ballistic coefficient (RA4)
BULLET_WGT  = 40               # Bullet weight in grains
CALIBER     = 0.223            # Caliber in inches

# ── Muzzle ───────────────────────────────────────────────────
MUZZLE_VEL  = 314              # Muzzle velocity in m/s

# ── Weapon ───────────────────────────────────────────────────
SIGHT_HEIGHT = 6               # Sight height above bore in cm
TWIST        = 16.5            # Barrel twist in inches
ZERO_RANGE   = 48              # Zero distance in meters

# ── Atmosphere ───────────────────────────────────────────────
ALTITUDE    = 180              # Altitude in meters
TEMPERATURE = 9                # Temperature in °C
PRESSURE    = 1019             # Pressure in hPa (mbar)
HUMIDITY    = 0.74             # Relative humidity (0–1)

# ── Wind ─────────────────────────────────────────────────────
WIND_SPEED  = 0                # Wind speed in m/s
WIND_DIR    = 90               # Wind direction in degrees (90 = full cross)

# ── Target ───────────────────────────────────────────────────
TARGET_DIST = 100              # Target distance in meters
STEP        = 5                # Trajectory step in meters

# ══════════════════════════════════════════════════════════════

dm = DragModelMultiBC(
    bc_points=[
        BCPoint(BC=0.110, V=Velocity.MPS(312)),
        BCPoint(BC=0.114, V=Velocity.MPS(214)),
    ],
    drag_table=TableRA4,
    weight=Weight(40, Weight.Grain),
    diameter=Distance(0.223, Distance.Inch),
)
ammo = Ammo(dm, Velocity(MUZZLE_VEL, Velocity.MPS))
atmo = Atmo(
    altitude    = Distance(ALTITUDE, Distance.Meter),
    pressure    = Pressure(PRESSURE, Pressure.hPa),
    temperature = Temperature(TEMPERATURE, Temperature.Celsius),
    humidity    = HUMIDITY,
)
weapon = Weapon(
    sight_height = Distance(SIGHT_HEIGHT, Distance.Centimeter),
    twist        = Distance(TWIST, Distance.Inch),
)
wind = [Wind(Velocity(WIND_SPEED, Velocity.MPS),
             Angular(WIND_DIR, Angular.Degree))]

shot = Shot(weapon=weapon, ammo=ammo, atmo=atmo, winds=wind)
calc = Calculator()
calc.set_weapon_zero(shot, Distance(ZERO_RANGE, Distance.Meter))

results = calc.fire(shot,
                    trajectory_range = Distance(TARGET_DIST, Distance.Meter),
                    trajectory_step  = Distance(STEP, Distance.Meter))

# ── Print trajectory table ───────────────────────────────────
header = f"{'Dist(m)':>8} {'Drop(cm)':>9} {'Drop(mrad)':>11} {'Wind(cm)':>9} {'Wind(mrad)':>11} {'Vel(m/s)':>9} {'Time(s)':>7}"
print(header)
print("─" * len(header))
for p in results:
    dist_m   = p.distance >> Distance.Meter
    drop_cm  = p.height >> Distance.Centimeter
    drop_mrad = p.drop_angle >> Angular.MRad
    wind_cm   = p.windage >> Distance.Centimeter
    wind_mrad = p.windage_angle >> Angular.MRad
    vel       = p.velocity >> Velocity.MPS
    time_s    = round(p.time, 3)
    print(f"{dist_m:8.0f} {drop_cm:9.1f} {drop_mrad:11.2f} {wind_cm:9.1f} {wind_mrad:11.2f} {vel:9.1f} {time_s:7.3f}")
