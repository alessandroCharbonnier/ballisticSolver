/// Domain models for the collect workflow (section 3 vocabulary).
///
/// Pure Dart: no Flutter or plugin imports. Times are integer milliseconds
/// since 1970 UTC, matching the database (section 8).
library;

import 'constants.dart';

DateTime? _date(Object? value) {
  if (value == null) return null;
  final ms = value as int;
  return DateTime.fromMillisecondsSinceEpoch(ms, isUtc: true);
}

/// An ammunition type (e.g. "SK Rifle Match"). Owns the box layout.
class Product {
  final int? id;
  final String name;
  final int rows;
  final int cols;
  final String hingeEdge;
  final DateTime? createdAt;

  const Product({
    this.id,
    required this.name,
    this.rows = AppConstants.defaultRows,
    this.cols = AppConstants.defaultCols,
    this.hingeEdge = AppConstants.defaultHingeEdge,
    this.createdAt,
  });

  factory Product.fromMap(Map<String, Object?> m) => Product(
        id: m['id'] as int?,
        name: m['name'] as String,
        rows: (m['rows'] as int?) ?? AppConstants.defaultRows,
        cols: (m['cols'] as int?) ?? AppConstants.defaultCols,
        hingeEdge: (m['hinge_edge'] as String?) ?? AppConstants.defaultHingeEdge,
        createdAt: _date(m['created_at']),
      );

  Map<String, Object?> toMap() => {
        if (id != null) 'id': id,
        'name': name,
        'rows': rows,
        'cols': cols,
        'hinge_edge': hingeEdge,
        'created_at': createdAt?.millisecondsSinceEpoch,
      };
}

/// A manufacturing lot number of one product.
class Lot {
  final int? id;
  final int? productId;
  final String lotNumber;
  final DateTime? createdAt;

  const Lot({
    this.id,
    this.productId,
    required this.lotNumber,
    this.createdAt,
  });

  factory Lot.fromMap(Map<String, Object?> m) => Lot(
        id: m['id'] as int?,
        productId: m['product_id'] as int?,
        lotNumber: m['lot_number'] as String,
        createdAt: _date(m['created_at']),
      );

  Map<String, Object?> toMap() => {
        if (id != null) 'id': id,
        'product_id': productId,
        'lot_number': lotNumber,
        'created_at': createdAt?.millisecondsSinceEpoch,
      };
}

/// One physical tray of cartridges belonging to one lot.
class Box {
  final int? id;
  final int? lotId;
  final String code;
  final bool hidden;
  final DateTime? createdAt;

  const Box({
    this.id,
    this.lotId,
    required this.code,
    this.hidden = false,
    this.createdAt,
  });

  factory Box.fromMap(Map<String, Object?> m) => Box(
        id: m['id'] as int?,
        lotId: m['lot_id'] as int?,
        code: m['code'] as String,
        hidden: (m['hidden'] as int? ?? 0) != 0,
        createdAt: _date(m['created_at']),
      );

  Map<String, Object?> toMap() => {
        if (id != null) 'id': id,
        'lot_id': lotId,
        'code': code,
        'hidden': hidden ? 1 : 0,
        'created_at': createdAt?.millisecondsSinceEpoch,
      };
}

/// The round in one slot of one box. Identified by (box, slot).
class Cartridge {
  final int? id;
  final int? boxId;
  final int slot;
  final double? weightG;
  final double? rimThicknessMm;
  final double? oalMm;
  final DateTime? measuredAt;
  final String state; // box | magazine | fired
  final int? magPosition;
  final double? predictedMps;
  final int? batchId;

  const Cartridge({
    this.id,
    this.boxId,
    required this.slot,
    this.weightG,
    this.rimThicknessMm,
    this.oalMm,
    this.measuredAt,
    this.state = 'box',
    this.magPosition,
    this.predictedMps,
    this.batchId,
  });

  /// Rule M1: a cartridge is measured when both values are present.
  bool get isMeasured => weightG != null && rimThicknessMm != null;

  factory Cartridge.fromMap(Map<String, Object?> m) => Cartridge(
        id: m['id'] as int?,
        boxId: m['box_id'] as int?,
        slot: m['slot'] as int,
        weightG: (m['weight_g'] as num?)?.toDouble(),
        rimThicknessMm: (m['rim_thickness_mm'] as num?)?.toDouble(),
        oalMm: (m['oal_mm'] as num?)?.toDouble(),
        measuredAt: _date(m['measured_at']),
        state: (m['state'] as String?) ?? 'box',
        magPosition: m['mag_position'] as int?,
        predictedMps: (m['predicted_mps'] as num?)?.toDouble(),
        batchId: m['batch_id'] as int?,
      );

  Map<String, Object?> toMap() => {
        if (id != null) 'id': id,
        'box_id': boxId,
        'slot': slot,
        'weight_g': weightG,
        'rim_thickness_mm': rimThicknessMm,
        'oal_mm': oalMm,
        'measured_at': measuredAt?.millisecondsSinceEpoch,
        'state': state,
        'mag_position': magPosition,
        'predicted_mps': predictedMps,
        'batch_id': batchId,
      };
}

/// One day at the range with one rifle.
class Session {
  final int? id;
  final int? rifleId;
  final DateTime? startedAt;
  final DateTime? lastActiveAt;
  final bool barrelCleaned;
  final int cleanBase;
  final double? manualTempC;

  const Session({
    this.id,
    this.rifleId,
    this.startedAt,
    this.lastActiveAt,
    this.barrelCleaned = false,
    this.cleanBase = 0,
    this.manualTempC,
  });

  factory Session.fromMap(Map<String, Object?> m) => Session(
        id: m['id'] as int?,
        rifleId: m['rifle_id'] as int?,
        startedAt: _date(m['started_at']),
        lastActiveAt: _date(m['last_active_at']),
        barrelCleaned: (m['barrel_cleaned'] as int? ?? 0) != 0,
        cleanBase: m['clean_base'] as int? ?? 0,
        manualTempC: (m['manual_temp_c'] as num?)?.toDouble(),
      );

  Map<String, Object?> toMap() => {
        if (id != null) 'id': id,
        'rifle_id': rifleId,
        'started_at': startedAt?.millisecondsSinceEpoch,
        'last_active_at': lastActiveAt?.millisecondsSinceEpoch,
        'barrel_cleaned': barrelCleaned ? 1 : 0,
        'clean_base': cleanBase,
        'manual_temp_c': manualTempC,
      };
}

/// One radar notification. Normally attached to a cartridge.
class Shot {
  final int? id;
  final int? sessionId;
  final int? cartridgeId;
  final int shotIndex;
  final int? magPosition;
  final int freqHz;
  final double bc;
  final DateTime? firedAt;
  final double? tempC;
  final double? pressureHpa;
  final double? humidityPct;
  final String weatherSource; // dongle | manual | none
  final bool coldBore;
  final int shotsSinceClean;
  final double? timeSinceLastShotS;
  final bool voided;
  final DateTime? voidedAt;

  const Shot({
    this.id,
    this.sessionId,
    this.cartridgeId,
    required this.shotIndex,
    this.magPosition,
    required this.freqHz,
    required this.bc,
    this.firedAt,
    this.tempC,
    this.pressureHpa,
    this.humidityPct,
    this.weatherSource = 'none',
    this.coldBore = false,
    this.shotsSinceClean = 0,
    this.timeSinceLastShotS,
    this.voided = false,
    this.voidedAt,
  });

  /// Velocity in m/s, computed for display only. The raw frequency is stored.
  double get velocityMps => freqHz / AppConstants.freqToMps;

  factory Shot.fromMap(Map<String, Object?> m) => Shot(
        id: m['id'] as int?,
        sessionId: m['session_id'] as int?,
        cartridgeId: m['cartridge_id'] as int?,
        shotIndex: m['shot_index'] as int,
        magPosition: m['mag_position'] as int?,
        freqHz: m['freq_hz'] as int,
        bc: (m['bc'] as num?)?.toDouble() ?? 0,
        firedAt: _date(m['fired_at']),
        tempC: (m['temp_c'] as num?)?.toDouble(),
        pressureHpa: (m['pressure_hpa'] as num?)?.toDouble(),
        humidityPct: (m['humidity_pct'] as num?)?.toDouble(),
        weatherSource: (m['weather_source'] as String?) ?? 'none',
        coldBore: (m['cold_bore'] as int? ?? 0) != 0,
        shotsSinceClean: m['shots_since_clean'] as int? ?? 0,
        timeSinceLastShotS: (m['time_since_last_shot_s'] as num?)?.toDouble(),
        voided: (m['voided'] as int? ?? 0) != 0,
        voidedAt: _date(m['voided_at']),
      );

  Map<String, Object?> toMap() => {
        if (id != null) 'id': id,
        'session_id': sessionId,
        'cartridge_id': cartridgeId,
        'shot_index': shotIndex,
        'mag_position': magPosition,
        'freq_hz': freqHz,
        'bc': bc,
        'fired_at': firedAt?.millisecondsSinceEpoch,
        'temp_c': tempC,
        'pressure_hpa': pressureHpa,
        'humidity_pct': humidityPct,
        'weather_source': weatherSource,
        'cold_bore': coldBore ? 1 : 0,
        'shots_since_clean': shotsSinceClean,
        'time_since_last_shot_s': timeSinceLastShotS,
        'voided': voided ? 1 : 0,
        'voided_at': voidedAt?.millisecondsSinceEpoch,
      };
}

/// One weather measurement received from the dongle.
class WeatherReading {
  final double tempC;
  final double pressureHpa;
  final double humidityPct;
  final DateTime receivedAt;

  const WeatherReading({
    required this.tempC,
    required this.pressureHpa,
    required this.humidityPct,
    required this.receivedAt,
  });
}

/// A rifle. One row ("Tikka T1X") is inserted when the database is created.
class Rifle {
  final int? id;
  final String name;
  final DateTime? createdAt;

  const Rifle({this.id, required this.name, this.createdAt});

  factory Rifle.fromMap(Map<String, Object?> m) => Rifle(
        id: m['id'] as int?,
        name: m['name'] as String,
        createdAt: _date(m['created_at']),
      );

  Map<String, Object?> toMap() => {
        if (id != null) 'id': id,
        'name': name,
        'created_at': createdAt?.millisecondsSinceEpoch,
      };
}
