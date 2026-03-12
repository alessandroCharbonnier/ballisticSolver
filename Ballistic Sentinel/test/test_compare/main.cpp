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

    // ── Scenario 1: 6.5 Creedmoor G7, std atmo, no wind ───────────────
    runScenario({
        "65cm_g7_std",
        0.315, DragTableId::G7,
        140.0, 0.264, 1.35,
        2710.0, 1.5, 8.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        2000.0, 100.0
    });

    // ── Scenario 2: .308 Winchester G7, std atmo, 10 mph crosswind ─────
    runScenario({
        "308_g7_wind",
        0.243, DragTableId::G7,
        175.0, 0.308, 1.24,
        2600.0, 1.5, 10.0, 100.0,
        59.0, 29.92, 0.0,
        14.667, 90.0,  // 10 mph ≈ 14.667 fps, from left
        1500.0, 100.0
    });

    // ── Scenario 3: .338 Lapua G7, hot weather ──────────────────────────
    runScenario({
        "338lm_g7_hot",
        0.417, DragTableId::G7,
        300.0, 0.338, 1.70,
        2750.0, 1.5, 9.375, 100.0,
        100.0, 29.50, 0.50,
        0.0, 0.0,
        2500.0, 100.0
    });

    // ── Scenario 4: .50 BMG G1, std atmo ───────────────────────────────
    runScenario({
        "50bmg_g1_std",
        1.050, DragTableId::G1,
        750.0, 0.510, 2.31,
        2820.0, 2.0, 15.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        2500.0, 100.0
    });

    // ── Scenario 5: 6mm Creedmoor G7, cold weather ─────────────────────
    runScenario({
        "6cm_g7_cold",
        0.275, DragTableId::G7,
        105.0, 0.243, 1.30,
        3050.0, 1.5, 7.5, 100.0,
        20.0, 30.10, 0.20,
        0.0, 0.0,
        1500.0, 100.0
    });

    // ── Scenario 6: .300 PRC G7 at altitude ─────────────────────────────
    runScenario({
        "300prc_g7_alt",
        0.391, DragTableId::G7,
        230.0, 0.308, 1.60,
        2800.0, 1.5, 8.0, 100.0,
        70.0, 24.90, 0.30,  // ~5000 ft elevation
        0.0, 0.0,
        2000.0, 100.0
    });

    // ── Scenario 7: .308 Winchester G1 (for G1 vs G7 comparison) ───────
    runScenario({
        "308_g1_std",
        0.462, DragTableId::G1,
        175.0, 0.308, 1.24,
        2600.0, 1.5, 10.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        1500.0, 100.0
    });

    // ── Scenario 8: .375 CheyTac G7, extreme long range ────────────────
    runScenario({
        "375ct_g7_elr",
        0.465, DragTableId::G7,
        350.0, 0.375, 2.00,
        2750.0, 1.5, 10.0, 100.0,
        59.0, 29.92, 0.0,
        0.0, 0.0,
        3000.0, 100.0
    });

    return 0;
}
