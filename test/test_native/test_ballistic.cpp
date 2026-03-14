/// @file test_ballistic.cpp
/// @brief Native (PC) unit tests for the C++ ballistic engine.
///
/// Validates: constants, vector math, PCHIP interpolation, atmosphere model,
/// drag model, zero-finding, trajectory computation, and corrections for
/// multiple calibers across various conditions and extreme ranges.
///
/// Build:  pio test -e native

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include <unity.h>
#include <ballistic.h>

using namespace ballistic;

// ═══════════════════════════════════════════════════════════════════════════
//  Helpers
// ═══════════════════════════════════════════════════════════════════════════

static constexpr double ABS_TOL = 1e-6;

static void ASSERT_NEAR(double expected, double actual, double tol,
                         const char* msg = "") {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "%s  expected=%.10f  actual=%.10f  diff=%.10e",
             msg, expected, actual, actual - expected);
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(tol, expected, actual, buf);
}

// ═══════════════════════════════════════════════════════════════════════════
//  1.  Constants
// ═══════════════════════════════════════════════════════════════════════════

void test_constants_gravity() {
    TEST_ASSERT_DOUBLE_WITHIN(1e-5, -32.17405, kGravityFps2);
}

void test_constants_std_atmosphere() {
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 59.0, kStdTempF);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 29.92, kStdPressureInHg);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.076474, kStdDensityLbFt3);
}

void test_constants_conversions() {
    // MOA ↔ radians round-trip
    double moa = 1.0;
    double rad = moa * kMoaToRad;
    double back = rad * kRadToMoa;
    ASSERT_NEAR(1.0, back, 1e-10, "MOA round-trip");

    // MRAD ↔ radians round-trip
    double mrad = 1.0;
    rad = mrad * kMradToRad;
    back = rad * kRadToMrad;
    ASSERT_NEAR(1.0, back, 1e-10, "MRAD round-trip");
}

// ═══════════════════════════════════════════════════════════════════════════
//  2.  Vector3
// ═══════════════════════════════════════════════════════════════════════════

void test_vector3_addition() {
    Vector3 a{1.0, 2.0, 3.0};
    Vector3 b{4.0, 5.0, 6.0};
    Vector3 c = a + b;
    ASSERT_NEAR(5.0, c.x, ABS_TOL, "x");
    ASSERT_NEAR(7.0, c.y, ABS_TOL, "y");
    ASSERT_NEAR(9.0, c.z, ABS_TOL, "z");
}

void test_vector3_magnitude() {
    Vector3 v{3.0, 4.0, 0.0};
    ASSERT_NEAR(5.0, v.magnitude(), ABS_TOL, "magnitude 3-4-5");
}

void test_vector3_normalize() {
    Vector3 v{3.0, 4.0, 0.0};
    Vector3 n = v.normalized();
    ASSERT_NEAR(1.0, n.magnitude(), 1e-12, "normalized magnitude");
    ASSERT_NEAR(0.6, n.x, ABS_TOL, "nx");
    ASSERT_NEAR(0.8, n.y, ABS_TOL, "ny");
}

void test_vector3_scalar_multiply() {
    Vector3 v{1.0, 2.0, 3.0};
    Vector3 r = v * 2.0;
    ASSERT_NEAR(2.0, r.x, ABS_TOL, "2x");
    ASSERT_NEAR(4.0, r.y, ABS_TOL, "2y");
    ASSERT_NEAR(6.0, r.z, ABS_TOL, "2z");
}

void test_vector3_dot() {
    Vector3 a{1, 0, 0};
    Vector3 b{0, 1, 0};
    ASSERT_NEAR(0.0, a.dot(b), ABS_TOL, "orthogonal dot");
    ASSERT_NEAR(1.0, a.dot(a), ABS_TOL, "self dot");
}

// ═══════════════════════════════════════════════════════════════════════════
//  3.  PCHIP Interpolation
// ═══════════════════════════════════════════════════════════════════════════

void test_pchip_basic() {
    // Build spline from a small dataset
    DragPoint pts[] = {
        {0.0, 0.100},
        {0.5, 0.200},
        {1.0, 0.400},
        {1.5, 0.500},
        {2.0, 0.450}
    };
    PchipSpline spline;
    spline.build(pts, 5);

    // At knot points, must match exactly
    ASSERT_NEAR(0.100, spline.eval(0.0), 1e-10, "at x=0.0");
    ASSERT_NEAR(0.200, spline.eval(0.5), 1e-10, "at x=0.5");
    ASSERT_NEAR(0.400, spline.eval(1.0), 1e-10, "at x=1.0");

    // Between knots, should be smooth and bounded
    double mid = spline.eval(0.25);
    TEST_ASSERT_TRUE_MESSAGE(mid > 0.100 && mid < 0.200, "between 0 and 0.5");

    // Below / above range: clamp
    ASSERT_NEAR(0.100, spline.eval(-1.0), 1e-10, "below range");
    ASSERT_NEAR(0.450, spline.eval(5.0), 1e-10, "above range");
}

void test_pchip_g7_table() {
    // Build spline from full G7 table and check a known point
    size_t sz = 0;
    const DragPoint* g7 = getDragTable(DragTableId::G7, sz);
    TEST_ASSERT_TRUE(sz > 10);

    PchipSpline spline;
    spline.build(g7, sz);

    // At knot points, must match
    for (size_t i = 0; i < sz; i += 10) {
        ASSERT_NEAR(g7[i].cd, spline.eval(g7[i].mach), 1e-8,
                    "G7 knot match");
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  4.  Atmosphere
// ═══════════════════════════════════════════════════════════════════════════

void test_atmosphere_standard() {
    Atmosphere atmo(kStdTempF, kStdPressureInHg, kStdHumidity);
    // density_ratio at standard should be ~1.0
    ASSERT_NEAR(1.0, atmo.density_ratio, 0.01, "std density ratio");
    // Speed of sound at 59°F ≈ 1116 fps
    TEST_ASSERT_TRUE_MESSAGE(atmo.mach_fps > 1100.0 && atmo.mach_fps < 1130.0,
                             "SoS at 59F");
}

void test_atmosphere_hot() {
    Atmosphere atmo(100.0, 29.92, 0.0);
    // Hot air is less dense → ratio < 1
    TEST_ASSERT_TRUE_MESSAGE(atmo.density_ratio < 1.0,
                             "hotter → less dense");
    // Speed of sound increases with temperature
    Atmosphere std(kStdTempF, kStdPressureInHg, kStdHumidity);
    TEST_ASSERT_TRUE_MESSAGE(atmo.mach_fps > std.mach_fps,
                             "SoS increases with temp");
}

void test_atmosphere_altitude_correction() {
    Atmosphere atmo(kStdTempF, kStdPressureInHg, kStdHumidity, 0.0);
    double dr_5k, mach_5k;
    atmo.atAltitude(5000.0, dr_5k, mach_5k);
    // At 5000 ft, air is thinner
    TEST_ASSERT_TRUE_MESSAGE(dr_5k < atmo.density_ratio,
                             "less dense at 5000 ft");
}

void test_atmosphere_humidity_effect() {
    Atmosphere dry(kStdTempF, kStdPressureInHg, 0.0);
    Atmosphere wet(kStdTempF, kStdPressureInHg, 1.0);
    // Wet air is slightly less dense (water vapour is lighter than N₂/O₂)
    TEST_ASSERT_TRUE_MESSAGE(wet.density_ratio < dry.density_ratio,
                             "humid air less dense");
}

void test_atmosphere_unit_conversions() {
    double c = Atmosphere::fToC(32.0);
    ASSERT_NEAR(0.0, c, 1e-6, "32F → 0C");

    double f = Atmosphere::cToF(100.0);
    ASSERT_NEAR(212.0, f, 1e-6, "100C → 212F");

    double k = Atmosphere::fToK(59.0);
    ASSERT_NEAR(288.15, k, 0.02, "59F → 288.15K");
}

// ═══════════════════════════════════════════════════════════════════════════
//  5.  Drag Model
// ═══════════════════════════════════════════════════════════════════════════

void test_drag_model_single_bc() {
    DragModel dm(0.315, DragTableId::G7, 140.0, 0.264, 1.35);

    // Sectional density = weight / (diameter^2 * 7000)
    double expected_sd = 140.0 / (0.264 * 0.264 * 7000.0);
    ASSERT_NEAR(expected_sd, dm.sect_density, 1e-6, "sectional density");

    // form_factor = sd / bc
    ASSERT_NEAR(expected_sd / 0.315, dm.form_factor, 1e-6, "form factor");

    // Cd at supersonic Mach
    double cd_1_5 = dm.cdAtMach(1.5);
    TEST_ASSERT_TRUE_MESSAGE(cd_1_5 > 0.0, "Cd at M1.5 must be positive");

    // Drag by mach should return sensible values
    double drag = dm.dragByMach(2.0);
    TEST_ASSERT_TRUE_MESSAGE(drag > 0.0, "drag at M2.0");
}

void test_drag_tables_all_exist() {
    DragTableId tables[] = {
        DragTableId::G1, DragTableId::G7, DragTableId::G2,
        DragTableId::G5, DragTableId::G6, DragTableId::G8,
        DragTableId::GI, DragTableId::GS, DragTableId::RA4
    };
    for (auto id : tables) {
        size_t sz = 0;
        const DragPoint* tbl = getDragTable(id, sz);
        char msg[64];
        snprintf(msg, sizeof(msg), "table %d should exist", (int)id);
        TEST_ASSERT_NOT_NULL_MESSAGE(tbl, msg);
        TEST_ASSERT_TRUE_MESSAGE(sz > 5, msg);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  6.  Zero-Finding
// ═══════════════════════════════════════════════════════════════════════════

void test_zero_finding_basic() {
    // 6.5 Creedmoor, 140gr ELD-M, G7 BC 0.315, MV 2710 fps, zero at 100 yd
    Calculator calc;
    calc.configure(0.315, DragTableId::G7,
                   140.0, 0.264, 1.35,
                   2710.0, 1.5, 8.0, 100.0);
    calc.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);

    bool ok = calc.solve();
    TEST_ASSERT_TRUE_MESSAGE(ok, "zero finding converged");

    // At zero range, correction should be near zero
    auto result = calc.correction(100.0, CorrectionUnit::MOA);
    TEST_ASSERT_TRUE_MESSAGE(result.valid, "correction at zero range valid");
    ASSERT_NEAR(0.0, result.vertical, 0.5, "correction at zero ~0 MOA");
}

void test_zero_finding_long_range() {
    // Zero at 200 yd
    Calculator calc;
    calc.configure(0.315, DragTableId::G7,
                   140.0, 0.264, 1.35,
                   2710.0, 1.5, 8.0, 200.0);
    calc.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);

    bool ok = calc.solve();
    TEST_ASSERT_TRUE_MESSAGE(ok, "200 yd zero converged");

    auto result = calc.correction(200.0, CorrectionUnit::MOA);
    ASSERT_NEAR(0.0, result.vertical, 0.5, "correction at 200 yd zero ~0");
}

// ═══════════════════════════════════════════════════════════════════════════
//  7.  Trajectory — 6.5 Creedmoor (G7)
// ═══════════════════════════════════════════════════════════════════════════

void test_trajectory_65cm_drop_increases() {
    Calculator calc;
    calc.configure(0.315, DragTableId::G7,
                   140.0, 0.264, 1.35,
                   2710.0, 1.5, 8.0, 100.0);
    calc.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);
    calc.solve();

    auto t = calc.trajectory(1000.0, 100.0);
    TEST_ASSERT_TRUE_MESSAGE(t.size() >= 9, "should have ≥9 points to 1000 yd");

    // Drop should monotonically increase past zero range
    for (size_t i = 2; i < t.size(); ++i) {
        TEST_ASSERT_TRUE_MESSAGE(t[i].height_ft <= t[i - 1].height_ft + 0.01,
                                 "drop should increase");
    }
}

void test_trajectory_65cm_velocity_decreases() {
    Calculator calc;
    calc.configure(0.315, DragTableId::G7,
                   140.0, 0.264, 1.35,
                   2710.0, 1.5, 8.0, 100.0);
    calc.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);
    calc.solve();

    auto t = calc.trajectory(2000.0, 100.0);
    for (size_t i = 1; i < t.size(); ++i) {
        TEST_ASSERT_TRUE_MESSAGE(t[i].velocity_fps < t[i - 1].velocity_fps + 0.1,
                                 "velocity should decrease");
    }
}

void test_trajectory_65cm_reasonable_values() {
    Calculator calc;
    calc.configure(0.315, DragTableId::G7,
                   140.0, 0.264, 1.35,
                   2710.0, 1.5, 8.0, 100.0);
    calc.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);
    calc.solve();

    // Correction at 500 yd: typical 6.5CM is ~5-12 MOA
    auto r500 = calc.correction(500.0, CorrectionUnit::MOA);
    TEST_ASSERT_TRUE_MESSAGE(r500.valid, "500 yd valid");
    char buf500[128];
    snprintf(buf500, sizeof(buf500), "500 yd drop=%.2f MOA", r500.vertical);
    TEST_ASSERT_TRUE_MESSAGE(r500.vertical > 2.0 && r500.vertical < 15.0, buf500);
    TEST_ASSERT_TRUE_MESSAGE(r500.vertical_up, "should dial up");

    // Correction at 1000 yd: expect ~15-50 MOA
    auto r1000 = calc.correction(1000.0, CorrectionUnit::MOA);
    TEST_ASSERT_TRUE_MESSAGE(r1000.valid, "1000 yd valid");
    char buf1000[128];
    snprintf(buf1000, sizeof(buf1000), "1000 yd drop=%.2f MOA", r1000.vertical);
    TEST_ASSERT_TRUE_MESSAGE(r1000.vertical > 15.0 && r1000.vertical < 50.0, buf1000);
}

// ═══════════════════════════════════════════════════════════════════════════
//  8.  Multiple Calibers / Drag Profiles
// ═══════════════════════════════════════════════════════════════════════════

void test_308_g7_trajectory() {
    // .308 Win 175gr SMK, G7 BC 0.243, MV 2600
    Calculator calc;
    calc.configure(0.243, DragTableId::G7,
                   175.0, 0.308, 1.24,
                   2600.0, 1.5, 10.0, 100.0);
    calc.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);
    calc.solve();

    auto r = calc.correction(800.0, CorrectionUnit::MOA);
    TEST_ASSERT_TRUE_MESSAGE(r.valid, ".308 at 800 yd valid");
    // .308 has more drop than 6.5CM → larger correction
    TEST_ASSERT_TRUE_MESSAGE(r.vertical > 10.0, ".308 more drop at 800 yd");
}

void test_338lm_g7_extreme_range() {
    // .338 Lapua 300gr Scenar, G7 BC 0.417, MV 2750
    Calculator calc;
    calc.configure(0.417, DragTableId::G7,
                   300.0, 0.338, 1.70,
                   2750.0, 1.5, 9.375, 100.0);
    calc.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);
    calc.solve();

    // 2000 yd — ELR
    auto r = calc.correction(2000.0, CorrectionUnit::MOA);
    TEST_ASSERT_TRUE_MESSAGE(r.valid, ".338LM at 2000 yd valid");
    TEST_ASSERT_TRUE_MESSAGE(r.vertical > 30.0, ".338LM high correction at 2000 yd");

    // Velocity should still be meaningful
    TEST_ASSERT_TRUE_MESSAGE(r.velocity_fps > 500.0,
                             ".338LM still moving at 2000 yd");
}

void test_50bmg_g1_extreme() {
    // .50 BMG 750gr A-Max, G1 BC 1.050, MV 2820
    Calculator calc;
    calc.configure(1.050, DragTableId::G1,
                   750.0, 0.510, 2.31,
                   2820.0, 2.0, 15.0, 100.0);
    calc.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);
    calc.solve();

    auto r = calc.correction(2000.0, CorrectionUnit::MRAD);
    TEST_ASSERT_TRUE_MESSAGE(r.valid, ".50 BMG at 2000 yd valid");
    TEST_ASSERT_TRUE_MESSAGE(r.energy_ftlb > 1000.0,
                             ".50 BMG has energy at 2000 yd");
}

void test_6mm_creedmoor_g7() {
    // 6mm CM 105gr Berger Hybrid, G7 BC 0.275, MV 3050
    Calculator calc;
    calc.configure(0.275, DragTableId::G7,
                   105.0, 0.243, 1.30,
                   3050.0, 1.5, 7.5, 100.0);
    calc.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);
    calc.solve();

    auto r = calc.correction(600.0, CorrectionUnit::MOA);
    TEST_ASSERT_TRUE_MESSAGE(r.valid, "6mm CM 600 yd valid");
    TEST_ASSERT_TRUE_MESSAGE(r.vertical > 3.0 && r.vertical < 20.0,
                             "6mm CM reasonable drop at 600 yd");
}

// ═══════════════════════════════════════════════════════════════════════════
//  9.  Weather Effects
// ═══════════════════════════════════════════════════════════════════════════

void test_hot_vs_cold_weather() {
    Calculator calc_hot, calc_cold;

    auto setupCalc = [](Calculator& c) {
        c.configure(0.315, DragTableId::G7,
                    140.0, 0.264, 1.35,
                    2710.0, 1.5, 8.0, 100.0);
    };

    setupCalc(calc_hot);
    calc_hot.setAtmosphere(100.0, 29.92, 0.0);  // 100°F
    calc_hot.solve();

    setupCalc(calc_cold);
    calc_cold.setAtmosphere(20.0, 29.92, 0.0);  // 20°F
    calc_cold.solve();

    auto r_hot  = calc_hot.correction(1000.0, CorrectionUnit::MOA);
    auto r_cold = calc_cold.correction(1000.0, CorrectionUnit::MOA);

    TEST_ASSERT_TRUE_MESSAGE(r_hot.valid && r_cold.valid, "both valid");
    // Hot air is thinner → less drag → less drop → less correction
    TEST_ASSERT_TRUE_MESSAGE(r_hot.vertical < r_cold.vertical,
                             "less correction in hot weather");
}

void test_altitude_effect() {
    Calculator calc_sea, calc_high;

    auto setupCalc = [](Calculator& c) {
        c.configure(0.315, DragTableId::G7,
                    140.0, 0.264, 1.35,
                    2710.0, 1.5, 8.0, 100.0);
    };

    setupCalc(calc_sea);
    calc_sea.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0, 0.0);
    calc_sea.solve();

    setupCalc(calc_high);
    // At 5000 ft elevation, lower pressure
    calc_high.setAtmosphere(kStdTempF, 24.90, 0.0, 5000.0);
    calc_high.solve();

    auto r_sea  = calc_sea.correction(1000.0, CorrectionUnit::MOA);
    auto r_high = calc_high.correction(1000.0, CorrectionUnit::MOA);

    TEST_ASSERT_TRUE_MESSAGE(r_sea.valid && r_high.valid, "both valid");
    // Altitude (thinner air) → less drop
    TEST_ASSERT_TRUE_MESSAGE(r_high.vertical < r_sea.vertical,
                             "less correction at altitude");
}

// ═══════════════════════════════════════════════════════════════════════════
//  10.  Wind
// ═══════════════════════════════════════════════════════════════════════════

void test_crosswind() {
    Calculator calc;
    calc.configure(0.315, DragTableId::G7,
                   140.0, 0.264, 1.35,
                   2710.0, 1.5, 8.0, 100.0);
    calc.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);

    // 10 mph full crosswind from left (90°)
    double wind_fps = 10.0 * 5280.0 / 3600.0;
    calc.setWind(wind_fps, 90.0);
    calc.solve();

    auto r = calc.correction(500.0, CorrectionUnit::MOA);
    TEST_ASSERT_TRUE_MESSAGE(r.valid, "crosswind 500 yd valid");
    TEST_ASSERT_TRUE_MESSAGE(r.horizontal > 0.5,
                             "should have significant windage");
}

void test_no_wind_zero_windage() {
    Calculator calc;
    calc.configure(0.315, DragTableId::G7,
                   140.0, 0.264, 1.35,
                   2710.0, 1.5, 8.0, 100.0);
    calc.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);
    calc.setWind(0.0, 0.0);
    calc.solve();

    auto r = calc.correction(500.0, CorrectionUnit::MOA);
    // Without wind, windage comes only from spin drift (Litz formula)
    // For 6.5CM with 8" twist at 500 yd, spin drift is typically 1-3 MOA
    char buf[128];
    snprintf(buf, sizeof(buf), "windage without wind = %.2f MOA", r.horizontal);
    TEST_ASSERT_TRUE_MESSAGE(r.horizontal < 5.0, buf);
}

// ═══════════════════════════════════════════════════════════════════════════
//  11.  Coriolis
// ═══════════════════════════════════════════════════════════════════════════

void test_coriolis_effect() {
    Calculator calc_no, calc_yes;

    auto setupCalc = [](Calculator& c) {
        c.configure(0.417, DragTableId::G7,
                    300.0, 0.338, 1.70,
                    2750.0, 1.5, 9.375, 100.0);
        c.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);
        c.setWind(0.0, 0.0);
    };

    setupCalc(calc_no);
    calc_no.disableCoriolis();
    calc_no.solve();

    setupCalc(calc_yes);
    calc_yes.setCoriolis(45.0, 90.0);  // 45°N, shooting East
    calc_yes.solve();

    auto r_no  = calc_no.correction(2000.0, CorrectionUnit::MOA);
    auto r_yes = calc_yes.correction(2000.0, CorrectionUnit::MOA);

    TEST_ASSERT_TRUE_MESSAGE(r_no.valid && r_yes.valid, "both valid");
    // Coriolis should change horizontal correction at long range
    double h_diff = std::abs(r_yes.horizontal - r_no.horizontal);
    TEST_ASSERT_TRUE_MESSAGE(h_diff > 0.01,
                             "Coriolis should affect windage at 2000 yd");
}

// ═══════════════════════════════════════════════════════════════════════════
//  12.  Correction Units
// ═══════════════════════════════════════════════════════════════════════════

void test_correction_units_consistency() {
    Calculator calc;
    calc.configure(0.315, DragTableId::G7,
                   140.0, 0.264, 1.35,
                   2710.0, 1.5, 8.0, 100.0);
    calc.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);
    calc.solve();

    auto r_moa  = calc.correction(600.0, CorrectionUnit::MOA);
    auto r_mrad = calc.correction(600.0, CorrectionUnit::MRAD);

    // 1 MRAD ≈ 3.438 MOA
    double ratio = r_moa.vertical / r_mrad.vertical;
    ASSERT_NEAR(3.438, ratio, 0.05, "MOA/MRAD ratio ~3.438");
}

void test_correction_clicks() {
    Calculator calc;
    calc.configure(0.315, DragTableId::G7,
                   140.0, 0.264, 1.35,
                   2710.0, 1.5, 8.0, 100.0);
    calc.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);
    calc.solve();

    double click_moa = 0.25;
    double click_rad = click_moa * kMoaToRad;

    auto r_moa   = calc.correction(600.0, CorrectionUnit::MOA);
    auto r_click = calc.correction(600.0, CorrectionUnit::CLICKS, click_rad);

    // Clicks = MOA / 0.25
    double expected_clicks = r_moa.vertical / click_moa;
    ASSERT_NEAR(expected_clicks, r_click.vertical, 1.0, "clicks ≈ MOA/0.25");
}

// ═══════════════════════════════════════════════════════════════════════════
//  13.  Powder Temperature Sensitivity
// ═══════════════════════════════════════════════════════════════════════════

void test_powder_sensitivity() {
    Calculator calc_no, calc_cold;

    auto setupCalc = [](Calculator& c) {
        c.configure(0.315, DragTableId::G7,
                    140.0, 0.264, 1.35,
                    2710.0, 1.5, 8.0, 100.0);
        c.setAtmosphere(20.0, 29.92, 0.0);  // 20°F cold
    };

    setupCalc(calc_no);
    calc_no.solve();

    setupCalc(calc_cold);
    // Strong sensitivity: 1.5 fps/°F, normalized
    calc_cold.setPowderSensitivity(59.0, 1.5 * 15.0 / 2710.0);
    calc_cold.solve();

    auto r_no   = calc_no.correction(1000.0, CorrectionUnit::MOA);
    auto r_cold = calc_cold.correction(1000.0, CorrectionUnit::MOA);

    TEST_ASSERT_TRUE_MESSAGE(r_no.valid && r_cold.valid, "both valid");
    // Cold powder → lower MV → more drop
    TEST_ASSERT_TRUE_MESSAGE(r_cold.vertical > r_no.vertical,
                             "cold powder → more drop");
}

// ═══════════════════════════════════════════════════════════════════════════
//  14.  Multi-BC
// ═══════════════════════════════════════════════════════════════════════════

void test_multi_bc() {
    Calculator calc;
    BCPoint pts[] = {
        {0.315, 2700.0},
        {0.310, 2200.0},
        {0.290, 1800.0}
    };
    calc.configureMultiBC(pts, 3, DragTableId::G7,
                          140.0, 0.264, 1.35,
                          2710.0, 1.5, 8.0, 100.0);
    calc.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);
    calc.solve();

    auto r = calc.correction(1000.0, CorrectionUnit::MOA);
    TEST_ASSERT_TRUE_MESSAGE(r.valid, "multi-BC 1000 yd valid");
    // Multi-BC with lower BC at lower velocities → more drop than single 0.315
    Calculator calc_single;
    calc_single.configure(0.315, DragTableId::G7,
                          140.0, 0.264, 1.35,
                          2710.0, 1.5, 8.0, 100.0);
    calc_single.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);
    calc_single.solve();
    auto r_s = calc_single.correction(1000.0, CorrectionUnit::MOA);

    TEST_ASSERT_TRUE_MESSAGE(r.vertical > r_s.vertical,
                             "multi-BC lower effective BC → more drop");
}

// ═══════════════════════════════════════════════════════════════════════════
//  15.  Edge Cases
// ═══════════════════════════════════════════════════════════════════════════

void test_zero_distance_correction() {
    Calculator calc;
    calc.configure(0.315, DragTableId::G7,
                   140.0, 0.264, 1.35,
                   2710.0, 1.5, 8.0, 100.0);
    calc.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);
    calc.solve();

    auto r = calc.correction(0.0, CorrectionUnit::MOA);
    TEST_ASSERT_FALSE_MESSAGE(r.valid, "0 yd should be invalid");
}

void test_very_close_range() {
    Calculator calc;
    calc.configure(0.315, DragTableId::G7,
                   140.0, 0.264, 1.35,
                   2710.0, 1.5, 8.0, 100.0);
    calc.setAtmosphere(kStdTempF, kStdPressureInHg, 0.0);
    calc.solve();

    auto r = calc.correction(25.0, CorrectionUnit::MOA);
    TEST_ASSERT_TRUE_MESSAGE(r.valid, "25 yd should work");
}

// ═══════════════════════════════════════════════════════════════════════════
//  Test Runner
// ═══════════════════════════════════════════════════════════════════════════

void setUp() {}
void tearDown() {}

int main(int argc, char** argv) {
    UNITY_BEGIN();

    // Constants
    RUN_TEST(test_constants_gravity);
    RUN_TEST(test_constants_std_atmosphere);
    RUN_TEST(test_constants_conversions);

    // Vector3
    RUN_TEST(test_vector3_addition);
    RUN_TEST(test_vector3_magnitude);
    RUN_TEST(test_vector3_normalize);
    RUN_TEST(test_vector3_scalar_multiply);
    RUN_TEST(test_vector3_dot);

    // PCHIP
    RUN_TEST(test_pchip_basic);
    RUN_TEST(test_pchip_g7_table);

    // Atmosphere
    RUN_TEST(test_atmosphere_standard);
    RUN_TEST(test_atmosphere_hot);
    RUN_TEST(test_atmosphere_altitude_correction);
    RUN_TEST(test_atmosphere_humidity_effect);
    RUN_TEST(test_atmosphere_unit_conversions);

    // Drag model
    RUN_TEST(test_drag_model_single_bc);
    RUN_TEST(test_drag_tables_all_exist);

    // Zero-finding
    RUN_TEST(test_zero_finding_basic);
    RUN_TEST(test_zero_finding_long_range);

    // Trajectory 6.5CM
    RUN_TEST(test_trajectory_65cm_drop_increases);
    RUN_TEST(test_trajectory_65cm_velocity_decreases);
    RUN_TEST(test_trajectory_65cm_reasonable_values);

    // Multiple calibers
    RUN_TEST(test_308_g7_trajectory);
    RUN_TEST(test_338lm_g7_extreme_range);
    RUN_TEST(test_50bmg_g1_extreme);
    RUN_TEST(test_6mm_creedmoor_g7);

    // Weather
    RUN_TEST(test_hot_vs_cold_weather);
    RUN_TEST(test_altitude_effect);

    // Wind
    RUN_TEST(test_crosswind);
    RUN_TEST(test_no_wind_zero_windage);

    // Coriolis
    RUN_TEST(test_coriolis_effect);

    // Correction units
    RUN_TEST(test_correction_units_consistency);
    RUN_TEST(test_correction_clicks);

    // Powder sensitivity
    RUN_TEST(test_powder_sensitivity);

    // Multi-BC
    RUN_TEST(test_multi_bc);

    // Edge cases
    RUN_TEST(test_zero_distance_correction);
    RUN_TEST(test_very_close_range);

    return UNITY_END();
}
