/// ChangeNotifier for the Data screen (export, backup, import, sessions).
library;

import 'package:flutter/foundation.dart';
import 'package:path_provider/path_provider.dart';

import '../app/app_services.dart';
import '../data/session_repository.dart';

class DataController extends ChangeNotifier {
  DataController(this.services);

  final AppServices services;

  List<SessionSummary> sessions = [];
  bool loading = true;

  Future<void> load() async {
    loading = true;
    notifyListeners();
    sessions = await services.sessions.listSessions();
    loading = false;
    notifyListeners();
  }

  Future<String> exportAllCsv() async {
    final temp = await getTemporaryDirectory();
    return services.csvExport.writeToTemp(
      tempFolder: temp.path,
      now: services.clock.now(),
    );
  }

  Future<String> exportSessionCsv(int sessionId) async {
    final temp = await getTemporaryDirectory();
    return services.csvExport.writeToTemp(
      tempFolder: temp.path,
      now: services.clock.now(),
      sessionId: sessionId,
    );
  }

  Future<String> exportBackup() async =>
      services.backup.makeBackup(now: services.clock.now());

  Future<String?> validateBackup(String path) =>
      services.backup.validateBackupFile(path);

  Future<void> importBackup(String path) async {
    await services.backup.importBackup(path);
    await load();
  }
}
