/// Backup and import (sections 12.2 and 12.3).
///
/// A backup is a copy of the database file named `sentinel_backup_<time>.db`
/// in a `backups` folder inside the app's documents folder. Automatic backups
/// stay on the phone; only an exported backup protects against a lost phone.
library;

import 'dart:io';

import 'package:path/path.dart' as p;
import 'package:sqflite/sqflite.dart';

import '../domain/constants.dart';
import 'app_database.dart';

class BackupService {
  BackupService({
    required this.db,
    required this.dbPath,
    required this.documentsPath,
  });

  final AppDatabase db;
  final String dbPath;
  final String documentsPath;

  String get folderPath =>
      p.join(documentsPath, AppConstants.backupFolderName);

  /// Make a backup: close the database, copy the file, open it again.
  /// Returns the backup file path.
  Future<String> makeBackup({DateTime? now}) async {
    final time = now ?? DateTime.now();
    await db.close();
    try {
      await Directory(folderPath).create(recursive: true);
      final name = 'sentinel_backup_${_stamp(time)}.db';
      final destination = p.join(folderPath, name);
      await File(dbPath).copy(destination);
      return destination;
    } finally {
      await db.database; // reopen
    }
  }

  /// Keep the 20 newest backups and remove older ones.
  Future<void> pruneOld() async {
    final folder = Directory(folderPath);
    if (!await folder.exists()) return;
    final files = await folder
        .list()
        .where((e) => e is File && e.path.endsWith('.db'))
        .toList();
    files.sort((a, b) =>
        b.statSync().modified.compareTo(a.statSync().modified));
    for (final file in files.skip(AppConstants.maxBackups)) {
      try {
        await file.delete();
      } catch (_) {
        // A backup that cannot be removed is left in place.
      }
    }
  }

  /// Section 12.2: automatic backup when the Fire screen is left, if the
  /// session received at least one shot since the previous backup.
  Future<void> autoBackupIfNeeded({required int sessionId}) async {
    final database = await db.database;
    final rows = await database.rawQuery(
      'SELECT MAX(fired_at) AS latest FROM shots WHERE session_id = ?',
      [sessionId],
    );
    final latestMs = (rows.first['latest'] as num?)?.toInt();
    if (latestMs == null) return; // No shot this session.

    final lastBackup = await newestBackupTime();
    final lastBackupMs = lastBackup?.millisecondsSinceEpoch ?? 0;
    if (latestMs > lastBackupMs) {
      await makeBackup();
      await pruneOld();
    }
  }

  Future<DateTime?> newestBackupTime() async {
    final folder = Directory(folderPath);
    if (!await folder.exists()) return null;
    DateTime? newest;
    await for (final entity in folder.list()) {
      if (entity is! File || !entity.path.endsWith('.db')) continue;
      final modified = entity.statSync().modified;
      if (newest == null || modified.isAfter(newest)) newest = modified;
    }
    return newest;
  }

  /// Section 12.3 step 3: check that the file opens as a SQLite database with
  /// schema version 1 and a `shots` table. Returns null when valid, or an
  /// error message.
  Future<String?> validateBackupFile(String path) async {
    Database? temp;
    try {
      temp = await openDatabase(path, readOnly: true);
      final version = Sqflite.firstIntValue(
            await temp.rawQuery('PRAGMA user_version'),
          ) ??
          -1;
      final shotsTable = await temp.rawQuery(
        "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'shots'",
      );
      if (version != 1 || shotsTable.isEmpty) {
        return 'This file is not a Ballistic Sentinel backup';
      }
      return null;
    } catch (_) {
      return 'This file is not a Ballistic Sentinel backup';
    } finally {
      await temp?.close();
    }
  }

  /// Section 12.3 steps 4-5: back up the current data, close the database,
  /// replace the file with the picked one, and open it again.
  Future<void> importBackup(String path) async {
    await makeBackup();

    await db.close();
    try {
      // Remove any SQLite sidecar files from the previous database.
      final sidecars = ['$dbPath-wal', '$dbPath-shm'];
      for (final sidecar in sidecars) {
        final file = File(sidecar);
        if (await file.exists()) {
          await file.delete();
        }
      }

      final target = File(dbPath);
      if (await target.exists()) {
        await target.delete();
      }
      await File(path).copy(dbPath);
    } finally {
      await db.database; // reopen
    }
  }

  static String _stamp(DateTime time) {
    String two(int n) => n.toString().padLeft(2, '0');
    String three(int n) => n.toString().padLeft(3, '0');
    // Milliseconds keep two backups made in the same second from colliding.
    return '${time.year}${two(time.month)}${two(time.day)}'
        '_${two(time.hour)}${two(time.minute)}${two(time.second)}.'
        '${three(time.millisecond)}';
  }
}
