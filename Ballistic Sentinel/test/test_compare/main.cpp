/// @file test_compare_output.cpp
/// @brief Outputs trajectory data for comparison with py_ballisticcalc.
///
/// Prints CSV rows for each test scenario/distance pair so the
/// Python comparison script can validate the C++ engine against
/// the reference Python library.
///
/// Output format:
///   scenario,dist_yd,drop_in,windage_in,vel_fps,mach,time_s,energy_ftlb

#include <cstdio>
#include <vector>
#include <ballistic.h>

using namespace ballistic;

struct Scenario {
    const char* name;
    double bc;
    DragTableId table;
    double weight_gr;
    double diameter_in;
    double length_in;
    double mv_fps;
    double sight_height_in;
    double twist_in;
    double zero_yd;
    double temp_f;
    double press_inhg;
    double humidity;
    double wind_fps;
    double wind_dir;      // degrees  (0 = tail, 90 = from left)
    double max_range_yd;
    double step_yd;
};

static void runScenario(const Scenario& s) {
    Calculator calc;
    calc.configure(s.bc, s.table,
                   s.weight_gr, s.diameter_in, s.length_in,
                   s.mv_fps, s.sight_height_in, s.twist_in, s.zero_yd);
    calc.setAtmosphere(s.temp_f, s.press_inhg, s.humidity);
    calc.setWind(s.wind_fps, s.wind_dir);
    if (!calc.solve()) {
        fprintf(stderr, "WARN: zero failed for %s\n", s.name);
    }

    auto traj = calc.trajectory(s.max_range_yd, s.step_yd);
    for (const auto& pt : traj) {
        double dist_yd = pt.distance_ft / kFeetPerYard;
        double drop_in = pt.height_ft * kInchesPerFoot;
        double wind_in = pt.windage_ft * kInchesPerFoot;
        printf("%s,%.1f,%.4f,%.4f,%.2f,%.4f,%.6f,%.2f\n",
               s.name, dist_yd, drop_in, wind_in,
               pt.velocity_fps, pt.mach, pt.time_s, pt.energy_ftlb);
    }
}

int main() {
    printf("scenario,dist_yd,drop_in,windage_in,vel_fps,mach,time_s,energy_ftlb\n");

    // ════════════════════════════════════════════════════════════════════
    // ORIGINAL BASELINE SCENARIOS
    // ════════════════════════════════════════════════════════════════════

    // ── 6.5 Creedmoor G7, std atmo, no wind ────────────────────────────
    runScenario({
        "65cm_g7_std",
        0.315, DragTableId::G7,
        140.0, 0.264, 1.35,
        2710.0, 1.5, 8.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        2000.0, 100.0
    });

    // ── .308 Winchester G7, std atmo, 10 mph crosswind ──────────────────
    runScenario({
        "308_g7_wind",
        0.243, DragTableId::G7,
        175.0, 0.308, 1.24,
        2600.0, 1.5, 10.0, 100.0,
        59.0, 29.92, 0.0,
        14.667, 90.0,
        1500.0, 100.0
    });

    // ── .338 Lapua G7, hot weather ──────────────────────────────────────
    runScenario({
        "338lm_g7_hot",
        0.417, DragTableId::G7,
        300.0, 0.338, 1.70,
        2750.0, 1.5, 9.375, 100.0,
        100.0, 29.50, 0.50,
        0.0, 0.0,
        2500.0, 100.0
    });

    // ── .50 BMG G1, std atmo ────────────────────────────────────────────
    runScenario({
        "50bmg_g1_std",
        1.050, DragTableId::G1,
        750.0, 0.510, 2.31,
        2820.0, 2.0, 15.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        2500.0, 100.0
    });

    // ── 6mm Creedmoor G7, cold weather ──────────────────────────────────
    runScenario({
        "6cm_g7_cold",
        0.275, DragTableId::G7,
        105.0, 0.243, 1.30,
        3050.0, 1.5, 7.5, 100.0,
        20.0, 30.10, 0.20,
        0.0, 0.0,
        1500.0, 100.0
    });

    // ── .300 PRC G7 at altitude ─────────────────────────────────────────
    runScenario({
        "300prc_g7_alt",
        0.391, DragTableId::G7,
        230.0, 0.308, 1.60,
        2800.0, 1.5, 8.0, 100.0,
        70.0, 24.90, 0.30,
        0.0, 0.0,
        2000.0, 100.0
    });

    // ── .308 Winchester G1 ──────────────────────────────────────────────
    runScenario({
        "308_g1_std",
        0.462, DragTableId::G1,
        175.0, 0.308, 1.24,
        2600.0, 1.5, 10.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1500.0, 100.0
    });

    // ── .375 CheyTac G7, extreme long range ─────────────────────────────
    runScenario({
        "375ct_g7_elr",
        0.465, DragTableId::G7,
        350.0, 0.375, 2.00,
        2750.0, 1.5, 10.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        3000.0, 100.0
    });

    // ════════════════════════════════════════════════════════════════════
    // .22 LR — COMPETITION BULLET BRANDS (King of 22LR distances)
    // All: dia=0.223", twist=16", zero=50yd, step=25yd, max=300yd
    // ════════════════════════════════════════════════════════════════════

    // SK Rifle Match — popular mid-tier match, 40gr LRN
    runScenario({
        "22lr_sk_rifle_std",
        0.131, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1073.0, 1.5, 16.0, 50.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        300.0, 25.0
    });

    // SK Long Range Match — optimized for distance, 40gr
    runScenario({
        "22lr_sk_lr_std",
        0.140, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1082.0, 1.5, 16.0, 50.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        300.0, 25.0
    });

    // Eley Tenex — gold standard .22 LR match ammo, 40gr flat-nose
    runScenario({
        "22lr_tenex_std",
        0.145, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1085.0, 1.5, 16.0, 50.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        300.0, 25.0
    });

    // Eley ELR (Contact) — subsonic design for long-range .22 LR, 42gr
    runScenario({
        "22lr_eley_elr_std",
        0.162, DragTableId::RA4,
        42.0, 0.223, 0.47,
        1040.0, 1.5, 16.0, 50.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        300.0, 25.0
    });

    // Lapua Center-X — consistent precision, 40gr
    runScenario({
        "22lr_centerx_std",
        0.138, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1073.0, 1.5, 16.0, 50.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        300.0, 25.0
    });

    // Lapua Midas+ — top-tier Lapua match, 40gr
    runScenario({
        "22lr_midas_std",
        0.143, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1090.0, 1.5, 16.0, 50.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        300.0, 25.0
    });

    // Lapua Long Range — designed for 50-100m competition, 40gr
    runScenario({
        "22lr_lap_lr_std",
        0.141, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1073.0, 1.5, 16.0, 50.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        300.0, 25.0
    });

    // Lapua Super Long Range — designed for 100m+ competition, 40gr
    runScenario({
        "22lr_lap_slr_std",
        0.147, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1066.0, 1.5, 16.0, 50.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        300.0, 25.0
    });

    // Lapua X-Act — ultimate precision, 40gr
    runScenario({
        "22lr_xact_std",
        0.145, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1073.0, 1.5, 16.0, 50.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        300.0, 25.0
    });

    // RWS R50 — German precision match, 40gr
    runScenario({
        "22lr_r50_std",
        0.136, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1082.0, 1.5, 16.0, 50.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        300.0, 25.0
    });

    // CCI Standard Velocity — budget baseline, 40gr
    runScenario({
        "22lr_cci_sv_std",
        0.129, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1070.0, 1.5, 16.0, 50.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        300.0, 25.0
    });

    // Federal Gold Medal Target — competition, 40gr
    runScenario({
        "22lr_fed_gmt_std",
        0.135, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1080.0, 1.5, 16.0, 50.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        300.0, 25.0
    });

    // ── .22 LR Weather Variations (Eley Tenex as reference) ─────────────

    // Hot outdoor match
    runScenario({
        "22lr_tenex_hot",
        0.145, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1085.0, 1.5, 16.0, 50.0,
        100.0, 29.50, 0.50,
        0.0, 0.0,
        300.0, 25.0
    });

    // Cold winter
    runScenario({
        "22lr_tenex_cold",
        0.145, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1085.0, 1.5, 16.0, 50.0,
        10.0, 30.20, 0.20,
        0.0, 0.0,
        300.0, 25.0
    });

    // Moderate altitude (~5000 ft)
    runScenario({
        "22lr_tenex_alt",
        0.145, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1085.0, 1.5, 16.0, 50.0,
        70.0, 24.90, 0.30,
        0.0, 0.0,
        300.0, 25.0
    });

    // Tropical high humidity
    runScenario({
        "22lr_tenex_humid",
        0.145, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1085.0, 1.5, 16.0, 50.0,
        90.0, 29.85, 0.90,
        0.0, 0.0,
        300.0, 25.0
    });

    // Desert extreme heat
    runScenario({
        "22lr_tenex_desert",
        0.145, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1085.0, 1.5, 16.0, 50.0,
        110.0, 29.80, 0.10,
        0.0, 0.0,
        300.0, 25.0
    });

    // ── .22 LR Wind Variations (Eley Tenex) ─────────────────────────────

    // 5 mph full-value crosswind
    runScenario({
        "22lr_tenex_w5",
        0.145, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1085.0, 1.5, 16.0, 50.0,
        59.0, 29.92, 0.0,
        7.33, 90.0,
        300.0, 25.0
    });

    // 10 mph full-value crosswind
    runScenario({
        "22lr_tenex_w10",
        0.145, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1085.0, 1.5, 16.0, 50.0,
        59.0, 29.92, 0.0,
        14.667, 90.0,
        300.0, 25.0
    });

    // 15 mph full-value crosswind
    runScenario({
        "22lr_tenex_w15",
        0.145, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1085.0, 1.5, 16.0, 50.0,
        59.0, 29.92, 0.0,
        22.0, 90.0,
        300.0, 25.0
    });

    // 10 mph quartering wind (45°)
    runScenario({
        "22lr_tenex_qw10",
        0.145, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1085.0, 1.5, 16.0, 50.0,
        59.0, 29.92, 0.0,
        14.667, 45.0,
        300.0, 25.0
    });

    // ── .22 LR King of 22LR Combined (hot + crosswind) ──────────────────
    runScenario({
        "22lr_tenex_k22",
        0.145, DragTableId::RA4,
        40.0, 0.223, 0.43,
        1085.0, 1.5, 16.0, 50.0,
        95.0, 29.60, 0.40,
        14.667, 90.0,
        300.0, 25.0
    });

    // ════════════════════════════════════════════════════════════════════
    // 6.5 CREEDMOOR — COMPETITION BULLETS (PRS distances)
    // ════════════════════════════════════════════════════════════════════

    // Berger 140gr Hybrid Target
    runScenario({
        "65cm_b140_std",
        0.311, DragTableId::G7,
        140.0, 0.264, 1.35,
        2710.0, 1.5, 8.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1200.0, 50.0
    });

    // Sierra 140gr MatchKing HPBT
    runScenario({
        "65cm_smk140_std",
        0.305, DragTableId::G7,
        140.0, 0.264, 1.33,
        2700.0, 1.5, 8.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1200.0, 50.0
    });

    // Hornady 147gr ELD-M
    runScenario({
        "65cm_eldm147_std",
        0.351, DragTableId::G7,
        147.0, 0.264, 1.42,
        2695.0, 1.5, 8.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1200.0, 50.0
    });

    // Handload: Berger 153.5gr LRHT @ 2650 fps
    runScenario({
        "65cm_b153hl_std",
        0.350, DragTableId::G7,
        153.5, 0.264, 1.48,
        2650.0, 1.5, 8.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1200.0, 50.0
    });

    // Handload: Hornady 140 ELD-M hot load @ 2750 fps
    runScenario({
        "65cm_140hl_hot",
        0.315, DragTableId::G7,
        140.0, 0.264, 1.35,
        2750.0, 1.5, 8.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1200.0, 50.0
    });

    // Handload: Hornady 147 ELD-M pushed @ 2720 fps
    runScenario({
        "65cm_147hl_std",
        0.351, DragTableId::G7,
        147.0, 0.264, 1.42,
        2720.0, 1.5, 8.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1200.0, 50.0
    });

    // ── 6.5 CM Weather + Wind (147 ELD-M PRS reference) ────────────────

    // 10 mph crosswind
    runScenario({
        "65cm_147_w10",
        0.351, DragTableId::G7,
        147.0, 0.264, 1.42,
        2695.0, 1.5, 8.0, 100.0,
        59.0, 29.92, 0.0,
        14.667, 90.0,
        1200.0, 50.0
    });

    // Hot summer match day
    runScenario({
        "65cm_147_hot",
        0.351, DragTableId::G7,
        147.0, 0.264, 1.42,
        2695.0, 1.5, 8.0, 100.0,
        100.0, 29.50, 0.50,
        0.0, 0.0,
        1200.0, 50.0
    });

    // Cold winter morning
    runScenario({
        "65cm_147_cold",
        0.351, DragTableId::G7,
        147.0, 0.264, 1.42,
        2695.0, 1.5, 8.0, 100.0,
        10.0, 30.20, 0.20,
        0.0, 0.0,
        1200.0, 50.0
    });

    // Moderate altitude (~5000 ft)
    runScenario({
        "65cm_147_alt",
        0.351, DragTableId::G7,
        147.0, 0.264, 1.42,
        2695.0, 1.5, 8.0, 100.0,
        70.0, 24.90, 0.30,
        0.0, 0.0,
        1200.0, 50.0
    });

    // PRS combined: altitude + warm + crosswind
    runScenario({
        "65cm_147_prs",
        0.351, DragTableId::G7,
        147.0, 0.264, 1.42,
        2695.0, 1.5, 8.0, 100.0,
        85.0, 25.00, 0.35,
        14.667, 90.0,
        1200.0, 50.0
    });

    // ════════════════════════════════════════════════════════════════════
    // 6MM CREEDMOOR — COMPETITION BULLETS (PRS distances)
    // ════════════════════════════════════════════════════════════════════

    // Hornady 110gr A-Tip Match
    runScenario({
        "6cm_atip110_std",
        0.301, DragTableId::G7,
        110.0, 0.243, 1.38,
        3020.0, 1.5, 7.5, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1200.0, 50.0
    });

    // Berger 105gr Hybrid Target
    runScenario({
        "6cm_b105_std",
        0.275, DragTableId::G7,
        105.0, 0.243, 1.30,
        3050.0, 1.5, 7.5, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1200.0, 50.0
    });

    // Handload: Berger 115gr DTAC @ 2950 fps
    runScenario({
        "6cm_b115hl_std",
        0.290, DragTableId::G7,
        115.0, 0.243, 1.36,
        2950.0, 1.5, 7.5, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1200.0, 50.0
    });

    // 110 A-Tip with 10 mph crosswind
    runScenario({
        "6cm_atip110_w10",
        0.301, DragTableId::G7,
        110.0, 0.243, 1.38,
        3020.0, 1.5, 7.5, 100.0,
        59.0, 29.92, 0.0,
        14.667, 90.0,
        1200.0, 50.0
    });

    // ════════════════════════════════════════════════════════════════════
    // .308 WINCHESTER — COMPETITION BULLETS (PRS / tactical)
    // ════════════════════════════════════════════════════════════════════

    // Hornady 168gr ELD-M
    runScenario({
        "308_eldm168_std",
        0.225, DragTableId::G7,
        168.0, 0.308, 1.20,
        2650.0, 1.5, 10.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1200.0, 50.0
    });

    // Hornady 178gr ELD-M
    runScenario({
        "308_eldm178_std",
        0.263, DragTableId::G7,
        178.0, 0.308, 1.28,
        2600.0, 1.5, 10.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1200.0, 50.0
    });

    // Berger 185gr Juggernaut Target
    runScenario({
        "308_b185_std",
        0.283, DragTableId::G7,
        185.0, 0.308, 1.32,
        2570.0, 1.5, 10.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1200.0, 50.0
    });

    // Handload: Berger 200gr Hybrid Target @ 2500 fps
    runScenario({
        "308_b200hl_std",
        0.310, DragTableId::G7,
        200.0, 0.308, 1.40,
        2500.0, 1.5, 10.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1200.0, 50.0
    });

    // 175 SMK with 15 mph crosswind
    runScenario({
        "308_175_w15",
        0.243, DragTableId::G7,
        175.0, 0.308, 1.24,
        2600.0, 1.5, 10.0, 100.0,
        59.0, 29.92, 0.0,
        22.0, 90.0,
        1200.0, 50.0
    });

    // 175 SMK hot day
    runScenario({
        "308_175_hot",
        0.243, DragTableId::G7,
        175.0, 0.308, 1.24,
        2600.0, 1.5, 10.0, 100.0,
        100.0, 29.50, 0.50,
        0.0, 0.0,
        1200.0, 50.0
    });

    // ════════════════════════════════════════════════════════════════════
    // .338 LAPUA MAGNUM — KING OF 1 MILE (max 1760 yd)
    // ════════════════════════════════════════════════════════════════════

    // Hornady 285gr ELD-M
    runScenario({
        "338lm_eldm285_std",
        0.375, DragTableId::G7,
        285.0, 0.338, 1.60,
        2745.0, 1.5, 9.375, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1760.0, 100.0
    });

    // Berger 300gr Hybrid OTM Tactical
    runScenario({
        "338lm_b300_std",
        0.419, DragTableId::G7,
        300.0, 0.338, 1.70,
        2750.0, 1.5, 9.375, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1760.0, 100.0
    });

    // Cutting Edge 275gr MTAC
    runScenario({
        "338lm_ce275_std",
        0.360, DragTableId::G7,
        275.0, 0.338, 1.55,
        2800.0, 1.5, 9.375, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1760.0, 100.0
    });

    // Handload: Berger 300gr OTM @ 2800 fps
    runScenario({
        "338lm_b300hl_std",
        0.419, DragTableId::G7,
        300.0, 0.338, 1.70,
        2800.0, 1.5, 9.375, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1760.0, 100.0
    });

    // K1M combined: altitude + 10 mph crosswind
    runScenario({
        "338lm_300_k1m",
        0.417, DragTableId::G7,
        300.0, 0.338, 1.70,
        2750.0, 1.5, 9.375, 100.0,
        80.0, 25.00, 0.30,
        14.667, 90.0,
        1760.0, 100.0
    });

    // ════════════════════════════════════════════════════════════════════
    // .300 PRC — ELR / KING OF 1 MILE
    // ════════════════════════════════════════════════════════════════════

    // Hornady 250gr A-Tip Match
    runScenario({
        "300prc_atip250_std",
        0.433, DragTableId::G7,
        250.0, 0.308, 1.75,
        2750.0, 1.5, 8.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1760.0, 100.0
    });

    // Handload: Berger 230gr Hybrid Target @ 2825 fps
    runScenario({
        "300prc_b230hl_std",
        0.368, DragTableId::G7,
        230.0, 0.308, 1.60,
        2825.0, 1.5, 8.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1760.0, 100.0
    });

    // K1M combined: altitude + 10 mph crosswind
    runScenario({
        "300prc_250_k1m",
        0.433, DragTableId::G7,
        250.0, 0.308, 1.75,
        2750.0, 1.5, 8.0, 100.0,
        80.0, 25.00, 0.30,
        14.667, 90.0,
        1760.0, 100.0
    });

    // ════════════════════════════════════════════════════════════════════
    // .375 CHEYTAC — KING OF 2 MILES (max 3520 yd)
    // ════════════════════════════════════════════════════════════════════

    // 350gr standard extended to 2-mile distance
    runScenario({
        "375ct_350_k2m",
        0.465, DragTableId::G7,
        350.0, 0.375, 2.00,
        2750.0, 1.5, 10.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        3520.0, 100.0
    });

    // Cutting Edge 377gr MTAC
    runScenario({
        "375ct_ce377_std",
        0.480, DragTableId::G7,
        377.0, 0.375, 2.10,
        2700.0, 1.5, 10.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        3520.0, 100.0
    });

    // Handload: 350gr pushed @ 2800 fps
    runScenario({
        "375ct_350hl_std",
        0.465, DragTableId::G7,
        350.0, 0.375, 2.00,
        2800.0, 1.5, 10.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        3520.0, 100.0
    });

    // K2M combined: altitude (~6000 ft) + 10 mph crosswind
    runScenario({
        "375ct_350_k2m_wx",
        0.465, DragTableId::G7,
        350.0, 0.375, 2.00,
        2750.0, 1.5, 10.0, 100.0,
        75.0, 24.00, 0.25,
        14.667, 90.0,
        3520.0, 100.0
    });

    // ════════════════════════════════════════════════════════════════════
    // .50 BMG — ELR (extended range + conditions)
    // ════════════════════════════════════════════════════════════════════

    // 750 A-Max with 10 mph crosswind
    runScenario({
        "50bmg_g1_w10",
        1.050, DragTableId::G1,
        750.0, 0.510, 2.31,
        2820.0, 2.0, 15.0, 100.0,
        59.0, 29.92, 0.0,
        14.667, 90.0,
        2500.0, 100.0
    });

    // 750 A-Max at altitude (~5000 ft)
    runScenario({
        "50bmg_g1_alt",
        1.050, DragTableId::G1,
        750.0, 0.510, 2.31,
        2820.0, 2.0, 15.0, 100.0,
        70.0, 24.90, 0.30,
        0.0, 0.0,
        2500.0, 100.0
    });

    // 750 A-Max extended to King of 2 Miles range
    runScenario({
        "50bmg_g1_k2m",
        1.050, DragTableId::G1,
        750.0, 0.510, 2.31,
        2820.0, 2.0, 15.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        3520.0, 100.0
    });

    // ════════════════════════════════════════════════════════════════════
    // EXTREME WEATHER MATRIX — cross-caliber
    // ════════════════════════════════════════════════════════════════════

    // 6.5 CM 140 ELD-M: desert extreme heat (110°F)
    runScenario({
        "65cm_140_desert",
        0.315, DragTableId::G7,
        140.0, 0.264, 1.35,
        2710.0, 1.5, 8.0, 100.0,
        110.0, 29.80, 0.10,
        0.0, 0.0,
        1200.0, 50.0
    });

    // 6.5 CM 140 ELD-M: mountain (high alt ~8500 ft, cold)
    runScenario({
        "65cm_140_mountain",
        0.315, DragTableId::G7,
        140.0, 0.264, 1.35,
        2710.0, 1.5, 8.0, 100.0,
        35.0, 22.00, 0.25,
        0.0, 0.0,
        1200.0, 50.0
    });

    // .308 175 SMK: tropical high humidity
    runScenario({
        "308_175_tropical",
        0.243, DragTableId::G7,
        175.0, 0.308, 1.24,
        2600.0, 1.5, 10.0, 100.0,
        90.0, 29.85, 0.90,
        0.0, 0.0,
        1200.0, 50.0
    });

    // .338 LM 300 SMK: very high altitude (~7500 ft)
    runScenario({
        "338lm_300_hialt",
        0.417, DragTableId::G7,
        300.0, 0.338, 1.70,
        2750.0, 1.5, 9.375, 100.0,
        60.0, 22.50, 0.15,
        0.0, 0.0,
        1760.0, 100.0
    });

    // ════════════════════════════════════════════════════════════════════
    // WIND ANGLE VARIATIONS — 6.5 CM 147 ELD-M
    // ════════════════════════════════════════════════════════════════════

    // 10 mph headwind
    runScenario({
        "65cm_147_headwind",
        0.351, DragTableId::G7,
        147.0, 0.264, 1.42,
        2695.0, 1.5, 8.0, 100.0,
        59.0, 29.92, 0.0,
        14.667, 180.0,
        1200.0, 50.0
    });

    // 10 mph tailwind
    runScenario({
        "65cm_147_tailwind",
        0.351, DragTableId::G7,
        147.0, 0.264, 1.42,
        2695.0, 1.5, 8.0, 100.0,
        59.0, 29.92, 0.0,
        14.667, 0.0,
        1200.0, 50.0
    });

    // 10 mph quartering from 45°
    runScenario({
        "65cm_147_qw45",
        0.351, DragTableId::G7,
        147.0, 0.264, 1.42,
        2695.0, 1.5, 8.0, 100.0,
        59.0, 29.92, 0.0,
        14.667, 45.0,
        1200.0, 50.0
    });

    // 10 mph quartering from 135°
    runScenario({
        "65cm_147_qw135",
        0.351, DragTableId::G7,
        147.0, 0.264, 1.42,
        2695.0, 1.5, 8.0, 100.0,
        59.0, 29.92, 0.0,
        14.667, 135.0,
        1200.0, 50.0
    });

    // 20 mph gust full crosswind
    runScenario({
        "65cm_147_gust20",
        0.351, DragTableId::G7,
        147.0, 0.264, 1.42,
        2695.0, 1.5, 8.0, 100.0,
        59.0, 29.92, 0.0,
        29.33, 90.0,
        1200.0, 50.0
    });

    return 0;
}
