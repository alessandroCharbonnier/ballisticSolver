/// Slot numbering maths (rules G1 to G4).
library;

class SlotMath {
  /// G2: row of a slot, 1-based, left to right then top to bottom.
  static int rowOf(int slot, int cols) => ((slot - 1) ~/ cols) + 1;

  /// G2: column of a slot, 1-based.
  static int colOf(int slot, int cols) => ((slot - 1) % cols) + 1;

  /// G3: label such as `R2 C4`.
  static String positionLabel(int slot, int cols) =>
      'R${rowOf(slot, cols)} C${colOf(slot, cols)}';

  /// G3: label with the box code, such as `B01 R2 C4`.
  static String withBox(String boxCode, int slot, int cols) =>
      '$boxCode ${positionLabel(slot, cols)}';

  /// Slot number from a 1-based row and column.
  static int slotOf(int row, int col, int cols) => (row - 1) * cols + col;

  /// G4: total number of slots in a box.
  static int totalSlots(int rows, int cols) => rows * cols;
}
