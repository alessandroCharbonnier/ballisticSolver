/// Simulated weather source (section 12.4).
///
/// While on, sends a reading immediately and then every 10 seconds. The
/// "Dongle on / off" switch toggles it.
library;

import 'dart:async';
import 'dart:math';

import '../domain/constants.dart';
import '../domain/models.dart';
import 'weather_source.dart';

class SimWeatherSource implements WeatherSource {
  SimWeatherSource({Random? random}) : _random = random ?? Random();

  final Random _random;
  final _readings = StreamController<WeatherReading>.broadcast();
  Timer? _timer;
  bool _on = false;

  @override
  Stream<WeatherReading> get readings => _readings.stream;

  @override
  Future<void> start() async {
    _on = true;
    _emit();
    _timer = Timer.periodic(AppConstants.simDongleInterval, (_) => _emit());
  }

  @override
  Future<void> stop() async {
    _on = false;
    _timer?.cancel();
    _timer = null;
  }

  void _emit() {
    if (!_on) return;
    _readings.add(WeatherReading(
      tempC: 20.0 + _random.nextDouble() * 3.0,
      pressureHpa: 1010.0 + _random.nextDouble() * 10.0,
      humidityPct: 40.0 + _random.nextDouble() * 20.0,
      receivedAt: DateTime.now(),
    ));
  }
}
