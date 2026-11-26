/// Key-value settings (section 8).
library;

import 'package:sqflite/sqflite.dart';

import 'app_database.dart';

class SettingsRepository {
  SettingsRepository(this.db);

  final AppDatabase db;

  Future<String?> get(String key) async {
    final database = await db.database;
    final rows = await database.query(
      'settings',
      columns: ['value'],
      where: 'key = ?',
      whereArgs: [key],
      limit: 1,
    );
    if (rows.isEmpty) return null;
    return rows.first['value'] as String?;
  }

  Future<int?> getInt(String key) async {
    final value = await get(key);
    return value == null ? null : int.tryParse(value);
  }

  Future<void> set(String key, String value) async {
    final database = await db.database;
    await database.insert(
      'settings',
      {'key': key, 'value': value},
      conflictAlgorithm: ConflictAlgorithm.replace,
    );
  }

  Future<void> remove(String key) async {
    final database = await db.database;
    await database.delete('settings', where: 'key = ?', whereArgs: [key]);
  }
}
