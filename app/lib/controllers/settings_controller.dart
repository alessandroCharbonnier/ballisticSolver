/// ChangeNotifier for the Settings screen.
library;

import 'package:flutter/foundation.dart';
import 'package:package_info_plus/package_info_plus.dart';

import '../app/app_services.dart';
import '../domain/constants.dart';
import '../sources/bluetooth_weather_source.dart';

class SettingsController extends ChangeNotifier {
  SettingsController(this.services);

  final AppServices services;

  int rifleId = 0;
  String rifleName = '';
  String radarRemoteId = AppConstants.defaultRadarRemoteId;
  String? dongleRemoteId;
  String appVersion = '';
  bool loading = true;

  /// Whether the simulator is on (section 12.4).
  bool get simulatorOn => services.simulatorOn;

  Future<void> load() async {
    final rifle = await services.sessions.activeRifle();
    if (rifle != null) {
      rifleId = rifle.id!;
      rifleName = rifle.name;
    }
    radarRemoteId = await services.settings.get(AppConstants.keyRadarRemoteId) ??
        AppConstants.defaultRadarRemoteId;
    dongleRemoteId =
        await services.settings.get(AppConstants.keyDongleRemoteId);

    final info = await PackageInfo.fromPlatform();
    appVersion = '${info.version}+${info.buildNumber}';

    loading = false;
    notifyListeners();
  }

  Future<void> saveRifleName(String name) async {
    if (rifleId == 0) return;
    await services.sessions.updateRifleName(rifleId, name);
    rifleName = name;
    notifyListeners();
  }

  Future<void> saveRadarId(String id) async {
    await services.settings.set(AppConstants.keyRadarRemoteId, id);
    radarRemoteId = id;
    notifyListeners();
  }

  Future<void> forgetDongle() async {
    await services.settings.remove(AppConstants.keyDongleRemoteId);
    dongleRemoteId = null;
    notifyListeners();
    final weather = services.weatherSource;
    if (weather is BluetoothWeatherSource) {
      await weather.forget();
    }
  }

  /// Create or remove the simulator marker file. The caller then rebuilds
  /// [AppServices] and returns to Home.
  Future<void> setSimulator(bool on) => AppServices.setSimulatorFlag(on);
}
