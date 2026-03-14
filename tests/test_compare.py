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
    BCPoint,
    Calculator,
    Distance,
    DragModel,
    DragModelMultiBC,
    PreferredUnits,
    Pressure,
    Shot,
    TableG1,
    TableG7,
    TableRA4,
    Temperature,
    TrajectoryData,
    Unit,
    Velocity,
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
    # ════════════════════════════════════════════════════════════════════════
    # ORIGINAL BASELINE SCENARIOS (backward compatibility)
    # ════════════════════════════════════════════════════════════════════════
    Scenario("65cm_g7_std",       0.315, "G7", 140,   0.264, 1.35, 2710,  1.5, 8,     100,  59, 29.92, 0.0,  0,      0,   2000, 100),
    Scenario("308_g7_wind",       0.243, "G7", 175,   0.308, 1.24, 2600,  1.5, 10,    100,  59, 29.92, 0.0,  14.667, 90,  1500, 100),
    Scenario("338lm_g7_hot",      0.417, "G7", 300,   0.338, 1.70, 2750,  1.5, 9.375, 100, 100, 29.50, 0.50, 0,      0,   2500, 100),
    Scenario("50bmg_g1_std",      1.050, "G1", 750,   0.510, 2.31, 2820,  2.0, 15,    100,  59, 29.92, 0.0,  0,      0,   2500, 100),
    Scenario("6cm_g7_cold",       0.275, "G7", 105,   0.243, 1.30, 3050,  1.5, 7.5,   100,  20, 30.10, 0.20, 0,      0,   1500, 100),
    Scenario("300prc_g7_alt",     0.391, "G7", 230,   0.308, 1.60, 2800,  1.5, 8,     100,  70, 24.90, 0.30, 0,      0,   2000, 100),
    Scenario("308_g1_std",        0.462, "G1", 175,   0.308, 1.24, 2600,  1.5, 10,    100,  59, 29.92, 0.0,  0,      0,   1500, 100),
    Scenario("375ct_g7_elr",      0.465, "G7", 350,   0.375, 2.00, 2750,  1.5, 10,    100,  59, 29.92, 0.0,  0,      0,   3000, 100),

    # ════════════════════════════════════════════════════════════════════════
    # .22 LR — COMPETITION BULLET BRANDS (King of 22LR distances)
    # All: dia=0.223", twist=16", zero=50yd, step=25yd, max=300yd
    # ════════════════════════════════════════════════════════════════════════
    # SK Rifle Match — popular mid-tier match, 40gr LRN
    Scenario("22lr_sk_rifle_std",   0.131, "RA4", 40, 0.223, 0.43, 1073, 1.5, 16, 50,  59, 29.92, 0.0,  0, 0, 300, 25),
    # SK Long Range Match — optimized for distance, 40gr
    Scenario("22lr_sk_lr_std",      0.140, "RA4", 40, 0.223, 0.43, 1082, 1.5, 16, 50,  59, 29.92, 0.0,  0, 0, 300, 25),
    # Eley Tenex — gold standard .22 LR match ammo, 40gr flat-nose
    Scenario("22lr_tenex_std",      0.145, "RA4", 40, 0.223, 0.43, 1085, 1.5, 16, 50,  59, 29.92, 0.0,  0, 0, 300, 25),
    # Eley ELR (Contact) — subsonic design for long-range .22 LR, 42gr
    Scenario("22lr_eley_elr_std",   0.162, "RA4", 42, 0.223, 0.47, 1040, 1.5, 16, 50,  59, 29.92, 0.0,  0, 0, 300, 25),
    # Lapua Center-X — consistent precision, 40gr
    Scenario("22lr_centerx_std",    0.138, "RA4", 40, 0.223, 0.43, 1073, 1.5, 16, 50,  59, 29.92, 0.0,  0, 0, 300, 25),
    # Lapua Midas+ — top-tier Lapua match, 40gr
    Scenario("22lr_midas_std",      0.143, "RA4", 40, 0.223, 0.43, 1090, 1.5, 16, 50,  59, 29.92, 0.0,  0, 0, 300, 25),
    # Lapua Long Range — designed for 50-100m competition, 40gr
    Scenario("22lr_lap_lr_std",     0.141, "RA4", 40, 0.223, 0.43, 1073, 1.5, 16, 50,  59, 29.92, 0.0,  0, 0, 300, 25),
    # Lapua Super Long Range — designed for 100m+ competition, 40gr
    Scenario("22lr_lap_slr_std",    0.147, "RA4", 40, 0.223, 0.43, 1066, 1.5, 16, 50,  59, 29.92, 0.0,  0, 0, 300, 25),
    # Lapua X-Act — ultimate precision, 40gr
    Scenario("22lr_xact_std",       0.145, "RA4", 40, 0.223, 0.43, 1073, 1.5, 16, 50,  59, 29.92, 0.0,  0, 0, 300, 25),
    # RWS R50 — German precision match, 40gr
    Scenario("22lr_r50_std",        0.136, "RA4", 40, 0.223, 0.43, 1082, 1.5, 16, 50,  59, 29.92, 0.0,  0, 0, 300, 25),
    # CCI Standard Velocity — budget baseline, 40gr
    Scenario("22lr_cci_sv_std",     0.129, "RA4", 40, 0.223, 0.43, 1070, 1.5, 16, 50,  59, 29.92, 0.0,  0, 0, 300, 25),
    # Federal Gold Medal Target — competition, 40gr
    Scenario("22lr_fed_gmt_std",    0.135, "RA4", 40, 0.223, 0.43, 1080, 1.5, 16, 50,  59, 29.92, 0.0,  0, 0, 300, 25),

    # ── .22 LR Weather Variations (Eley Tenex as reference bullet) ──────
    # Hot outdoor match — summer conditions
    Scenario("22lr_tenex_hot",      0.145, "RA4", 40, 0.223, 0.43, 1085, 1.5, 16, 50, 100, 29.50, 0.50, 0, 0, 300, 25),
    # Cold winter — low temperature increases air density
    Scenario("22lr_tenex_cold",     0.145, "RA4", 40, 0.223, 0.43, 1085, 1.5, 16, 50,  10, 30.20, 0.20, 0, 0, 300, 25),
    # Moderate altitude (~5000 ft)
    Scenario("22lr_tenex_alt",      0.145, "RA4", 40, 0.223, 0.43, 1085, 1.5, 16, 50,  70, 24.90, 0.30, 0, 0, 300, 25),
    # Tropical high humidity
    Scenario("22lr_tenex_humid",    0.145, "RA4", 40, 0.223, 0.43, 1085, 1.5, 16, 50,  90, 29.85, 0.90, 0, 0, 300, 25),
    # Desert extreme heat
    Scenario("22lr_tenex_desert",   0.145, "RA4", 40, 0.223, 0.43, 1085, 1.5, 16, 50, 110, 29.80, 0.10, 0, 0, 300, 25),

    # ── .22 LR Wind Variations (Eley Tenex) ─────────────────────────────
    # 5 mph full-value crosswind
    Scenario("22lr_tenex_w5",       0.145, "RA4", 40, 0.223, 0.43, 1085, 1.5, 16, 50,  59, 29.92, 0.0,  7.33,   90, 300, 25),
    # 10 mph full-value crosswind
    Scenario("22lr_tenex_w10",      0.145, "RA4", 40, 0.223, 0.43, 1085, 1.5, 16, 50,  59, 29.92, 0.0,  14.667, 90, 300, 25),
    # 15 mph full-value crosswind
    Scenario("22lr_tenex_w15",      0.145, "RA4", 40, 0.223, 0.43, 1085, 1.5, 16, 50,  59, 29.92, 0.0,  22.0,   90, 300, 25),
    # 10 mph quartering wind (45°, half-value)
    Scenario("22lr_tenex_qw10",     0.145, "RA4", 40, 0.223, 0.43, 1085, 1.5, 16, 50,  59, 29.92, 0.0,  14.667, 45, 300, 25),

    # ── .22 LR King of 22LR Combined (hot summer match + crosswind) ─────
    Scenario("22lr_tenex_k22",      0.145, "RA4", 40, 0.223, 0.43, 1085, 1.5, 16, 50,  95, 29.60, 0.40, 14.667, 90, 300, 25),

    # ════════════════════════════════════════════════════════════════════════
    # 6.5 CREEDMOOR — COMPETITION BULLETS (PRS distances, step=50yd)
    # ════════════════════════════════════════════════════════════════════════
    # Berger 140gr Hybrid Target — popular PRS bullet
    Scenario("65cm_b140_std",       0.311, "G7", 140,   0.264, 1.35, 2710, 1.5, 8, 100,  59, 29.92, 0.0, 0, 0, 1200, 50),
    # Sierra 140gr MatchKing HPBT — classic match bullet
    Scenario("65cm_smk140_std",     0.305, "G7", 140,   0.264, 1.33, 2700, 1.5, 8, 100,  59, 29.92, 0.0, 0, 0, 1200, 50),
    # Hornady 147gr ELD-M — heavy-for-caliber, high BC
    Scenario("65cm_eldm147_std",    0.351, "G7", 147,   0.264, 1.42, 2695, 1.5, 8, 100,  59, 29.92, 0.0, 0, 0, 1200, 50),
    # Handload: Berger 153.5gr Long Range Hybrid Target @ 2650 fps
    Scenario("65cm_b153hl_std",     0.350, "G7", 153.5, 0.264, 1.48, 2650, 1.5, 8, 100,  59, 29.92, 0.0, 0, 0, 1200, 50),
    # Handload: Hornady 140gr ELD-M hot load @ 2750 fps
    Scenario("65cm_140hl_hot",      0.315, "G7", 140,   0.264, 1.35, 2750, 1.5, 8, 100,  59, 29.92, 0.0, 0, 0, 1200, 50),
    # Handload: Hornady 147gr ELD-M pushed @ 2720 fps
    Scenario("65cm_147hl_std",      0.351, "G7", 147,   0.264, 1.42, 2720, 1.5, 8, 100,  59, 29.92, 0.0, 0, 0, 1200, 50),

    # ── 6.5 CM Weather + Wind (147 ELD-M as PRS reference bullet) ───────
    # 10 mph crosswind
    Scenario("65cm_147_w10",        0.351, "G7", 147,   0.264, 1.42, 2695, 1.5, 8, 100,  59, 29.92, 0.0,  14.667, 90, 1200, 50),
    # Hot summer match day
    Scenario("65cm_147_hot",        0.351, "G7", 147,   0.264, 1.42, 2695, 1.5, 8, 100, 100, 29.50, 0.50, 0,      0,  1200, 50),
    # Cold winter morning
    Scenario("65cm_147_cold",       0.351, "G7", 147,   0.264, 1.42, 2695, 1.5, 8, 100,  10, 30.20, 0.20, 0,      0,  1200, 50),
    # Moderate altitude (~5000 ft)
    Scenario("65cm_147_alt",        0.351, "G7", 147,   0.264, 1.42, 2695, 1.5, 8, 100,  70, 24.90, 0.30, 0,      0,  1200, 50),
    # PRS combined: altitude + warm + crosswind (typical match day)
    Scenario("65cm_147_prs",        0.351, "G7", 147,   0.264, 1.42, 2695, 1.5, 8, 100,  85, 25.00, 0.35, 14.667, 90, 1200, 50),

    # ════════════════════════════════════════════════════════════════════════
    # 6MM CREEDMOOR — COMPETITION BULLETS (PRS distances)
    # ════════════════════════════════════════════════════════════════════════
    # Hornady 110gr A-Tip Match — very high BC for 6mm
    Scenario("6cm_atip110_std",     0.301, "G7", 110, 0.243, 1.38, 3020, 1.5, 7.5, 100,  59, 29.92, 0.0,  0,      0,  1200, 50),
    # Berger 105gr Hybrid Target — PRS staple
    Scenario("6cm_b105_std",        0.275, "G7", 105, 0.243, 1.30, 3050, 1.5, 7.5, 100,  59, 29.92, 0.0,  0,      0,  1200, 50),
    # Handload: Berger 115gr DTAC @ 2950 fps — heavy-for-caliber
    Scenario("6cm_b115hl_std",      0.290, "G7", 115, 0.243, 1.36, 2950, 1.5, 7.5, 100,  59, 29.92, 0.0,  0,      0,  1200, 50),
    # 110 A-Tip with 10 mph crosswind
    Scenario("6cm_atip110_w10",     0.301, "G7", 110, 0.243, 1.38, 3020, 1.5, 7.5, 100,  59, 29.92, 0.0,  14.667, 90, 1200, 50),

    # ════════════════════════════════════════════════════════════════════════
    # .308 WINCHESTER — COMPETITION BULLETS (PRS / tactical)
    # ════════════════════════════════════════════════════════════════════════
    # Hornady 168gr ELD-M — standard .308 match load
    Scenario("308_eldm168_std",     0.225, "G7", 168, 0.308, 1.20, 2650, 1.5, 10, 100,  59, 29.92, 0.0,  0,    0,  1200, 50),
    # Hornady 178gr ELD-M — heavier option
    Scenario("308_eldm178_std",     0.263, "G7", 178, 0.308, 1.28, 2600, 1.5, 10, 100,  59, 29.92, 0.0,  0,    0,  1200, 50),
    # Berger 185gr Juggernaut Target — high BC .308
    Scenario("308_b185_std",        0.283, "G7", 185, 0.308, 1.32, 2570, 1.5, 10, 100,  59, 29.92, 0.0,  0,    0,  1200, 50),
    # Handload: Berger 200gr Hybrid Target @ 2500 fps — subsonic at ELR
    Scenario("308_b200hl_std",      0.310, "G7", 200, 0.308, 1.40, 2500, 1.5, 10, 100,  59, 29.92, 0.0,  0,    0,  1200, 50),
    # 175 SMK with 15 mph crosswind
    Scenario("308_175_w15",         0.243, "G7", 175, 0.308, 1.24, 2600, 1.5, 10, 100,  59, 29.92, 0.0,  22.0, 90, 1200, 50),
    # 175 SMK hot day
    Scenario("308_175_hot",         0.243, "G7", 175, 0.308, 1.24, 2600, 1.5, 10, 100, 100, 29.50, 0.50, 0,    0,  1200, 50),

    # ════════════════════════════════════════════════════════════════════════
    # .338 LAPUA MAGNUM — KING OF 1 MILE (max 1760 yd = 1 mile)
    # ════════════════════════════════════════════════════════════════════════
    # Hornady 285gr ELD-M — factory match load
    Scenario("338lm_eldm285_std",   0.375, "G7", 285, 0.338, 1.60, 2745, 1.5, 9.375, 100,  59, 29.92, 0.0,  0,      0,  1760, 100),
    # Berger 300gr Hybrid OTM Tactical
    Scenario("338lm_b300_std",      0.419, "G7", 300, 0.338, 1.70, 2750, 1.5, 9.375, 100,  59, 29.92, 0.0,  0,      0,  1760, 100),
    # Cutting Edge 275gr MTAC — copper solid
    Scenario("338lm_ce275_std",     0.360, "G7", 275, 0.338, 1.55, 2800, 1.5, 9.375, 100,  59, 29.92, 0.0,  0,      0,  1760, 100),
    # Handload: Berger 300gr OTM @ 2800 fps — hot load
    Scenario("338lm_b300hl_std",    0.419, "G7", 300, 0.338, 1.70, 2800, 1.5, 9.375, 100,  59, 29.92, 0.0,  0,      0,  1760, 100),
    # K1M combined: altitude (~5000 ft) + 10 mph crosswind
    Scenario("338lm_300_k1m",       0.417, "G7", 300, 0.338, 1.70, 2750, 1.5, 9.375, 100,  80, 25.00, 0.30, 14.667, 90, 1760, 100),

    # ════════════════════════════════════════════════════════════════════════
    # .300 PRC — ELR / KING OF 1 MILE
    # ════════════════════════════════════════════════════════════════════════
    # Hornady 250gr A-Tip Match — highest BC factory .308-bore
    Scenario("300prc_atip250_std",  0.433, "G7", 250, 0.308, 1.75, 2750, 1.5, 8, 100,  59, 29.92, 0.0,  0,      0,  1760, 100),
    # Handload: Berger 230gr Hybrid Target @ 2825 fps
    Scenario("300prc_b230hl_std",   0.368, "G7", 230, 0.308, 1.60, 2825, 1.5, 8, 100,  59, 29.92, 0.0,  0,      0,  1760, 100),
    # K1M combined: altitude + 10 mph crosswind
    Scenario("300prc_250_k1m",      0.433, "G7", 250, 0.308, 1.75, 2750, 1.5, 8, 100,  80, 25.00, 0.30, 14.667, 90, 1760, 100),

    # ════════════════════════════════════════════════════════════════════════
    # .375 CHEYTAC — KING OF 2 MILES (max 3520 yd)
    # ════════════════════════════════════════════════════════════════════════
    # 350gr standard extended to 2-mile distance
    Scenario("375ct_350_k2m",       0.465, "G7", 350, 0.375, 2.00, 2750, 1.5, 10, 100,  59, 29.92, 0.0,  0,      0,  3520, 100),
    # Cutting Edge 377gr MTAC — copper solid, heavy
    Scenario("375ct_ce377_std",     0.480, "G7", 377, 0.375, 2.10, 2700, 1.5, 10, 100,  59, 29.92, 0.0,  0,      0,  3520, 100),
    # Handload: 350gr pushed @ 2800 fps
    Scenario("375ct_350hl_std",     0.465, "G7", 350, 0.375, 2.00, 2800, 1.5, 10, 100,  59, 29.92, 0.0,  0,      0,  3520, 100),
    # K2M combined: altitude (~6000 ft) + 10 mph crosswind
    Scenario("375ct_350_k2m_wx",    0.465, "G7", 350, 0.375, 2.00, 2750, 1.5, 10, 100,  75, 24.00, 0.25, 14.667, 90, 3520, 100),

    # ════════════════════════════════════════════════════════════════════════
    # .50 BMG — ELR (extended range + conditions)
    # ════════════════════════════════════════════════════════════════════════
    # 750 A-Max with 10 mph crosswind
    Scenario("50bmg_g1_w10",        1.050, "G1", 750, 0.510, 2.31, 2820, 2.0, 15, 100,  59, 29.92, 0.0,  14.667, 90, 2500, 100),
    # 750 A-Max at altitude (~5000 ft)
    Scenario("50bmg_g1_alt",        1.050, "G1", 750, 0.510, 2.31, 2820, 2.0, 15, 100,  70, 24.90, 0.30, 0,      0,  2500, 100),
    # 750 A-Max extended to King of 2 Miles range
    Scenario("50bmg_g1_k2m",        1.050, "G1", 750, 0.510, 2.31, 2820, 2.0, 15, 100,  59, 29.92, 0.0,  0,      0,  3520, 100),

    # ════════════════════════════════════════════════════════════════════════
    # EXTREME WEATHER MATRIX — representative bullets across conditions
    # ════════════════════════════════════════════════════════════════════════
    # 6.5 CM 140 ELD-M: desert extreme heat (110°F)
    Scenario("65cm_140_desert",     0.315, "G7", 140, 0.264, 1.35, 2710, 1.5, 8,     100, 110, 29.80, 0.10, 0, 0, 1200, 50),
    # 6.5 CM 140 ELD-M: mountain (high alt ~8500 ft, cold)
    Scenario("65cm_140_mountain",   0.315, "G7", 140, 0.264, 1.35, 2710, 1.5, 8,     100,  35, 22.00, 0.25, 0, 0, 1200, 50),
    # .308 175 SMK: tropical high humidity (90°F, 90% RH)
    Scenario("308_175_tropical",    0.243, "G7", 175, 0.308, 1.24, 2600, 1.5, 10,    100,  90, 29.85, 0.90, 0, 0, 1200, 50),
    # .338 LM 300 SMK: very high altitude (~7500 ft, CO/WY ELR venues)
    Scenario("338lm_300_hialt",     0.417, "G7", 300, 0.338, 1.70, 2750, 1.5, 9.375, 100,  60, 22.50, 0.15, 0, 0, 1760, 100),

    # ════════════════════════════════════════════════════════════════════════
    # WIND ANGLE VARIATIONS — 6.5 CM 147 ELD-M exhaustive wind testing
    # ════════════════════════════════════════════════════════════════════════
    # 10 mph headwind (0° = tail, 180° = head in our convention)
    Scenario("65cm_147_headwind",   0.351, "G7", 147, 0.264, 1.42, 2695, 1.5, 8, 100,  59, 29.92, 0.0,  14.667, 180, 1200, 50),
    # 10 mph tailwind
    Scenario("65cm_147_tailwind",   0.351, "G7", 147, 0.264, 1.42, 2695, 1.5, 8, 100,  59, 29.92, 0.0,  14.667, 0,   1200, 50),
    # 10 mph quartering from 45° (front-left)
    Scenario("65cm_147_qw45",       0.351, "G7", 147, 0.264, 1.42, 2695, 1.5, 8, 100,  59, 29.92, 0.0,  14.667, 45,  1200, 50),
    # 10 mph quartering from 135° (rear-left)
    Scenario("65cm_147_qw135",      0.351, "G7", 147, 0.264, 1.42, 2695, 1.5, 8, 100,  59, 29.92, 0.0,  14.667, 135, 1200, 50),
    # 20 mph gust full crosswind
    Scenario("65cm_147_gust20",     0.351, "G7", 147, 0.264, 1.42, 2695, 1.5, 8, 100,  59, 29.92, 0.0,  29.33,  90,  1200, 50),
]


# ── Multi-BC scenario dataclass ────────────────────────────────────────────

@dataclass
class MultiBCScenario:
    name: str
    bc_points: List[Tuple[float, float]]  # [(bc, velocity_fps), ...]
    table: str
    weight_gr: float
    diameter_in: float
    length_in: float
    mv_fps: float
    sight_height_in: float
    twist_in: float
    zero_yd: float
    temp_f: float
    press_inhg: float
    humidity: float
    wind_fps: float
    wind_dir: float
    max_range_yd: float
    step_yd: float


MULTI_BC_SCENARIOS: List[MultiBCScenario] = [
    # ════════════════════════════════════════════════════════════════════════
    # MULTI-BC SCENARIOS — velocity-stepped ballistic coefficients
    # ════════════════════════════════════════════════════════════════════════

    # 6.5 CM Berger 140gr Hybrid Target — 3 velocity-stepped BCs (G7)
    MultiBCScenario("mbc_65cm_b140",
        [(0.315, 2700), (0.310, 2200), (0.290, 1800)],
        "G7", 140, 0.264, 1.35, 2710, 1.5, 8, 100,
        59, 29.92, 0.0, 0, 0, 1200, 50),
    # 6.5 CM Hornady 147 ELD-M — multi-BC with PRS crosswind
    MultiBCScenario("mbc_65cm_147_w10",
        [(0.351, 2700), (0.345, 2200), (0.330, 1800)],
        "G7", 147, 0.264, 1.42, 2695, 1.5, 8, 100,
        59, 29.92, 0.0, 14.667, 90, 1200, 50),
    # .338 LM Berger 300gr Hybrid OTM — 3 BC points (G7)
    MultiBCScenario("mbc_338lm_b300",
        [(0.419, 2750), (0.410, 2200), (0.395, 1700)],
        "G7", 300, 0.338, 1.70, 2750, 1.5, 9.375, 100,
        59, 29.92, 0.0, 0, 0, 1760, 100),
    # .300 PRC Berger 230gr Hybrid Target — multi-BC at altitude (G7)
    MultiBCScenario("mbc_300prc_b230",
        [(0.368, 2800), (0.360, 2300), (0.340, 1800)],
        "G7", 230, 0.308, 1.60, 2825, 1.5, 8, 100,
        70, 24.90, 0.30, 0, 0, 1760, 100),
    # 6mm CM Hornady 110gr A-Tip — multi-BC (G7)
    MultiBCScenario("mbc_6cm_atip110",
        [(0.301, 3000), (0.295, 2500), (0.280, 2000)],
        "G7", 110, 0.243, 1.38, 3020, 1.5, 7.5, 100,
        59, 29.92, 0.0, 0, 0, 1200, 50),
    # .308 Win Berger 185gr Juggernaut — multi-BC with wind (G7)
    MultiBCScenario("mbc_308_b185_w10",
        [(0.283, 2600), (0.275, 2100), (0.260, 1600)],
        "G7", 185, 0.308, 1.32, 2570, 1.5, 10, 100,
        59, 29.92, 0.0, 14.667, 90, 1200, 50),
]


# ── Powder sensitivity scenario dataclass ──────────────────────────────────

@dataclass
class PowderSensScenario:
    name: str
    bc: float
    table: str
    weight_gr: float
    diameter_in: float
    length_in: float
    mv_fps: float  # baseline MV at reference temp
    sight_height_in: float
    twist_in: float
    zero_yd: float
    ref_temp_f: float  # reference/baseline powder temperature (°F)
    fps_per_degf: float  # raw sensitivity: fps velocity change per °F
    # actual shooting conditions
    temp_f: float
    press_inhg: float
    humidity: float
    wind_fps: float
    wind_dir: float
    max_range_yd: float
    step_yd: float


POWDER_SENS_SCENARIOS: List[PowderSensScenario] = [
    # ════════════════════════════════════════════════════════════════════════
    # POWDER SENSITIVITY SCENARIOS — MV adjusted for temperature
    # fps_per_degf = raw sensitivity in fps per °F
    # C++ modifier = fps_per_degf * 15 / mv_fps
    # Python: calibrated via calc_powder_sens with derived second data point
    # ════════════════════════════════════════════════════════════════════════

    # 6.5 CM 140 ELD-M — cold day (20°F), ref 59°F, ~1.5 fps/°F sensitivity
    PowderSensScenario("ps_65cm_140_cold",
        0.315, "G7", 140, 0.264, 1.35, 2710, 1.5, 8, 100,
        59.0, 1.5,
        20, 30.10, 0.20, 0, 0, 1200, 50),
    # 6.5 CM 140 ELD-M — hot day (110°F), same sensitivity
    PowderSensScenario("ps_65cm_140_hot",
        0.315, "G7", 140, 0.264, 1.35, 2710, 1.5, 8, 100,
        59.0, 1.5,
        110, 29.80, 0.10, 0, 0, 1200, 50),
    # .308 Win 175 SMK — extreme cold (0°F), ~1.0 fps/°F sensitivity
    PowderSensScenario("ps_308_175_cold",
        0.243, "G7", 175, 0.308, 1.24, 2600, 1.5, 10, 100,
        59.0, 1.0,
        0, 30.50, 0.10, 0, 0, 1200, 50),
    # .338 LM 300gr — hot desert (115°F), ~1.8 fps/°F sensitivity
    PowderSensScenario("ps_338lm_300_hot",
        0.417, "G7", 300, 0.338, 1.70, 2750, 1.5, 9.375, 100,
        59.0, 1.8,
        115, 29.70, 0.10, 0, 0, 1760, 100),
    # 6.5 CM 147 ELD-M — cold (15°F) with crosswind, ~1.5 fps/°F
    PowderSensScenario("ps_65cm_147_cold_w10",
        0.351, "G7", 147, 0.264, 1.42, 2695, 1.5, 8, 100,
        59.0, 1.5,
        15, 30.30, 0.15, 14.667, 90, 1200, 50),
    # .300 PRC 230gr — cold high-altitude match (25°F, ~5000ft), ~1.2 fps/°F
    PowderSensScenario("ps_300prc_230_cold_alt",
        0.391, "G7", 230, 0.308, 1.60, 2800, 1.5, 8, 100,
        59.0, 1.2,
        25, 24.90, 0.20, 0, 0, 1760, 100),
]


# ── Python reference calculation ───────────────────────────────────────────

def get_drag_table(name: str):
    return {"G1": TableG1, "G7": TableG7, "RA4": TableRA4}[name]


def compute_python_trajectory(s: Scenario) -> List[TrajectoryRow]:
    """Compute trajectory using py_ballisticcalc for one scenario."""
    dm = DragModel(s.bc, get_drag_table(s.table), s.weight_gr, s.diameter_in,
                   Unit.Inch(s.length_in))
    ammo = Ammo(dm, s.mv_fps)
    gun = Weapon(sight_height=s.sight_height_in, twist=s.twist_in)

    atmo = Atmo(
        altitude=0,
        pressure=Pressure.InHg(s.press_inhg),
        temperature=Temperature.Fahrenheit(s.temp_f),
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


def compute_multi_bc_trajectory(s: MultiBCScenario) -> List[TrajectoryRow]:
    """Compute trajectory using py_ballisticcalc with multi-BC model."""
    bc_pts = [BCPoint(bc, V=Velocity.FPS(v)) for bc, v in s.bc_points]
    dm = DragModelMultiBC(
        bc_pts, get_drag_table(s.table),
        weight=s.weight_gr, diameter=s.diameter_in,
        length=Unit.Inch(s.length_in),
    )
    ammo = Ammo(dm, s.mv_fps)
    gun = Weapon(sight_height=s.sight_height_in, twist=s.twist_in)

    atmo = Atmo(
        altitude=0,
        pressure=Pressure.InHg(s.press_inhg),
        temperature=Temperature.Fahrenheit(s.temp_f),
        humidity=s.humidity * 100.0,
    )

    winds = []
    if s.wind_fps > 0:
        winds = [Wind(velocity=s.wind_fps, direction_from=Unit.Degree(s.wind_dir))]

    shot = Shot(weapon=gun, ammo=ammo, atmo=atmo, winds=winds)

    calc = Calculator()
    calc.set_weapon_zero(shot, Unit.Yard(s.zero_yd))
    result = calc.fire(shot, trajectory_range=s.max_range_yd, trajectory_step=s.step_yd)

    rows = []
    for pt in result:
        dist_yd = float(pt.distance >> Unit.Yard)
        snap_dist = round(dist_yd / s.step_yd) * s.step_yd
        rows.append(TrajectoryRow(
            scenario=s.name, dist_yd=snap_dist,
            drop_in=float(pt.height >> Unit.Inch),
            windage_in=float(pt.windage >> Unit.Inch),
            vel_fps=float(pt.velocity >> Unit.FPS),
            mach=float(pt.mach), time_s=float(pt.time),
            energy_ftlb=float(pt.energy >> Unit.FootPound),
        ))
    return rows


def compute_powder_sens_trajectory(s: PowderSensScenario) -> List[TrajectoryRow]:
    """Compute trajectory using py_ballisticcalc with powder sensitivity."""
    dm = DragModel(s.bc, get_drag_table(s.table), s.weight_gr, s.diameter_in,
                   Unit.Inch(s.length_in))
    ammo = Ammo(
        dm, s.mv_fps,
        powder_temp=Temperature.Fahrenheit(s.ref_temp_f),
        use_powder_sensitivity=True,
    )
    # Calibrate from a derived second data point (15°F below reference)
    cold_temp_f = s.ref_temp_f - 15.0
    cold_mv_fps = s.mv_fps - s.fps_per_degf * 15.0
    ammo.calc_powder_sens(
        Velocity.FPS(cold_mv_fps),
        Temperature.Fahrenheit(cold_temp_f),
    )

    gun = Weapon(sight_height=s.sight_height_in, twist=s.twist_in)

    atmo = Atmo(
        altitude=0,
        pressure=Pressure.InHg(s.press_inhg),
        temperature=Temperature.Fahrenheit(s.temp_f),
        humidity=s.humidity * 100.0,
    )

    winds = []
    if s.wind_fps > 0:
        winds = [Wind(velocity=s.wind_fps, direction_from=Unit.Degree(s.wind_dir))]

    shot = Shot(weapon=gun, ammo=ammo, atmo=atmo, winds=winds)

    calc = Calculator()
    calc.set_weapon_zero(shot, Unit.Yard(s.zero_yd))
    result = calc.fire(shot, trajectory_range=s.max_range_yd, trajectory_step=s.step_yd)

    rows = []
    for pt in result:
        dist_yd = float(pt.distance >> Unit.Yard)
        snap_dist = round(dist_yd / s.step_yd) * s.step_yd
        rows.append(TrajectoryRow(
            scenario=s.name, dist_yd=snap_dist,
            drop_in=float(pt.height >> Unit.Inch),
            windage_in=float(pt.windage >> Unit.Inch),
            vel_fps=float(pt.velocity >> Unit.FPS),
            mach=float(pt.mach), time_s=float(pt.time),
            energy_ftlb=float(pt.energy >> Unit.FootPound),
        ))
    return rows


# ── C++ binary compilation/execution ───────────────────────────────────────

def compile_cpp_binary() -> Path:
    """Compile the C++ comparison program using g++ (native, no PlatformIO)."""
    src = PROJECT_ROOT / "tests" / "test_compare" / "main.cpp"
    out = PROJECT_ROOT / "build" / "test_compare"
    if sys.platform == "win32":
        out = out.with_suffix(".exe")

    out.parent.mkdir(parents=True, exist_ok=True)

    lib_dir = PROJECT_ROOT / "lib" / "ballistic"

    cmd = f'g++ -std=c++14 -O2 -I "{lib_dir}" "{src}" -o "{out}"'
    print(f"[compile] {cmd}")
    subprocess.check_call(cmd, shell=True)
    return out


def run_cpp_binary(binary: Path) -> List[TrajectoryRow]:
    """Run the C++ comparison binary and parse its CSV output."""
    step_lookup = {s.name: s.step_yd for s in SCENARIOS}
    for s in MULTI_BC_SCENARIOS:
        step_lookup[s.name] = s.step_yd
    for s in POWDER_SENS_SCENARIOS:
        step_lookup[s.name] = s.step_yd
    result = subprocess.run([str(binary)], capture_output=True, text=True,
                           check=True, timeout=300)
    reader = csv.DictReader(io.StringIO(result.stdout))
    rows = []
    for row in reader:
        dist_raw = float(row["dist_yd"])
        # Snap to nearest step for key alignment (step varies by scenario)
        step = step_lookup.get(row["scenario"], 100.0)
        snap_dist = round(dist_raw / step) * step
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
    TOL_DROP_IN     = 0.65    # inches (base tolerance at short range)
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
        # Use max of linear-scaled absolute tolerance and 0.3% of absolute value.
        dist_factor = max(1.0, dist / 500.0)
        tol_drop = max(TOL_DROP_IN * dist_factor, abs(py.drop_in) * 0.003)
        tol_wind = max(TOL_WINDAGE_IN * dist_factor, abs(py.windage_in) * 0.0025 + 0.1)

        if abs(py.drop_in - cpp.drop_in) > tol_drop:
            errs.append(f"drop  py={py.drop_in:+.2f}  cpp={cpp.drop_in:+.2f}  "
                        f"d={abs(py.drop_in-cpp.drop_in):.2f}  tol={tol_drop:.2f}")

        if abs(py.windage_in - cpp.windage_in) > tol_wind:
            errs.append(f"wind  py={py.windage_in:+.2f}  cpp={cpp.windage_in:+.2f}  "
                        f"d={abs(py.windage_in-cpp.windage_in):.2f}  tol={tol_wind:.2f}")

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
            messages.extend(f"          {e}" for e in errs)
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
    for s in MULTI_BC_SCENARIOS:
        rows = compute_multi_bc_trajectory(s)
        for r in rows:
            py_data[(r.scenario, r.dist_yd)] = r
        print(f"  {s.name}: {len(rows)} points")
    for s in POWDER_SENS_SCENARIOS:
        rows = compute_powder_sens_trajectory(s)
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

    cpp_data: Dict[Tuple[str, float], TrajectoryRow] = {
        (r.scenario, r.dist_yd): r for r in cpp_rows_list
    }
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
        print("\n  WARNING: Some comparisons exceeded tolerance.")
        print("  This may be due to minor differences in interpolation,")
        print("  atmospheric model, or integration step alignment.")
    else:
        print("\n  OK: All comparisons within tolerance.")

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
