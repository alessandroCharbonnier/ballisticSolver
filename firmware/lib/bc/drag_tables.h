/*
 * drag_tables.h - Standard drag coefficient tables
 *
 * Cd vs Mach data for G1, G7, G2, G5, G6, G8, GI, GS, RA4 profiles.
 * Port of py_ballisticcalc.drag_tables
 */

#pragma once

#include <cstddef>

namespace bc {

struct DragPoint {
    double mach;
    double cd;
};

// Forward declarations
extern const DragPoint TABLE_G1[];
extern const size_t    TABLE_G1_SIZE;

extern const DragPoint TABLE_G7[];
extern const size_t    TABLE_G7_SIZE;

extern const DragPoint TABLE_G2[];
extern const size_t    TABLE_G2_SIZE;

extern const DragPoint TABLE_G5[];
extern const size_t    TABLE_G5_SIZE;

extern const DragPoint TABLE_G6[];
extern const size_t    TABLE_G6_SIZE;

extern const DragPoint TABLE_G8[];
extern const size_t    TABLE_G8_SIZE;

extern const DragPoint TABLE_GI[];
extern const size_t    TABLE_GI_SIZE;

extern const DragPoint TABLE_GS[];
extern const size_t    TABLE_GS_SIZE;

extern const DragPoint TABLE_RA4[];
extern const size_t    TABLE_RA4_SIZE;

enum class DragTableId : uint8_t {
    G1 = 0,
    G7 = 1,
    G2 = 2,
    G5 = 3,
    G6 = 4,
    G8 = 5,
    GI = 6,
    GS = 7,
    RA4 = 8,
    COUNT
};

// Get table and size by id
const DragPoint* get_drag_table(DragTableId id);
size_t get_drag_table_size(DragTableId id);
const char* get_drag_table_name(DragTableId id);

}  // namespace bc
