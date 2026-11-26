/// Weather tracker (rules W1, W2, W3).
///
/// Keeps only the latest reading. A reading is current for
/// [AppConstants.weatherCurrentWindow] after it was received.
library;

import 'constants.dart';
import 'models.dart';

/// The weather that applies to a shot being saved (rule W3).
class WeatherForShot {
  final String source; // dongle | manual | none
  final double? tempC;
  final double? pressureHpa;
  final double? humidityPct;

  const WeatherForShot({
    required this.source,
    this.tempC,
    this.pressureHpa,
    this.humidityPct,
  });
}

class WeatherTracker {
  WeatherReading? _latest;

  /// W1: only the latest reading is kept.
  void add(WeatherReading reading) {
    _latest = reading;
  }

  WeatherReading? get latest => _latest;

  /// W2: a reading is current for 30 seconds after it was received.
  bool isCurrent(DateTime now) {
    final reading = _latest;
    if (reading == null) return false;
    return now.difference(reading.receivedAt) <=
        AppConstants.weatherCurrentWindow;
  }

  /// W3: resolve the weather stored with a shot.
  WeatherForShot resolve(DateTime now, double? manualTempC) {
    final reading = _latest;
    if (reading != null &&
        now.difference(reading.receivedAt) <= AppConstants.weatherCurrentWindow) {
      return WeatherForShot(
        source: 'dongle',
        tempC: reading.tempC,
        pressureHpa: reading.pressureHpa,
        humidityPct: reading.humidityPct,
      );
    }
    if (manualTempC != null) {
      return WeatherForShot(source: 'manual', tempC: manualTempC);
    }
    return const WeatherForShot(source: 'none');
  }

  void clear() {
    _latest = null;
  }
}
