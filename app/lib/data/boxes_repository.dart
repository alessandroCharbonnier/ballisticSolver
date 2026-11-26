/// Products, lots, boxes and cartridges (the measure side).
library;

import '../domain/models.dart';
import '../domain/slots.dart';
import 'app_database.dart';

/// A row of the Boxes list: a box with its product, lot and counters.
class BoxListItem {
  final Box box;
  final String productName;
  final String lotNumber;
  final int measuredCount;
  final int firedCount;
  final int totalSlots;

  const BoxListItem({
    required this.box,
    required this.productName,
    required this.lotNumber,
    required this.measuredCount,
    required this.firedCount,
    required this.totalSlots,
  });
}

class BoxesRepository {
  BoxesRepository(this.db);

  final AppDatabase db;

  // ── products ──────────────────────────────────────────────────────────────

  Future<int?> findProductIdByName(String name) async {
    final database = await db.database;
    final rows = await database.query(
      'products',
      columns: ['id'],
      where: 'name = ?',
      whereArgs: [name],
      limit: 1,
    );
    if (rows.isEmpty) return null;
    return rows.first['id'] as int?;
  }

  Future<int> insertProduct(Product product, {required DateTime now}) async {
    final database = await db.database;
    final map = product.toMap()..['created_at'] = now.millisecondsSinceEpoch;
    return database.insert('products', map);
  }

  Future<Product?> getProduct(int id) async {
    final database = await db.database;
    final rows = await database.query(
      'products',
      where: 'id = ?',
      whereArgs: [id],
      limit: 1,
    );
    if (rows.isEmpty) return null;
    return Product.fromMap(rows.first);
  }

  Future<List<Product>> listProducts() async {
    final database = await db.database;
    final rows = await database.query('products', orderBy: 'name');
    return rows.map(Product.fromMap).toList();
  }

  // ── lots ──────────────────────────────────────────────────────────────────

  Future<int?> findLotId(int productId, String lotNumber) async {
    final database = await db.database;
    final rows = await database.query(
      'lots',
      columns: ['id'],
      where: 'product_id = ? AND lot_number = ?',
      whereArgs: [productId, lotNumber],
      limit: 1,
    );
    if (rows.isEmpty) return null;
    return rows.first['id'] as int?;
  }

  Future<int> insertLot(Lot lot, {required DateTime now}) async {
    final database = await db.database;
    final map = lot.toMap()..['created_at'] = now.millisecondsSinceEpoch;
    return database.insert('lots', map);
  }

  Future<Lot?> getLot(int id) async {
    final database = await db.database;
    final rows = await database.query(
      'lots',
      where: 'id = ?',
      whereArgs: [id],
      limit: 1,
    );
    if (rows.isEmpty) return null;
    return Lot.fromMap(rows.first);
  }

  Future<List<Lot>> listLotsForProduct(int productId) async {
    final database = await db.database;
    final rows = await database.query(
      'lots',
      where: 'product_id = ?',
      whereArgs: [productId],
      orderBy: 'lot_number',
    );
    return rows.map(Lot.fromMap).toList();
  }

  // ── boxes ─────────────────────────────────────────────────────────────────

  Future<bool> boxCodeExists(String code) async {
    final database = await db.database;
    final rows = await database.query(
      'boxes',
      columns: ['id'],
      where: 'code = ?',
      whereArgs: [code],
      limit: 1,
    );
    return rows.isNotEmpty;
  }

  /// The next free box code: `B01`, `B02`, ... (the highest existing numeric
  /// suffix plus one).
  Future<String> nextFreeBoxCode() async {
    final database = await db.database;
    final rows = await database.query('boxes', columns: ['code']);
    var highest = 0;
    for (final row in rows) {
      final code = row['code'] as String? ?? '';
      final match = RegExp(r'^B(\d+)$').firstMatch(code);
      if (match != null) {
        final n = int.tryParse(match.group(1)!);
        if (n != null && n > highest) highest = n;
      }
    }
    return 'B${(highest + 1).toString().padLeft(2, '0')}';
  }

  /// Create a box and all its empty cartridges in one transaction (section
  /// 11.2). Returns the new box id.
  Future<int> createBox({
    required int lotId,
    required String code,
    required int rows,
    required int cols,
    required DateTime now,
  }) async {
    final database = await db.database;
    return database.transaction((txn) async {
      final boxId = await txn.insert('boxes', {
        'lot_id': lotId,
        'code': code,
        'hidden': 0,
        'created_at': now.millisecondsSinceEpoch,
      });

      final total = SlotMath.totalSlots(rows, cols);
      final batch = txn.batch();
      for (var slot = 1; slot <= total; slot++) {
        batch.insert('cartridges', {
          'box_id': boxId,
          'slot': slot,
          'state': 'box',
        });
      }
      await batch.commit(noResult: true);

      return boxId;
    });
  }

  Future<Box?> getBox(int id) async {
    final database = await db.database;
    final rows = await database.query(
      'boxes',
      where: 'id = ?',
      whereArgs: [id],
      limit: 1,
    );
    if (rows.isEmpty) return null;
    return Box.fromMap(rows.first);
  }

  Future<List<BoxListItem>> listVisibleBoxes() async {
    final database = await db.database;
    final rows = await database.rawQuery('''
      SELECT
        b.id AS box_id,
        b.lot_id,
        b.code,
        b.hidden,
        b.created_at,
        p.name AS product_name,
        p.rows AS rows,
        p.cols AS cols,
        l.lot_number,
        SUM(CASE WHEN c.weight_g IS NOT NULL AND c.rim_thickness_mm IS NOT NULL
                 THEN 1 ELSE 0 END) AS measured_count,
        SUM(CASE WHEN c.state = 'fired' THEN 1 ELSE 0 END) AS fired_count,
        COUNT(c.id) AS total_slots
      FROM boxes b
      JOIN lots l ON l.id = b.lot_id
      JOIN products p ON p.id = l.product_id
      LEFT JOIN cartridges c ON c.box_id = b.id
      WHERE b.hidden = 0
      GROUP BY b.id
      ORDER BY b.created_at DESC, b.id DESC
    ''');
    return rows.map((r) => BoxListItem(
          box: Box(
            id: r['box_id'] as int,
            lotId: r['lot_id'] as int,
            code: r['code'] as String,
            hidden: false,
            createdAt: _dateFromMap(r['created_at']),
          ),
          productName: r['product_name'] as String,
          lotNumber: r['lot_number'] as String,
          measuredCount: (r['measured_count'] as num?)?.toInt() ?? 0,
          firedCount: (r['fired_count'] as num?)?.toInt() ?? 0,
          totalSlots: (r['total_slots'] as num?)?.toInt() ?? 0,
        )).toList();
  }

  Future<bool> hasCartridgeInMagazine(int boxId) async {
    final database = await db.database;
    final rows = await database.query(
      'cartridges',
      columns: ['id'],
      where: 'box_id = ? AND state = ?',
      whereArgs: [boxId, 'magazine'],
      limit: 1,
    );
    return rows.isNotEmpty;
  }

  Future<void> hideBox(int boxId) async {
    final database = await db.database;
    await database.update(
      'boxes',
      {'hidden': 1},
      where: 'id = ?',
      whereArgs: [boxId],
    );
  }

  /// Product of a box (via lot).
  Future<Product?> productForBox(int boxId) async {
    final database = await db.database;
    final rows = await database.rawQuery(
      'SELECT p.* FROM boxes b '
      'JOIN lots l ON l.id = b.lot_id '
      'JOIN products p ON p.id = l.product_id '
      'WHERE b.id = ?',
      [boxId],
    );
    if (rows.isEmpty) return null;
    return Product.fromMap(rows.first);
  }

  /// Lot of a box.
  Future<Lot?> lotForBox(int boxId) async {
    final database = await db.database;
    final rows = await database.rawQuery(
      'SELECT l.* FROM boxes b JOIN lots l ON l.id = b.lot_id WHERE b.id = ?',
      [boxId],
    );
    if (rows.isEmpty) return null;
    return Lot.fromMap(rows.first);
  }

  // ── cartridges ────────────────────────────────────────────────────────────

  Future<List<Cartridge>> getCartridges(int boxId) async {
    final database = await db.database;
    final rows = await database.query(
      'cartridges',
      where: 'box_id = ?',
      whereArgs: [boxId],
      orderBy: 'slot',
    );
    return rows.map(Cartridge.fromMap).toList();
  }

  Future<Cartridge?> getCartridge(int cartridgeId) async {
    final database = await db.database;
    final rows = await database.query(
      'cartridges',
      where: 'id = ?',
      whereArgs: [cartridgeId],
      limit: 1,
    );
    if (rows.isEmpty) return null;
    return Cartridge.fromMap(rows.first);
  }

  /// Save or correct a measurement (rule M5). Sets `measured_at` to now.
  Future<void> saveMeasurement({
    required int cartridgeId,
    required double weightG,
    required double rimThicknessMm,
    required DateTime now,
  }) async {
    final database = await db.database;
    await database.update(
      'cartridges',
      {
        'weight_g': weightG,
        'rim_thickness_mm': rimThicknessMm,
        'measured_at': now.millisecondsSinceEpoch,
      },
      where: 'id = ?',
      whereArgs: [cartridgeId],
    );
  }

  /// Measured values of the other cartridges in a box, for the box-median
  /// warning (rule M3). The edited cartridge is excluded.
  Future<({List<double> weights, List<double> rims})> boxMeasuredValues(
    int boxId, {
    int? excludeCartridgeId,
  }) async {
    final database = await db.database;
    final rows = await database.query(
      'cartridges',
      columns: ['id', 'weight_g', 'rim_thickness_mm'],
      where: 'box_id = ? AND weight_g IS NOT NULL AND rim_thickness_mm IS NOT NULL',
      whereArgs: [boxId],
    );
    final weights = <double>[];
    final rims = <double>[];
    for (final row in rows) {
      if (row['id'] == excludeCartridgeId) continue;
      weights.add((row['weight_g'] as num).toDouble());
      rims.add((row['rim_thickness_mm'] as num).toDouble());
    }
    return (weights: weights, rims: rims);
  }
}

DateTime? _dateFromMap(Object? value) {
  if (value == null) return null;
  return DateTime.fromMillisecondsSinceEpoch(value as int, isUtc: true);
}
