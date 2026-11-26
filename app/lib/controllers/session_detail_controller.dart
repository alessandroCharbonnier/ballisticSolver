/// ChangeNotifier for the session detail screen (section 11.5).
library;

import 'package:flutter/foundation.dart';
import 'package:path_provider/path_provider.dart';

import '../app/app_services.dart';
import '../data/shot_repository.dart';
import '../domain/models.dart';

class SessionDetailController extends ChangeNotifier {
  SessionDetailController(this.services, this.sessionId);

  final AppServices services;
  final int sessionId;

  Session? session;
  List<ShotDetail> shots = [];
  bool loading = true;

  Future<void> load() async {
    loading = true;
    notifyListeners();
    session = await services.sessions.getSession(sessionId);
    shots = await services.shots.shotDetails(sessionId);
    loading = false;
    notifyListeners();
  }

  Future<String> exportCsv() async {
    final temp = await getTemporaryDirectory();
    return services.csvExport.writeToTemp(
      tempFolder: temp.path,
      now: services.clock.now(),
      sessionId: sessionId,
    );
  }

  /// W7: set weather for shots whose source is `none`. Returns the number of
  /// updated shots.
  Future<int> setWeatherForShotsWithoutData({
    required double tempC,
    double? pressureHpa,
    double? humidityPct,
  }) async {
    final updated = await services.shots.setWeatherForShotsWithoutData(
      sessionId: sessionId,
      tempC: tempC,
      pressureHpa: pressureHpa,
      humidityPct: humidityPct,
    );
    await load();
    return updated;
  }
}
