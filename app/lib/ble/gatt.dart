/// GATT contract for the Ballistic Sentinel device (`src/ble_server.cpp`).
///
/// The device exposes one service with four characteristics; config and
/// telemetry are JSON documents. Config/command writes are gated by a 4-digit
/// app-layer PIN (shown on the device OLED).
library;

class Gatt {
  static const String service = '19b10000-e8f2-537e-4f6c-d104768a1214';
  static const String config = '19b10001-e8f2-537e-4f6c-d104768a1214';
  static const String telemetry = '19b10002-e8f2-537e-4f6c-d104768a1214';
  static const String command = '19b10003-e8f2-537e-4f6c-d104768a1214';
  static const String status = '19b10004-e8f2-537e-4f6c-d104768a1214';

  /// Advertised local name of the device.
  static const String deviceName = 'BallisticSentinel';
}

// ── Lenient JSON coercion helpers ───────────────────────────────────────────

double _d(Object? v, double dflt) => v is num ? v.toDouble() : dflt;
int _i(Object? v, int dflt) => v is num ? v.toInt() : dflt;
bool _b(Object? v, bool dflt) => v is bool ? v : dflt;
String _s(Object? v, String dflt) => v is String ? v : dflt;

// ── Models (mirror the device JSON) ─────────────────────────────────────────

class StageTarget {
  String name;
  int distance;

  StageTarget({this.name = '', this.distance = 0});

  factory StageTarget.fromJson(Map<String, dynamic> j) => StageTarget(
        name: _s(j['name'], ''),
        distance: _i(j['distance'], 0),
      );

  Map<String, dynamic> toJson() => {'name': name, 'distance': distance};
}

class UnitPrefs {
  int distance = 0; // 0=yd, 1=m
  int velocity = 0; // 0=fps, 1=m/s
  int weight = 0; // 0=gr, 1=g
  int length = 0; // 0=in, 1=mm
  int temperature = 0; // 0=F, 1=C
  int pressure = 0; // 0=inHg, 1=hPa

  UnitPrefs();

  factory UnitPrefs.fromJson(Map<String, dynamic>? j) {
    final u = UnitPrefs();
    if (j == null) return u;
    u.distance = _i(j['distance'], 0);
    u.velocity = _i(j['velocity'], 0);
    u.weight = _i(j['weight'], 0);
    u.length = _i(j['length'], 0);
    u.temperature = _i(j['temperature'], 0);
    u.pressure = _i(j['pressure'], 0);
    return u;
  }

  Map<String, dynamic> toJson() => {
        'distance': distance,
        'velocity': velocity,
        'weight': weight,
        'length': length,
        'temperature': temperature,
        'pressure': pressure,
      };
}

class ManualWeather {
  double tempF = 59.0;
  double pressureInhg = 29.92;
  double humidity = 0.0;

  ManualWeather();

  factory ManualWeather.fromJson(Map<String, dynamic>? j) {
    final m = ManualWeather();
    if (j == null) return m;
    m.tempF = _d(j['temp_f'], 59.0);
    m.pressureInhg = _d(j['pressure_inhg'], 29.92);
    m.humidity = _d(j['humidity'], 0.0);
    return m;
  }

  Map<String, dynamic> toJson() => {
        'temp_f': tempF,
        'pressure_inhg': pressureInhg,
        'humidity': humidity,
      };
}

class RifleConfig {
  // Drag
  double bc = 0.315;
  String dragTable = 'G7';
  bool multiBc = false;

  // Bullet
  double weight = 140.0;
  double caliber = 0.264;
  double length = 1.35;

  // Rifle
  double mv = 2710.0;
  double sh = 1.5;
  double twist = 8.0;
  double zero = 100.0;
  double lat = 0.0;

  // Correction preference
  String corrUnit = 'MOA';
  double clickSize = 0.25;
  int clickUnit = 0;

  // Cant calibration
  double cantOffset = 0.0;
  double cantSens = 1.0;

  // Powder sensitivity
  bool usePs = false;

  // BLE
  int blePin = 1234;
  String dongleMac = '';

  UnitPrefs units = UnitPrefs();
  ManualWeather manual = ManualWeather();
  List<StageTarget> stages = [];

  RifleConfig();

  factory RifleConfig.fromJson(Map<String, dynamic> doc) {
    final c = RifleConfig();
    final cfg = doc['config'];
    if (cfg is Map<String, dynamic>) {
      c.bc = _d(cfg['bc'], c.bc);
      c.dragTable = _s(cfg['drag_table'], c.dragTable);
      c.multiBc = _b(cfg['multi_bc'], c.multiBc);
      c.weight = _d(cfg['weight'], c.weight);
      c.caliber = _d(cfg['caliber'], c.caliber);
      c.length = _d(cfg['length'], c.length);
      c.mv = _d(cfg['mv'], c.mv);
      c.sh = _d(cfg['sh'], c.sh);
      c.twist = _d(cfg['twist'], c.twist);
      c.zero = _d(cfg['zero'], c.zero);
      c.lat = _d(cfg['lat'], c.lat);
      c.corrUnit = _s(cfg['corr_unit'], c.corrUnit);
      c.clickSize = _d(cfg['click_size'], c.clickSize);
      c.clickUnit = _i(cfg['click_unit'], c.clickUnit);
      c.cantOffset = _d(cfg['cant_offset'], c.cantOffset);
      c.cantSens = _d(cfg['cant_sens'], c.cantSens);
      c.usePs = _b(cfg['use_ps'], c.usePs);
      c.blePin = _i(cfg['ble_pin'], c.blePin);
      c.dongleMac = _s(cfg['dongle_mac'], '');
      c.units = UnitPrefs.fromJson(cfg['units'] as Map<String, dynamic>?);
      c.manual = ManualWeather.fromJson(cfg['manual'] as Map<String, dynamic>?);
    }
    final stages = doc['stages'];
    if (stages is List) {
      c.stages = stages
          .whereType<Map<String, dynamic>>()
          .map(StageTarget.fromJson)
          .toList();
    }
    return c;
  }

  Map<String, dynamic> toJson() => {
        'config': {
          'bc': bc,
          'drag_table': dragTable,
          'multi_bc': multiBc,
          'weight': weight,
          'caliber': caliber,
          'length': length,
          'mv': mv,
          'sh': sh,
          'twist': twist,
          'zero': zero,
          'lat': lat,
          'corr_unit': corrUnit,
          'click_size': clickSize,
          'click_unit': clickUnit,
          'cant_offset': cantOffset,
          'cant_sens': cantSens,
          'use_ps': usePs,
          'ble_pin': blePin,
          'dongle_mac': dongleMac,
          'units': units.toJson(),
          'manual': manual.toJson(),
        },
        'stages': stages.map((s) => s.toJson()).toList(),
      };
}

/// Live snapshot pushed by the device at the display refresh rate.
class Telemetry {
  double tempF = 59.0;
  double pressureInhg = 29.92;
  double humidity = 0.0;
  double headingDeg = 0.0;
  double cantDeg = 0.0;
  double batteryPct = -1.0;
  double batteryV = 0.0;
  bool weatherLive = false;
  double distance = 0.0;
  double vCorr = 0.0;
  double hCorr = 0.0;
  bool staged = false;

  Telemetry();

  factory Telemetry.fromJson(Map<String, dynamic> j) => Telemetry()
    ..tempF = _d(j['t_f'], 59.0)
    ..pressureInhg = _d(j['p_inhg'], 29.92)
    ..humidity = _d(j['rh_pct'], 0.0)
    ..headingDeg = _d(j['head_deg'], 0.0)
    ..cantDeg = _d(j['cant_deg'], 0.0)
    ..batteryPct = _d(j['batt_pct'], -1.0)
    ..batteryV = _d(j['batt_v'], 0.0)
    ..weatherLive = _b(j['wx_live'], false)
    ..distance = _d(j['distance'], 0.0)
    ..vCorr = _d(j['v_corr'], 0.0)
    ..hCorr = _d(j['h_corr'], 0.0)
    ..staged = _b(j['staged'], false);
}
