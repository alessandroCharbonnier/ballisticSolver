/// The single place where the app's services are created and owned.
///
/// [AppServices] is created once in `main` before the first screen (and again
/// when the simulator is toggled). It owns the database, the repositories, the
/// clock and the radar and weather sources. [AppScope] makes it available to
/// every screen.
library;

import 'dart:io';

import 'package:path/path.dart' as p;
import 'package:path_provider/path_provider.dart';

import '../data/app_database.dart';
import '../data/backup_service.dart';
import '../data/boxes_repository.dart';
import '../data/csv_export_service.dart';
import '../data/demo_data.dart';
import '../data/magazine_repository.dart';
import '../data/session_repository.dart';
import '../data/settings_repository.dart';
import '../data/shot_recorder.dart';
import '../data/shot_repository.dart';
import '../domain/clock.dart';
import '../domain/constants.dart';
import '../sources/bluetooth_radar_source.dart';
import '../sources/bluetooth_weather_source.dart';
import '../sources/radar_source.dart';
import '../sources/scanner.dart';
import '../sources/sim_radar_source.dart';
import '../sources/sim_weather_source.dart';
import '../sources/weather_source.dart';

class AppServices {
  AppServices._({
    required this.db,
    required this.clock,
    required this.settings,
    required this.boxes,
    required this.magazine,
    required this.sessions,
    required this.shots,
    required this.recorder,
    required this.scanner,
    required this.radarSource,
    required this.weatherSource,
    required this.backup,
    required this.csvExport,
    required this.dbPath,
    required this.simulatorOn,
    required this.documentsPath,
  });

  final AppDatabase db;
  final Clock clock;
  final SettingsRepository settings;
  final BoxesRepository boxes;
  final MagazineRepository magazine;
  final SessionRepository sessions;
  final ShotRepository shots;
  final ShotRecorder recorder;

  /// The scanner is null in simulator mode (there is nothing to scan).
  final BluetoothScanner? scanner;
  final RadarSource radarSource;
  final WeatherSource weatherSource;
  final BackupService backup;
  final CsvExportService csvExport;

  /// The path of the open database file (used by the backup service).
  final String dbPath;

  /// Whether the simulator is on (section 12.4).
  final bool simulatorOn;

  /// The app's documents folder.
  final String documentsPath;

  /// The path of the simulator marker file.
  static Future<String> simulatorFlagPath() async {
    final documents = await getApplicationDocumentsDirectory();
    return p.join(documents.path, AppConstants.simulatorFlagFileName);
  }

  /// Create or remove the simulator marker file.
  static Future<void> setSimulatorFlag(bool on) async {
    final flagPath = await simulatorFlagPath();
    final file = File(flagPath);
    if (on) {
      file.createSync(recursive: true);
    } else if (file.existsSync()) {
      file.deleteSync();
    }
  }

  static Future<AppServices> create({Clock? clock}) async {
    final resolvedClock = clock ?? const SystemClock();
    final documents = await getApplicationDocumentsDirectory();
    final documentsPath = documents.path;

    final flagPath = p.join(documentsPath, AppConstants.simulatorFlagFileName);
    final simulatorOn = File(flagPath).existsSync();

    final dbFileName =
        simulatorOn ? AppConstants.demoDbFileName : AppConstants.dbFileName;
    final dbPath = p.join(documentsPath, dbFileName);

    final db = AppDatabase(dbPath);
    await db.database;
    if (simulatorOn) {
      await seedDemoDatabase(db);
    }

    final settings = SettingsRepository(db);
    final boxes = BoxesRepository(db);
    final magazine = MagazineRepository(db);
    final sessions = SessionRepository(db);
    final shots = ShotRepository(db);
    final recorder = ShotRecorder(db: db, sessions: sessions, shots: shots);
    final backup = BackupService(
      db: db,
      dbPath: dbPath,
      documentsPath: documentsPath,
    );
    final csvExport = CsvExportService(db);

    late final RadarSource radarSource;
    late final WeatherSource weatherSource;
    BluetoothScanner? scanner;

    if (simulatorOn) {
      radarSource = SimRadarSource();
      weatherSource = SimWeatherSource();
    } else {
      late final BluetoothRadarSource bluetoothRadar;
      late final BluetoothWeatherSource bluetoothWeather;
      final bluetoothScanner = BluetoothScanner(onResults: (results) {
        bluetoothRadar.onScanResults(results);
        bluetoothWeather.onScanResults(results);
      });
      bluetoothRadar = BluetoothRadarSource(
        settings: settings,
        pauseScan: bluetoothScanner.pause,
        resumeScan: bluetoothScanner.resume,
      );
      bluetoothWeather = BluetoothWeatherSource(
        settings: settings,
        clock: resolvedClock,
      );
      radarSource = bluetoothRadar;
      weatherSource = bluetoothWeather;
      scanner = bluetoothScanner;
    }

    return AppServices._(
      db: db,
      clock: resolvedClock,
      settings: settings,
      boxes: boxes,
      magazine: magazine,
      sessions: sessions,
      shots: shots,
      recorder: recorder,
      scanner: scanner,
      radarSource: radarSource,
      weatherSource: weatherSource,
      backup: backup,
      csvExport: csvExport,
      dbPath: dbPath,
      simulatorOn: simulatorOn,
      documentsPath: documentsPath,
    );
  }

  /// Start the radar and weather sources, and the scanner when in Bluetooth
  /// mode (section 10.1: the scanner stops when the screen closes).
  Future<void> startSources() async {
    await radarSource.start();
    await weatherSource.start();
    await scanner?.start();
  }

  Future<void> stopSources() async {
    await scanner?.stop();
    await radarSource.stop();
    await weatherSource.stop();
  }

  Future<void> dispose() async {
    await scanner?.stop();
    await radarSource.stop();
    await weatherSource.stop();
    await db.close();
  }
}
