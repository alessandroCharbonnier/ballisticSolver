/// Weather source interface (section 10).
///
/// A weather source delivers a stream of readings (temperature in C, pressure
/// in hPa, humidity in %) and has start and stop. The app never connects to
/// the dongle; it only receives its advertisements.
library;

import '../domain/models.dart';

abstract class WeatherSource {
  Stream<WeatherReading> get readings;

  Future<void> start();
  Future<void> stop();
}
