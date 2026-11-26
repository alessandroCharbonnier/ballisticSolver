/// Sessions and the active rifle (rules S1, S3).
library;

import '../domain/models.dart';
import 'app_database.dart';

/// A row of the Data list: a session with its counters.
class SessionSummary {
  final Session session;
  final int shotCount;
  final int unassignedCount;

  const SessionSummary({
    required this.session,
    required this.shotCount,
    required this.unassignedCount,
  });
}

class SessionRepository {
  SessionRepository(this.db);

  final AppDatabase db;

  // ── rifle ─────────────────────────────────────────────────────────────────

  /// The active rifle: the first one (only one exists in phase 1).
  Future<Rifle?> activeRifle() async {
    final database = await db.database;
    final rows = await database.query('rifles', orderBy: 'id', limit: 1);
    if (rows.isEmpty) return null;
    return Rifle.fromMap(rows.first);
  }

  Future<Rifle?> getRifle(int id) async {
    final database = await db.database;
    final rows = await database.query(
      'rifles',
      where: 'id = ?',
      whereArgs: [id],
      limit: 1,
    );
    if (rows.isEmpty) return null;
    return Rifle.fromMap(rows.first);
  }

  Future<void> updateRifleName(int id, String name) async {
    final database = await db.database;
    await database.update(
      'rifles',
      {'name': name},
      where: 'id = ?',
      whereArgs: [id],
    );
  }

  // ── sessions ──────────────────────────────────────────────────────────────

  /// S1: the session of the active rifle started today (local date), if any.
  Future<Session?> findSessionForDay(int rifleId, DateTime now) async {
    final local = now.toLocal();
    final startOfDay = DateTime(local.year, local.month, local.day);
    final endOfDay = startOfDay.add(const Duration(days: 1));

    final database = await db.database;
    final rows = await database.query(
      'sessions',
      where: 'rifle_id = ? AND started_at >= ? AND started_at < ?',
      whereArgs: [
        rifleId,
        startOfDay.millisecondsSinceEpoch,
        endOfDay.millisecondsSinceEpoch,
      ],
      orderBy: 'started_at DESC',
      limit: 1,
    );
    if (rows.isEmpty) return null;
    return Session.fromMap(rows.first);
  }

  /// S3: `clean_base` is fixed when the session is created.
  ///
  /// 0 if the barrel was cleaned; otherwise the `shots_since_clean` of the
  /// rifle's most recent non-voided shot in earlier sessions, or 0 if none.
  Future<int> computeCleanBase(int rifleId) async {
    final database = await db.database;
    final rows = await database.rawQuery('''
      SELECT s.shots_since_clean
      FROM shots s
      JOIN sessions se ON se.id = s.session_id
      WHERE se.rifle_id = ? AND s.voided = 0
      ORDER BY s.fired_at DESC
      LIMIT 1
    ''', [rifleId]);
    if (rows.isEmpty) return 0;
    return (rows.first['shots_since_clean'] as num?)?.toInt() ?? 0;
  }

  Future<Session> createSession({
    required int rifleId,
    required bool barrelCleaned,
    required DateTime now,
  }) async {
    final cleanBase = barrelCleaned ? 0 : await computeCleanBase(rifleId);
    final database = await db.database;
    final id = await database.insert('sessions', {
      'rifle_id': rifleId,
      'started_at': now.millisecondsSinceEpoch,
      'last_active_at': now.millisecondsSinceEpoch,
      'barrel_cleaned': barrelCleaned ? 1 : 0,
      'clean_base': cleanBase,
      'manual_temp_c': null,
    });
    return Session(
      id: id,
      rifleId: rifleId,
      startedAt: now,
      lastActiveAt: now,
      barrelCleaned: barrelCleaned,
      cleanBase: cleanBase,
      manualTempC: null,
    );
  }

  Future<Session?> getSession(int id) async {
    final database = await db.database;
    final rows = await database.query(
      'sessions',
      where: 'id = ?',
      whereArgs: [id],
      limit: 1,
    );
    if (rows.isEmpty) return null;
    return Session.fromMap(rows.first);
  }

  Future<void> updateLastActive(int sessionId, DateTime now) async {
    final database = await db.database;
    await database.update(
      'sessions',
      {'last_active_at': now.millisecondsSinceEpoch},
      where: 'id = ?',
      whereArgs: [sessionId],
    );
  }

  Future<void> setManualTemp(int sessionId, double? tempC) async {
    final database = await db.database;
    await database.update(
      'sessions',
      {'manual_temp_c': tempC},
      where: 'id = ?',
      whereArgs: [sessionId],
    );
  }

  Future<List<SessionSummary>> listSessions() async {
    final database = await db.database;
    final rows = await database.rawQuery('''
      SELECT
        se.*,
        SUM(CASE WHEN s.id IS NOT NULL AND s.voided = 0 THEN 1 ELSE 0 END)
          AS shot_count,
        SUM(CASE WHEN s.cartridge_id IS NULL AND s.voided = 0 THEN 1 ELSE 0 END)
          AS unassigned_count
      FROM sessions se
      LEFT JOIN shots s ON s.session_id = se.id
      GROUP BY se.id
      ORDER BY se.started_at DESC
    ''');
    return rows.map((r) => SessionSummary(
          session: Session.fromMap(r),
          shotCount: (r['shot_count'] as num?)?.toInt() ?? 0,
          unassignedCount: (r['unassigned_count'] as num?)?.toInt() ?? 0,
        )).toList();
  }
}
