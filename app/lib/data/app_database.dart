/// SQLite database (section 8).
///
/// One file, schema version 1. Foreign keys are turned on when the database
/// opens. All times are integer milliseconds since 1970 UTC; booleans are 0 or
/// 1. Nothing is ever deleted by the app: boxes are hidden, shots are voided.
library;

import 'package:sqflite/sqflite.dart';

import '../domain/constants.dart';

class AppDatabase {
  AppDatabase(this.path);

  final String path;
  Database? _db;

  Future<Database> get database async => _db ??= await _open();

  Future<Database> _open() async {
    return openDatabase(
      path,
      version: 1,
      onConfigure: (db) => db.execute('PRAGMA foreign_keys = ON'),
      onCreate: _onCreate,
    );
  }

  Future<void> _onCreate(Database db, int version) async {
    await db.execute('''
      CREATE TABLE products (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL UNIQUE,
        rows INTEGER NOT NULL DEFAULT 5,
        cols INTEGER NOT NULL DEFAULT 10,
        hinge_edge TEXT NOT NULL DEFAULT 'top',
        created_at INTEGER
      )
    ''');

    await db.execute('''
      CREATE TABLE lots (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        product_id INTEGER NOT NULL REFERENCES products(id),
        lot_number TEXT NOT NULL,
        created_at INTEGER,
        UNIQUE (product_id, lot_number)
      )
    ''');

    await db.execute('''
      CREATE TABLE boxes (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        lot_id INTEGER NOT NULL REFERENCES lots(id),
        code TEXT NOT NULL UNIQUE,
        hidden INTEGER NOT NULL DEFAULT 0,
        created_at INTEGER
      )
    ''');

    await db.execute('''
      CREATE TABLE cartridges (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        box_id INTEGER NOT NULL REFERENCES boxes(id),
        slot INTEGER NOT NULL,
        weight_g REAL,
        rim_thickness_mm REAL,
        oal_mm REAL,
        measured_at INTEGER,
        state TEXT NOT NULL DEFAULT 'box',
        mag_position INTEGER,
        predicted_mps REAL,
        batch_id INTEGER,
        UNIQUE (box_id, slot)
      )
    ''');

    await db.execute('''
      CREATE TABLE rifles (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        created_at INTEGER
      )
    ''');

    await db.execute('''
      CREATE TABLE sessions (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        rifle_id INTEGER NOT NULL REFERENCES rifles(id),
        started_at INTEGER,
        last_active_at INTEGER,
        barrel_cleaned INTEGER NOT NULL DEFAULT 0,
        clean_base INTEGER NOT NULL DEFAULT 0,
        manual_temp_c REAL
      )
    ''');

    await db.execute('''
      CREATE TABLE shots (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        session_id INTEGER NOT NULL REFERENCES sessions(id),
        cartridge_id INTEGER REFERENCES cartridges(id),
        shot_index INTEGER NOT NULL,
        mag_position INTEGER,
        freq_hz INTEGER NOT NULL,
        bc REAL NOT NULL,
        fired_at INTEGER,
        temp_c REAL,
        pressure_hpa REAL,
        humidity_pct REAL,
        weather_source TEXT NOT NULL DEFAULT 'none',
        cold_bore INTEGER NOT NULL DEFAULT 0,
        shots_since_clean INTEGER NOT NULL DEFAULT 0,
        time_since_last_shot_s REAL,
        voided INTEGER NOT NULL DEFAULT 0,
        voided_at INTEGER
      )
    ''');

    await db.execute('''
      CREATE TABLE settings (
        key TEXT PRIMARY KEY,
        value TEXT
      )
    ''');

    // One rifle is inserted when the database is created.
    await db.insert('rifles', {
      'name': AppConstants.defaultRifleName,
      'created_at': DateTime.now().millisecondsSinceEpoch,
    });
  }

  Future<void> close() async {
    final db = _db;
    _db = null;
    await db?.close();
  }
}
