/// Named constants for the whole app.
///
/// Every number that may need tuning (limits, timeouts, capacities, defaults)
/// lives here so it can be changed in one place.
library;

class AppConstants {
  // Magazine
  static const int magazineCapacity = 10;

  // Box grid defaults
  static const int defaultRows = 5;
  static const int defaultCols = 10;
  static const String defaultHingeEdge = 'top';

  // Measurement plausibility ranges (rule M3)
  static const double minWeightG = 2.50;
  static const double maxWeightG = 4.50;
  static const double minRimThicknessMm = 0.80;
  static const double maxRimThicknessMm = 1.30;

  // Box median warning (rule M3)
  static const int medianMinMeasured = 5;
  static const double medianWeightToleranceG = 0.20;
  static const double medianRimToleranceMm = 0.10;

  // Weather (rules W1, W2, W6)
  static const Duration weatherCurrentWindow = Duration(seconds: 30);
  static const Duration manualTempOfferDelay = Duration(seconds: 10);

  // Weather dongle protocol (section 10.3)
  static const int dongleCompanyId = 0xFFFF;
  static const int donglePayloadLength = 20;
  static const String dongleAdvName = 'WeatherDongle';
  static const double minTempC = -40;
  static const double maxTempC = 85;
  static const double minPressureHpa = 300;
  static const double maxPressureHpa = 1100;
  static const double minHumidityPct = 0;
  static const double maxHumidityPct = 100;

  // Radar protocol (section 10.2)
  static const String radarAdvName = 'vXfS3c6k';
  static const String defaultRadarRemoteId = 'D0:3A:C4:71:D4:AD';
  static const String radarServiceUuid = '00001623-88ec-688c-644b-3fa706c0bb76';
  static const String radarShotCharUuid = '00001624-88ec-688c-644b-3fa706c0bb76';
  static const double freqToMps = 160.647;

  // Scanner (section 10.1)
  static const Duration scanRestartInterval = Duration(minutes: 5);

  // Files and backup (section 12)
  static const String dbFileName = 'sentinel.db';
  static const String demoDbFileName = 'sentinel_demo.db';
  static const String simulatorFlagFileName = 'simulator.flag';
  static const String backupFolderName = 'backups';
  static const int maxBackups = 20;

  // Simulator (section 12.4)
  static const int simMinFreqHz = 50900;
  static const int simMaxFreqHz = 52400;
  static const double simMinBc = 0.120;
  static const double simMaxBc = 0.140;
  static const Duration simDongleInterval = Duration(seconds: 10);
  static const double demoMinWeightG = 3.30;
  static const double demoMaxWeightG = 3.40;
  static const double demoMinRimMm = 1.00;
  static const double demoMaxRimMm = 1.08;
  static const String demoProductName = 'Demo';
  static const String demoLotNumber = 'DEMO';
  static const String demoBoxCode = 'D01';

  // Settings keys (section 8)
  static const String keyRadarRemoteId = 'radar_remote_id';
  static const String keyDongleRemoteId = 'dongle_remote_id';
  static const String keyLastBoxId = 'last_box_id';
  static const String keyLastProductId = 'last_product_id';
  static const String keyLastLotId = 'last_lot_id';

  // Default rifle (section 8)
  static const String defaultRifleName = 'Tikka T1X';
}
