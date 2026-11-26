/// The single Bluetooth scanner (section 10.1).
///
/// One scan at a time, no time limit, with repeated advertisements enabled
/// (`continuousUpdates`). The scan is restarted every five minutes because
/// Android can quietly slow down a long-running scan. It stops when the Fire
/// screen (or the Settings Bluetooth check) closes.
library;

import 'dart:async';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../domain/constants.dart';

class BluetoothScanner {
  BluetoothScanner({required this.onResults});

  /// Called with every batch of scan results.
  final void Function(List<ScanResult> results) onResults;

  StreamSubscription<List<ScanResult>>? _subscription;
  Timer? _restartTimer;

  Future<void> start() async {
    if (_subscription != null) return;

    _subscription = FlutterBluePlus.scanResults.listen(onResults);
    await _ensureAdapterOn();
    await _restartScan();

    _restartTimer = Timer.periodic(
      AppConstants.scanRestartInterval,
      (_) => _restartScan(),
    );
  }

  Future<void> _ensureAdapterOn() async {
    try {
      if (FlutterBluePlus.adapterStateNow != BluetoothAdapterState.on) {
        await FlutterBluePlus.turnOn();
      }
    } catch (_) {
      // Scanning may still work if the adapter is already on.
    }
  }

  Future<void> _restartScan() async {
    try {
      if (FlutterBluePlus.isScanningNow) {
        await FlutterBluePlus.stopScan();
      }
      await FlutterBluePlus.startScan(continuousUpdates: true);
    } catch (_) {
      // A transient error; the periodic timer retries.
    }
  }

  /// Pause scanning (used while the radar source connects).
  Future<void> pause() async {
    try {
      if (FlutterBluePlus.isScanningNow) {
        await FlutterBluePlus.stopScan();
      }
    } catch (_) {}
  }

  /// Resume scanning after a pause.
  Future<void> resume() async {
    await _restartScan();
  }

  Future<void> stop() async {
    _restartTimer?.cancel();
    _restartTimer = null;
    await _subscription?.cancel();
    _subscription = null;
    try {
      if (FlutterBluePlus.isScanningNow) {
        await FlutterBluePlus.stopScan();
      }
    } catch (_) {}
  }
}
