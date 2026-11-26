/// The on-screen magazine (rules L). The magazine lives in the database
/// (cartridge state and position), so it survives a restart (rule L6).
library;

import '../domain/constants.dart';
import '../domain/models.dart';
import '../domain/slots.dart';
import 'app_database.dart';

/// A cartridge currently in the magazine, with its box code and grid size so
/// its origin label can be built.
class MagazineEntry {
  final Cartridge cartridge;
  final String boxCode;
  final int cols;

  const MagazineEntry({
    required this.cartridge,
    required this.boxCode,
    required this.cols,
  });

  int get slot => cartridge.slot;
  int get position => cartridge.magPosition ?? 0;
  String get originLabel => SlotMath.withBox(boxCode, slot, cols);
}

enum LoadResult { loaded, notMeasured, notInBox, full }

class UnloadResult {
  final MagazineEntry? entry;
  bool get unloaded => entry != null;
  const UnloadResult(this.entry);
}

class MagazineRepository {
  MagazineRepository(this.db);

  final AppDatabase db;

  Future<int> count() async {
    final database = await db.database;
    final rows = await database.rawQuery(
      "SELECT COUNT(*) AS n FROM cartridges WHERE state = 'magazine'",
    );
    return (rows.first['n'] as num?)?.toInt() ?? 0;
  }

  /// The magazine from the bottom (position 1) to the top.
  Future<List<MagazineEntry>> contents() async {
    final database = await db.database;
    final rows = await database.rawQuery('''
      SELECT c.*, b.code AS box_code, p.cols AS cols
      FROM cartridges c
      JOIN boxes b ON b.id = c.box_id
      JOIN lots l ON l.id = b.lot_id
      JOIN products p ON p.id = l.product_id
      WHERE c.state = 'magazine'
      ORDER BY c.mag_position
    ''');
    return rows.map((r) => MagazineEntry(
          cartridge: Cartridge.fromMap(r),
          boxCode: r['box_code'] as String,
          cols: (r['cols'] as num?)?.toInt() ?? AppConstants.defaultCols,
        )).toList();
  }

  Future<MagazineEntry?> top() async {
    final database = await db.database;
    final rows = await database.rawQuery('''
      SELECT c.*, b.code AS box_code, p.cols AS cols
      FROM cartridges c
      JOIN boxes b ON b.id = c.box_id
      JOIN lots l ON l.id = b.lot_id
      JOIN products p ON p.id = l.product_id
      WHERE c.state = 'magazine'
      ORDER BY c.mag_position DESC
      LIMIT 1
    ''');
    if (rows.isEmpty) return null;
    final r = rows.first;
    return MagazineEntry(
      cartridge: Cartridge.fromMap(r),
      boxCode: r['box_code'] as String,
      cols: (r['cols'] as num?)?.toInt() ?? AppConstants.defaultCols,
    );
  }

  /// L2: load a cartridge. Allowed when it is measured, its state is `box` and
  /// the magazine holds fewer than 10. Runs in one transaction.
  Future<LoadResult> load(int cartridgeId) async {
    final database = await db.database;
    return database.transaction((txn) async {
      final rows = await txn.query(
        'cartridges',
        where: 'id = ?',
        whereArgs: [cartridgeId],
        limit: 1,
      );
      if (rows.isEmpty) return LoadResult.notInBox;
      final cartridge = Cartridge.fromMap(rows.first);

      if (!cartridge.isMeasured) return LoadResult.notMeasured;
      if (cartridge.state != 'box') return LoadResult.notInBox;

      final countRows = await txn.rawQuery(
        "SELECT COUNT(*) AS n FROM cartridges WHERE state = 'magazine'",
      );
      final currentCount = (countRows.first['n'] as num?)?.toInt() ?? 0;
      if (currentCount >= AppConstants.magazineCapacity) return LoadResult.full;

      await txn.update(
        'cartridges',
        {'state': 'magazine', 'mag_position': currentCount + 1},
        where: 'id = ?',
        whereArgs: [cartridgeId],
      );
      return LoadResult.loaded;
    });
  }

  /// L3: unload the top cartridge, returning it to state `box`. With an empty
  /// magazine nothing happens.
  Future<UnloadResult> unloadTop() async {
    final database = await db.database;
    return database.transaction((txn) async {
      final rows = await txn.rawQuery('''
        SELECT c.*, b.code AS box_code, p.cols AS cols
        FROM cartridges c
        JOIN boxes b ON b.id = c.box_id
        JOIN lots l ON l.id = b.lot_id
        JOIN products p ON p.id = l.product_id
        WHERE c.state = 'magazine'
        ORDER BY c.mag_position DESC
        LIMIT 1
      ''');
      if (rows.isEmpty) return const UnloadResult(null);

      final r = rows.first;
      final cartridgeId = r['id'] as int;
      await txn.update(
        'cartridges',
        {'state': 'box', 'mag_position': null},
        where: 'id = ?',
        whereArgs: [cartridgeId],
      );
      return UnloadResult(MagazineEntry(
        cartridge: Cartridge.fromMap(r),
        boxCode: r['box_code'] as String,
        cols: (r['cols'] as num?)?.toInt() ?? AppConstants.defaultCols,
      ));
    });
  }

  /// Return a cartridge in the magazine to the top (used by undo). The
  /// cartridge is set to `magazine` with the next position.
  Future<void> returnToTop(int cartridgeId) async {
    final database = await db.database;
    await database.transaction((txn) async {
      final countRows = await txn.rawQuery(
        "SELECT COUNT(*) AS n FROM cartridges WHERE state = 'magazine'",
      );
      final currentCount = (countRows.first['n'] as num?)?.toInt() ?? 0;
      await txn.update(
        'cartridges',
        {'state': 'magazine', 'mag_position': currentCount + 1},
        where: 'id = ?',
        whereArgs: [cartridgeId],
      );
    });
  }
}
