# .22LR MV Prediction — Prototype Spec

Predict muzzle velocity from cartridge measurements, then sort-and-batch ammo into
consistent-velocity groups. Proof-of-work first, real model later.

## 1. Goal

Given measured cartridges (weight, rim thickness, OAL) + environment, predict each
cartridge's **muzzle velocity (MV)** before firing. Use the predictions to sort the
ammo into batches that shoot the same MV.

Target = **MV only** (BC is a later model).

## 2. Data schema — one CSV row per fired shot

| Column | Type | Source | Notes |
|--------|------|--------|-------|
| `shot_index` | int | auto | firing order, 1..N |
| `mag_position` | int | auto | position in mag (count − k + 1) |
| `box_id` | str | manual | which box |
| `slot_in_box` | int | manual | which slot in that box |
| `weight_g` | float | bench | 0.01 g |
| `rim_thickness_mm` | float | bench | 0.01 mm, one consistent point (or 3-pt avg) |
| `oal_mm` | float | bench | optional |
| `lot` | str | manual | lot number (all same for now) |
| `freq_hz` | int | radar | raw doppler, uint16 BE from `00001624` |
| `bc` | float | radar | float32 BE from same packet |
| `temp_c` | float | auto | device weather |
| `pressure_hpa` | float | auto | optional |
| `humidity_pct` | float | auto | optional |
| `cold_bore` | bool | manual | first shot of session / clean barrel |
| `shots_since_clean` | int | auto | 1 manual "cleaned" reset, then auto-increments |
| `time_since_last_shot_s` | float | auto | device clock |
| `timestamp_ms` | int | auto | device clock |

- `velocity_mps` is **derived**, never measured: `velocity_mps = freq_hz / 160.647`.
- **Store the raw `freq_hz`, not the radar screen value** — the screen rounds to
  0.1 m/s; the freq gives ~0.006 m/s resolution.

## 3. Collection protocol (non-negotiable)

1. **One session** for the 2-box PoW (same day, same barrel, same temp).
2. Clean the barrel **once at the start**; never mid-run.
3. Measure rim thickness at **one consistent point** (or 3-point average).
4. Fixed radar distance.
5. **Never discard flyers.** Flag true misfires separately; don't delete.
6. Temp / time-since-last-shot / shots-since-clean are **automatic** (device-logged).

## 4. Model

**Label**: `freq_hz` (raw; convert to m/s only for reporting — linear transform,
so training on freq or m/s is equivalent).

**Features — core** (PoW): `weight_g`, `rim_thickness_mm`, `temp_c`.
**Features — optional** (log now, use later): `oal_mm`, `cold_bore`,
`shots_since_clean`, `time_since_last_shot_s`.

**Baseline**: Ridge regression (alpha via CV) — proves the signal + gives effect
sizes / coefficients.

**Main model**: LightGBM
- `max_depth` 3–4, `num_leaves` ≤ 16, `learning_rate` 0.05
- early stopping on a validation fold, `subsample` 0.8, `colsample_bytree` 0.8
- regularize hard; with n≈100, shallow is safer.

Also report a **dumb baseline** (predict the mean MV) so every R² is relative.

## 5. Validation

- **5-fold KFold, shuffled, fixed seed** (single lot → no grouping needed now).
- Predict out-of-fold; report: **R², MAE (m/s), RMSE (m/s)**.
- If multi-lot ever happens: **GroupKFold by lot**.

## 6. Grouping evaluation (the real PoW test)

1. Sort all N shots by **predicted MV**.
2. Cut into consecutive bins of `mag_size` (default 10 — Tikka T1X mag).
3. Compute **pooled within-bin SD of ACTUAL MV**.
4. Compare against **1000 random shuffles** (same bin sizes).
5. Report **% SD reduction vs random** + p-value.

PoW passes when the predicted sort produces meaningfully tighter groups than
chance.

## 7. Success criteria

| | Proof-of-work (n=100) | Good enough (n=250–400) |
|---|---|---|
| out-of-fold R² | > ~0.10 | > ~0.30 |
| MAE | — | < ~1.5 m/s |
| grouping SD reduction | **> 30% vs random** | > 40% vs random |

Achievable R² is capped ~0.3–0.5 (one radar reading per cartridge; powder charge
is unmeasured). That's fine — ranking/binning is all the product needs.

## 8. Outputs

1. Console report: R² / MAE / RMSE for mean-baseline vs Ridge vs LightGBM, plus
   the grouping SD reduction.
2. `batch_assignment.csv`: `shot_index, box_id, slot_in_box, predicted_mps,
   actual_mps, batch_id` — sorted by predicted MV, batched consecutively.
3. Predicted-vs-actual scatter (optional, for the write-up).

## 9. Future — new lot

A new lot shifts MV by 5–15 m/s. Handle by a **calibration run**: fire ~20 rounds
of the new lot, learn its offset (mean shift), apply to the rest. Full cross-lot
generalization is a separate, harder problem — out of scope for the PoW.
