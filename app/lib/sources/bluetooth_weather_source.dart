/// Bluetooth weather source (section 10.3, rule W5).
///
/// The dongle advertises manufacturer data with company id 0xFFFF and the name
/// `WeatherDongle`. The app only listens; it never connects. The first dongle
/// ever seen is remembered (`dongle_remote_id`); beacons from any other device
/// are ignored until the shooter chooses "Forget dongle" in Settings.
library;

import 'dart:async';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../data/settings_repository.dart';
import '../domain/clock.dart';
import '../domain/constants.dart';
import '../domain/dongle_packet.dart';
import '../domain/models.dart';
import 'weather_source.dart';

class BluetoothWeatherSource implements WeatherSource {
  BluetoothWeatherSource({
    required this.settings,
    required this.clock,
  });

  final SettingsRepository settings;
  final Clock clock;

  final _readings = StreamController<WeatherReading>.broadcast();
  String? _dongleId;

  @override
  Stream<WeatherReading> get readings => _readings.stream;

  @override
  Future<void> start() async {
    // W5: load the remembered dongle id (empty until one is first seen).
    _dongleId = await settings.get(AppConstants.keyDongleRemoteId);
  }

  @override
  Future<void> stop() async {
    // Nothing to disconnect: the dongle only broadcasts.
  }

  /// Called by the scanner with every batch of scan results.
  void onScanResults(List<ScanResult> results) {
    for (final result in results) {
      final advertisement = result.advertisementData;
      if (advertisement.advName != AppConstants.dongleAdvName) continue;

      final id = result.device.remoteId.str;
      if (_dongleId == null) {
        // W5: remember the first dongle ever seen.
        _dongleId = id;
        settings.set(AppConstants.keyDongleRemoteId, id);
      } else if (id != _dongleId) {
        continue; // W5: ignore any other device.
      }

      final bytes =
          advertisement.manufacturerData[AppConstants.dongleCompanyId];
      if (bytes == null) continue;

      final payload = decodeDonglePayload(bytes);
      if (payload == null) continue;

      _readings.add(WeatherReading(
        tempC: payload.tempC,
        pressureHpa: payload.pressureHpa,
        humidityPct: payload.humidityPct,
        receivedAt: clock.now(),
      ));
    }
  }

  /// Settings "Forget dongle": clear the remembered id.
  Future<void> forget() async {
    _dongleId = null;
    await settings.remove(AppConstants.keyDongleRemoteId);
  }
}
