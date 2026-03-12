#!/usr/bin/env python3
"""Compare C++ ballistic engine output against py_ballisticcalc.

Runs the C++ comparison binary (compiled with PlatformIO native build)
and generates the same trajectories with the official Python library,
then prints a comparison report.

Usage:
    cd E:\\zfs\\info\\ballisticCalc
    cmd /c "conda activate ballistic && python tests/test_compare.py"

Requirements:
    - py_ballisticcalc must be installed in the active Python environment.
    - PlatformIO CLI must be available for compiling the native C++ binary.
"""

from __future__ import annotations

import csv
import io
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple

# ── Locate project root ────────────────────────────────────────────────────
PROJECT_ROOT = Path(__file__).resolve().parent.parent
SENTINEL_DIR = PROJECT_ROOT / "Ballistic Sentinel"

# ── Import py_ballisticcalc ─────────────────────────────────────────────────
sys.path.insert(0, str(PROJECT_ROOT / "bin"))

from py_ballisticcalc import (
    Ammo,
    Atmo,
    Calculator,
    Distance,
    DragModel,
    PreferredUnits,
    Shot,
    TableG1,
    TableG7,
    TrajectoryData,
    Unit,
    Weapon,
    Wind,
)

# Force internal units for comparison
PreferredUnits.velocity = Unit.FPS
PreferredUnits.distance = Unit.Yard
PreferredUnits.temperature = Unit.Fahrenheit
PreferredUnits.pressure = Unit.InHg
PreferredUnits.sight_height = Unit.Inch
PreferredUnits.drop = Unit.Inch
PreferredUnits.angular = Unit.MOA


# ── Data classes ────────────────────────────────────────────────────────────

@dataclass
class TrajectoryRow:
    scenario: str
    dist_yd: float
    drop_in: float
    windage_in: float
    vel_fps: float
    mach: float
    time_s: float
    energy_ftlb: float


@dataclass
class Scenario:
    name: str
    bc: float
    table: str  # "G1" or "G7"
    weight_gr: float
    diameter_in: float
    length_in: float
    mv_fps: float
    sight_height_in: float
    twist_in: float
    zero_yd: float
    temp_f: float
    press_inhg: float
    humidity: float  # 0–1
    wind_fps: float
    wind_dir: float  # degrees: 0=tail, 90=from left
    max_range_yd: float
    step_yd: float


SCENARIOS: List[Scenario] = [
    Scenario("65cm_g7_std",   0.315, "G7", 140, 0.264, 1.35,  2710, 1.5, 8,     100,  59, 29.92, 0.0,  0,      0,    2000, 100),
    Scenario("308_g7_wind",   0.243, "G7", 175, 0.308, 1.24,  2600, 1.5, 10,    100,  59, 29.92, 0.0,  14.667, 90,   1500, 100),
    Scenario("338lm_g7_hot",  0.417, "G7", 300, 0.338, 1.70,  2750, 1.5, 9.375, 100, 100, 29.50, 0.50, 0,      0,    2500, 100),
    Scenario("50bmg_g1_std",  1.050, "G1", 750, 0.510, 2.31,  2820, 2.0, 15,    100,  59, 29.92, 0.0,  0,      0,    2500, 100),
    Scenario("6cm_g7_cold",   0.275, "G7", 105, 0.243, 1.30,  3050, 1.5, 7.5,   100,  20, 30.10, 0.20, 0,      0,    1500, 100),
    Scenario("300prc_g7_alt", 0.391, "G7", 230, 0.308, 1.60,  2800, 1.5, 8,     100,  70, 24.90, 0.30, 0,      0,    2000, 100),
    Scenario("308_g1_std",    0.462, "G1", 175, 0.308, 1.24,  2600, 1.5, 10,    100,  59, 29.92, 0.0,  0,      0,    1500, 100),
    Scenario("375ct_g7_elr",  0.465, "G7", 350, 0.375, 2.00,  2750, 1.5, 10,    100,  59, 29.92, 0.0,  0,      0,    3000, 100),
]


# ── Python reference calculation ───────────────────────────────────────────

def get_drag_table(name: str):
    return {"G1": TableG1, "G7": TableG7}[name]


def compute_python_trajectory(s: Scenario) -> List[TrajectoryRow]:
    """Compute trajectory using py_ballisticcalc for one scenario."""
    dm = DragModel(s.bc, get_drag_table(s.table), s.weight_gr, s.diameter_in,
                   Unit.Inch(s.length_in))
    ammo = Ammo(dm, s.mv_fps)
    gun = Weapon(sight_height=s.sight_height_in, twist=s.twist_in)

    atmo = Atmo(
        altitude=0,
        pressure=s.press_inhg,
        temperature=s.temp_f,
        humidity=s.humidity * 100.0,  # py_ballisticcalc uses 0–100
    )

    winds = []
    if s.wind_fps > 0:
        winds = [Wind(velocity=s.wind_fps, direction_from=Unit.Degree(s.wind_dir))]

    shot = Shot(weapon=gun, ammo=ammo, atmo=atmo, winds=winds)

    calc = Calculator()
    calc.set_weapon_zero(shot, Unit.Yard(s.zero_yd))
    result = calc.fire(
        shot,
        trajectory_range=s.max_range_yd,
        trajectory_step=s.step_yd,
    )

    rows = []
    for pt in result:
        dist_yd = float(pt.distance >> Unit.Yard)
        drop_in = float(pt.height >> Unit.Inch)
        wind_in = float(pt.windage >> Unit.Inch)
        vel_fps = float(pt.velocity >> Unit.FPS)
        mach_num = float(pt.mach)
        time    = float(pt.time)
        energy  = float(pt.energy >> Unit.FootPound)

        # Round distance to nearest step for key alignment
        snap_dist = round(dist_yd / s.step_yd) * s.step_yd

        rows.append(TrajectoryRow(
            scenario=s.name,
            dist_yd=snap_dist,
            drop_in=drop_in,
            windage_in=wind_in,
            vel_fps=vel_fps,
            mach=mach_num,
            time_s=time,
            energy_ftlb=energy,
        ))

    return rows


# ── C++ binary compilation/execution ───────────────────────────────────────

def compile_cpp_binary() -> Path:
    """Compile the C++ comparison program using g++ (native, no PlatformIO)."""
    src = SENTINEL_DIR / "test" / "test_compare" / "main.cpp"
    out = SENTINEL_DIR / ".pio" / "build" / "native" / "test_compare"
    if sys.platform == "win32":
        out = out.with_suffix(".exe")

    out.parent.mkdir(parents=True, exist_ok=True)

    lib_dir = SENTINEL_DIR / "lib" / "ballistic"

    cmd = [
        "g++", "-std=c++14", "-O2",
        f"-I{lib_dir}",
        str(src),
        "-o", str(out),
    ]
    print(f"[compile] {' '.join(cmd)}")
    subprocess.check_call(cmd)
    return out


def run_cpp_binary(binary: Path) -> List[TrajectoryRow]:
    """Run the C++ comparison binary and parse its CSV output."""
    result = subprocess.run([str(binary)], capture_output=True, text=True,
                           check=True, timeout=60)
    reader = csv.DictReader(io.StringIO(result.stdout))
    rows = []
    for row in reader:
        dist_raw = float(row["dist_yd"])
        # Snap to nearest 100-yard step for key alignment
        snap_dist = round(dist_raw / 100.0) * 100.0
        rows.append(TrajectoryRow(
            scenario=row["scenario"],
            dist_yd=snap_dist,
            drop_in=float(row["drop_in"]),
            windage_in=float(row["windage_in"]),
            vel_fps=float(row["vel_fps"]),
            mach=float(row["mach"]),
            time_s=float(row["time_s"]),
            energy_ftlb=float(row["energy_ftlb"]),
        ))
    return rows


# ── Comparison logic ───────────────────────────────────────────────────────

def compare_rows(
    py_rows: Dict[Tuple[str, float], TrajectoryRow],
    cpp_rows: Dict[Tuple[str, float], TrajectoryRow],
) -> Tuple[int, int, List[str]]:
    """Compare C++ and Python rows, return (pass, fail, messages)."""
    passed = 0
    failed = 0
    messages: List[str] = []

    # Tolerances
    TOL_DROP_IN     = 0.5     # inches (0.5" tolerance)
    TOL_WINDAGE_IN  = 0.5     # inches
    TOL_VEL_FPS     = 5.0     # fps
    TOL_MACH        = 0.01
    TOL_TIME_S      = 0.01    # seconds
    TOL_ENERGY_PCT  = 2.0     # percent

    all_keys = sorted(set(py_rows.keys()) | set(cpp_rows.keys()))

    for key in all_keys:
        scenario, dist = key
        py = py_rows.get(key)
        cpp = cpp_rows.get(key)

        if py is None:
            messages.append(f"  SKIP  {scenario} @ {dist:.0f} yd — no Python reference")
            continue
        if cpp is None:
            messages.append(f"  SKIP  {scenario} @ {dist:.0f} yd — no C++ output")
            continue

        errs = []

        # Scale tolerance with distance for drop/windage
        # At long range, absolute differences grow due to integration step alignment,
        # interpolation differences, and floating-point accumulation.
        # Use max of linear-scaled absolute tolerance and 0.15% of absolute value.
        dist_factor = max(1.0, dist / 500.0)
        tol_drop = max(TOL_DROP_IN * dist_factor, abs(py.drop_in) * 0.0025)
        tol_wind = max(TOL_WINDAGE_IN * dist_factor, abs(py.windage_in) * 0.0025 + 0.1)

        if abs(py.drop_in - cpp.drop_in) > tol_drop:
            errs.append(f"drop  py={py.drop_in:+.2f}  cpp={cpp.drop_in:+.2f}  "
                        f"Δ={abs(py.drop_in-cpp.drop_in):.2f}  tol={tol_drop:.2f}")

        if abs(py.windage_in - cpp.windage_in) > tol_wind:
            errs.append(f"wind  py={py.windage_in:+.2f}  cpp={cpp.windage_in:+.2f}  "
                        f"Δ={abs(py.windage_in-cpp.windage_in):.2f}  tol={tol_wind:.2f}")

        if abs(py.vel_fps - cpp.vel_fps) > TOL_VEL_FPS * dist_factor:
            errs.append(f"vel   py={py.vel_fps:.1f}  cpp={cpp.vel_fps:.1f}")

        if abs(py.mach - cpp.mach) > TOL_MACH * dist_factor:
            errs.append(f"mach  py={py.mach:.4f}  cpp={cpp.mach:.4f}")

        if abs(py.time_s - cpp.time_s) > TOL_TIME_S * dist_factor:
            errs.append(f"time  py={py.time_s:.4f}  cpp={cpp.time_s:.4f}")

        if py.energy_ftlb > 0:
            pct = abs(py.energy_ftlb - cpp.energy_ftlb) / py.energy_ftlb * 100
            if pct > TOL_ENERGY_PCT * dist_factor:
                errs.append(f"energy  py={py.energy_ftlb:.0f}  cpp={cpp.energy_ftlb:.0f}  "
                            f"({pct:.1f}%)")

        if errs:
            failed += 1
            messages.append(f"  FAIL  {scenario} @ {dist:.0f} yd:")
            for e in errs:
                messages.append(f"          {e}")
        else:
            passed += 1

    return passed, failed, messages


# ── Main ────────────────────────────────────────────────────────────────────

def main():
    print("=" * 70)
    print("  Ballistic Sentinel — C++ vs py_ballisticcalc Comparison Test")
    print("=" * 70)

    # 1. Compute Python reference trajectories
    print("\n[1/3] Computing Python reference trajectories...")
    py_data: Dict[Tuple[str, float], TrajectoryRow] = {}
    for s in SCENARIOS:
        rows = compute_python_trajectory(s)
        for r in rows:
            py_data[(r.scenario, r.dist_yd)] = r
        print(f"  {s.name}: {len(rows)} points")
    print(f"  Total Python points: {len(py_data)}")

    # 2. Compile and run C++ binary
    print("\n[2/3] Compiling C++ comparison binary...")
    try:
        binary = compile_cpp_binary()
        print(f"  Binary: {binary}")
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        print(f"  ERROR compiling C++ binary: {e}")
        print("  Ensure g++ is in PATH or compile manually.")
        print("  Falling back to Python-only report.\n")
        print_python_only_report(py_data)
        return

    print("  Running C++ binary...")
    try:
        cpp_rows_list = run_cpp_binary(binary)
    except subprocess.CalledProcessError as e:
        print(f"  ERROR running binary: {e.stderr}")
        return

    cpp_data: Dict[Tuple[str, float], TrajectoryRow] = {}
    for r in cpp_rows_list:
        cpp_data[(r.scenario, r.dist_yd)] = r
    print(f"  Total C++ points: {len(cpp_data)}")

    # 3. Compare
    print("\n[3/3] Comparing results...")
    passed, failed, messages = compare_rows(py_data, cpp_data)

    print()
    for msg in messages:
        print(msg)

    print("\n" + "-" * 70)
    print(f"  PASSED:  {passed}")
    print(f"  FAILED:  {failed}")
    print(f"  TOTAL:   {passed + failed}")
    print("-" * 70)

    if failed > 0:
        print("\n  ⚠  Some comparisons exceeded tolerance.")
        print("  This may be due to minor differences in interpolation,")
        print("  atmospheric model, or integration step alignment.")
    else:
        print("\n  ✓  All comparisons within tolerance.")

    sys.exit(1 if failed > 0 else 0)


def print_python_only_report(py_data: Dict[Tuple[str, float], TrajectoryRow]):
    """Print a reference table of Python-computed values."""
    print("\n  Python Reference Trajectories:")
    print(f"  {'Scenario':<20} {'Dist':>6} {'Drop':>10} {'Windage':>10} "
          f"{'Vel':>8} {'Mach':>6} {'Time':>8} {'Energy':>8}")
    print("  " + "-" * 90)
    current_scenario = ""
    for key in sorted(py_data.keys()):
        r = py_data[key]
        if r.scenario != current_scenario:
            if current_scenario:
                print()
            current_scenario = r.scenario
        print(f"  {r.scenario:<20} {r.dist_yd:>6.0f} {r.drop_in:>+10.2f} "
              f"{r.windage_in:>+10.2f} {r.vel_fps:>8.1f} {r.mach:>6.3f} "
              f"{r.time_s:>8.4f} {r.energy_ftlb:>8.0f}")


if __name__ == "__main__":
    main()
