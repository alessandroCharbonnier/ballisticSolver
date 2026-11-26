/// Shots (rules F1 to F4, F6, F7).
///
/// Saving a shot and changing the cartridge happen in one transaction (rule
/// F4). Undo voids the most recent non-voided shot and, when it had a
/// cartridge, returns the cartridge to the top of the magazine.
library;

import '../domain/constants.dart';
import '../domain/models.dart';
import '../domain/shot_context.dart';
import '../domain/slots.dart';
import '../domain/weather_tracker.dart';
import 'app_database.dart';

class RecordShotOutcome {
  final Shot shot;
  final String? cartridgeLabel; // 'B01 R2 C4' when assigned
  final bool assigned;

  const RecordShotOutcome({
    required this.shot,
    required this.cartridgeLabel,
    required this.assigned,
  });
}

enum UndoStatus { done, nothingToUndo, refusedMagazineFull }

class UndoOutcome {
  final UndoStatus status;
  final Shot? voidedShot;
  final String? cartridgeLabel;

  const UndoOutcome({
    required this.status,
    this.voidedShot,
    this.cartridgeLabel,
  });
}

class ShotRepository {
  ShotRepository(this.db);

  final AppDatabase db;

  /// All shots of a session in firing order, voided included.
  Future<List<Shot>> shotsForSession(int sessionId) async {
    final database = await db.database;
    final rows = await database.query(
      'shots',
      where: 'session_id = ?',
      whereArgs: [sessionId],
      orderBy: 'shot_index',
    );
    return rows.map(Shot.fromMap).toList();
  }

  /// F1 to F4: record a shot. If the magazine is not empty the shot is
  /// attached to the top cartridge, which becomes `fired` and leaves the
  /// magazine. Otherwise the shot is unassigned. One transaction.
  Future<RecordShotOutcome> recordShot({
    required int sessionId,
    required ShotContext context,
    required WeatherForShot weather,
    required DateTime now,
    required int freqHz,
    required double bc,
  }) async {
    final database = await db.database;
    return database.transaction((txn) async {
      final topRows = await txn.rawQuery('''
        SELECT c.*, b.code AS box_code, p.cols AS cols
        FROM cartridges c
        JOIN boxes b ON b.id = c.box_id
        JOIN lots l ON l.id = b.lot_id
        JOIN products p ON p.id = l.product_id
        WHERE c.state = 'magazine'
        ORDER BY c.mag_position DESC
        LIMIT 1
      ''');

      int? cartridgeId;
      String? cartridgeLabel;

      if (topRows.isNotEmpty) {
        final r = topRows.first;
        cartridgeId = r['id'] as int;
        final boxCode = r['box_code'] as String;
        final cols = (r['cols'] as num?)?.toInt() ?? AppConstants.defaultCols;
        final slot = r['slot'] as int;
        cartridgeLabel = SlotMath.withBox(boxCode, slot, cols);

        // F2: the cartridge becomes `fired` and leaves the magazine.
        await txn.update(
          'cartridges',
          {'state': 'fired', 'mag_position': null},
          where: 'id = ?',
          whereArgs: [cartridgeId],
        );
      }

      final shot = Shot(
        sessionId: sessionId,
        cartridgeId: cartridgeId,
        shotIndex: context.shotIndex,
        magPosition: cartridgeId != null
            ? (topRows.first['mag_position'] as int?)
            : null,
        freqHz: freqHz,
        bc: bc,
        firedAt: now,
        tempC: weather.tempC,
        pressureHpa: weather.pressureHpa,
        humidityPct: weather.humidityPct,
        weatherSource: weather.source,
        coldBore: context.coldBore,
        shotsSinceClean: context.shotsSinceClean,
        timeSinceLastShotS: context.timeSinceLastShotS,
        voided: false,
      );

      final id = await txn.insert('shots', shot.toMap());
      final saved = Shot.fromMap({
        ...shot.toMap(),
        'id': id,
        'voided': 0,
      });

      return RecordShotOutcome(
        shot: saved,
        cartridgeLabel: cartridgeLabel,
        assigned: cartridgeId != null,
      );
    });
  }

  /// F6: undo voids the most recent non-voided shot of the session.
  ///
  /// If that shot had a cartridge, the cartridge returns to the top of the
  /// magazine. If the magazine already holds 10, undo is refused and nothing
  /// changes.
  Future<UndoOutcome> undoLast({
    required int sessionId,
    required DateTime now,
  }) async {
    final database = await db.database;
    return database.transaction((txn) async {
      final shotRows = await txn.query(
        'shots',
        where: 'session_id = ? AND voided = 0',
        whereArgs: [sessionId],
        orderBy: 'shot_index DESC',
        limit: 1,
      );
      if (shotRows.isEmpty) {
        return const UndoOutcome(status: UndoStatus.nothingToUndo);
      }

      final shot = Shot.fromMap(shotRows.first);
      String? cartridgeLabel;

      if (shot.cartridgeId != null) {
        final countRows = await txn.rawQuery(
          "SELECT COUNT(*) AS n FROM cartridges WHERE state = 'magazine'",
        );
        final currentCount = (countRows.first['n'] as num?)?.toInt() ?? 0;
        if (currentCount >= AppConstants.magazineCapacity) {
          return const UndoOutcome(status: UndoStatus.refusedMagazineFull);
        }

        // Fetch the cartridge's box code and grid size for the label.
        final cartridgeRows = await txn.rawQuery('''
          SELECT c.slot AS slot, b.code AS box_code, p.cols AS cols
          FROM cartridges c
          JOIN boxes b ON b.id = c.box_id
          JOIN lots l ON l.id = b.lot_id
          JOIN products p ON p.id = l.product_id
          WHERE c.id = ?
        ''', [shot.cartridgeId]);
        if (cartridgeRows.isNotEmpty) {
          final r = cartridgeRows.first;
          cartridgeLabel = SlotMath.withBox(
            r['box_code'] as String,
            r['slot'] as int,
            (r['cols'] as num?)?.toInt() ?? AppConstants.defaultCols,
          );
        }

        // Return the cartridge to the top of the magazine.
        await txn.update(
          'cartridges',
          {'state': 'magazine', 'mag_position': currentCount + 1},
          where: 'id = ?',
          whereArgs: [shot.cartridgeId],
        );
      }

      await txn.update(
        'shots',
        {'voided': 1, 'voided_at': now.millisecondsSinceEpoch},
        where: 'id = ?',
        whereArgs: [shot.id],
      );

      return UndoOutcome(
        status: UndoStatus.done,
        voidedShot: shot,
        cartridgeLabel: cartridgeLabel,
      );
    });
  }

  /// A shot with its cartridge label for the session detail table (section
  /// 11.5).
  Future<List<ShotDetail>> shotDetails(int sessionId) async {
    final database = await db.database;
    final rows = await database.rawQuery('''
      SELECT sh.*, b.code AS box_code, c.slot AS slot_in_box, p.cols AS cols
      FROM shots sh
      LEFT JOIN cartridges c ON c.id = sh.cartridge_id
      LEFT JOIN boxes b ON b.id = c.box_id
      LEFT JOIN lots l ON l.id = b.lot_id
      LEFT JOIN products p ON p.id = l.product_id
      WHERE sh.session_id = ?
      ORDER BY sh.shot_index
    ''', [sessionId]);

    return rows.map((r) {
      final shot = Shot.fromMap(r);
      String? label;
      if (shot.cartridgeId != null && r['box_code'] != null) {
        label = SlotMath.withBox(
          r['box_code'] as String,
          r['slot_in_box'] as int,
          (r['cols'] as num?)?.toInt() ?? AppConstants.defaultCols,
        );
      }
      return ShotDetail(shot: shot, cartridgeLabel: label);
    }).toList();
  }

  /// W7: add weather to shots whose source is `none`. Those shots get source
  /// `manual`. Temperature is required; pressure and humidity are optional.
  /// Returns the number of updated shots.
  Future<int> setWeatherForShotsWithoutData({
    required int sessionId,
    required double tempC,
    double? pressureHpa,
    double? humidityPct,
  }) async {
    final database = await db.database;
    return database.update(
      'shots',
      {
        'temp_c': tempC,
        'pressure_hpa': ?pressureHpa,
        'humidity_pct': ?humidityPct,
        'weather_source': 'manual',
      },
      where: 'session_id = ? AND weather_source = ?',
      whereArgs: [sessionId, 'none'],
    );
  }
}

/// A shot plus its cartridge label for display.
class ShotDetail {
  final Shot shot;
  final String? cartridgeLabel;

  const ShotDetail({required this.shot, required this.cartridgeLabel});
}
