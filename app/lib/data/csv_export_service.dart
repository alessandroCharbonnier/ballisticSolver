/// CSV export (section 12.1).
///
/// One row per shot, voided and unassigned included, oldest first. The file is
/// written to the temporary folder and handed to the Android share menu.
library;

import 'dart:convert';
import 'dart:io';

import 'package:path/path.dart' as p;

import '../domain/csv.dart';
import 'app_database.dart';

class CsvExportService {
  CsvExportService(this.db);

  final AppDatabase db;

  /// Build the export rows. When [sessionId] is null, every shot is exported.
  Future<List<CsvShotRow>> buildRows({int? sessionId}) async {
    final database = await db.database;
    final where = sessionId == null ? '' : 'WHERE sh.session_id = ?';
    final args = sessionId == null ? <Object?>[] : [sessionId];

    final rows = await database.rawQuery('''
      SELECT
        s.id AS session_id,
        sh.shot_index,
        sh.mag_position,
        sh.freq_hz,
        sh.bc,
        sh.fired_at,
        sh.temp_c,
        sh.pressure_hpa,
        sh.humidity_pct,
        sh.weather_source,
        sh.cold_bore,
        sh.shots_since_clean,
        sh.time_since_last_shot_s,
        sh.voided,
        b.code AS box_code,
        c.slot AS slot_in_box,
        c.weight_g,
        c.rim_thickness_mm,
        c.oal_mm,
        l.lot_number
      FROM shots sh
      JOIN sessions s ON s.id = sh.session_id
      LEFT JOIN cartridges c ON c.id = sh.cartridge_id
      LEFT JOIN boxes b ON b.id = c.box_id
      LEFT JOIN lots l ON l.id = b.lot_id
      $where
      ORDER BY sh.fired_at ASC, sh.session_id ASC, sh.shot_index ASC
    ''', args);

    return rows.map((r) {
      final firedAt = _date(r['fired_at']);
      return CsvShotRow(
        sessionId: r['session_id'] as int,
        shotIndex: r['shot_index'] as int,
        magPosition: r['mag_position'] as int?,
        boxCode: r['box_code'] as String?,
        slotInBox: r['slot_in_box'] as int?,
        weightG: (r['weight_g'] as num?)?.toDouble(),
        rimThicknessMm: (r['rim_thickness_mm'] as num?)?.toDouble(),
        oalMm: (r['oal_mm'] as num?)?.toDouble(),
        lot: r['lot_number'] as String?,
        freqHz: r['freq_hz'] as int,
        bc: (r['bc'] as num).toDouble(),
        tempC: (r['temp_c'] as num?)?.toDouble(),
        pressureHpa: (r['pressure_hpa'] as num?)?.toDouble(),
        humidityPct: (r['humidity_pct'] as num?)?.toDouble(),
        coldBore: (r['cold_bore'] as int? ?? 0) != 0,
        shotsSinceClean: r['shots_since_clean'] as int? ?? 0,
        timeSinceLastShotS: (r['time_since_last_shot_s'] as num?)?.toDouble(),
        timestampMs: firedAt?.millisecondsSinceEpoch ?? 0,
        firedAtUtc: firedAt == null ? '' : formatFiredAtUtc(firedAt),
        firedAtLocal: firedAt == null ? '' : formatFiredAtLocal(firedAt),
        weatherSource: r['weather_source'] as String? ?? 'none',
        voided: (r['voided'] as int? ?? 0) != 0,
      );
    }).toList();
  }

  /// Write the CSV to [tempFolder] and return the file path.
  Future<String> writeToTemp({
    required String tempFolder,
    required DateTime now,
    int? sessionId,
  }) async {
    final rows = await buildRows(sessionId: sessionId);
    final name = 'borecore_shots_${_stamp(now)}.csv';
    final file = File(p.join(tempFolder, name));
    await file.writeAsString(buildCsv(rows), encoding: utf8);
    return file.path;
  }

  static String _stamp(DateTime time) {
    String two(int n) => n.toString().padLeft(2, '0');
    String three(int n) => n.toString().padLeft(3, '0');
    // Milliseconds keep two exports made in the same second from colliding.
    return '${time.year}${two(time.month)}${two(time.day)}'
        '_${two(time.hour)}${two(time.minute)}${two(time.second)}.'
        '${three(time.millisecond)}';
  }
}

DateTime? _date(Object? value) {
  if (value == null) return null;
  return DateTime.fromMillisecondsSinceEpoch(value as int, isUtc: true);
}
