/// Thin wrapper around a connected BoreCore One device.
///
/// Handles connection + service discovery, config read/write, command writes,
/// and the telemetry/status notification streams.
library;

import 'dart:async';
import 'dart:convert';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import 'gatt.dart';

class BoreCoreDevice {
  BoreCoreDevice(this.device);

  final BluetoothDevice device;

  BluetoothCharacteristic? _config;
  BluetoothCharacteristic? _telemetry;
  BluetoothCharacteristic? _command;
  BluetoothCharacteristic? _status;

  StreamSubscription<List<int>>? _telemetrySub;
  StreamSubscription<List<int>>? _statusSub;

  /// Live telemetry (temperature, pressure, cant, correction, …).
  final _telemetryCtrl = StreamController<Telemetry>.broadcast();
  Stream<Telemetry> get telemetry => _telemetryCtrl.stream;

  /// Status acks from the device ("authed", "bad pin", "saved", …).
  final _statusCtrl = StreamController<String>.broadcast();
  Stream<String> get status => _statusCtrl.stream;

  bool get isConnected => device.isConnected;

  Future<void> connectAndDiscover() async {
    // `mtu` + `autoConnect` are mutually exclusive; use an explicit MTU so
    // long JSON config payloads fit in fewer ATT writes.
    await device.connect(
      license: License.nonprofit,
      autoConnect: false,
      mtu: 512,
    );
    await device.discoverServices();

    final service = device.servicesList.firstWhere(
      (s) => s.uuid == Guid(Gatt.service),
      orElse: () => throw StateError(
          'Service ${Gatt.service} not found — is this the BoreCore One?'),
    );

    _config = _char(service, Gatt.config);
    _telemetry = _char(service, Gatt.telemetry);
    _command = _char(service, Gatt.command);
    _status = _char(service, Gatt.status);

    await _telemetry?.setNotifyValue(true);
    _telemetrySub = _telemetry?.lastValueStream.listen(_onTelemetry);

    await _status?.setNotifyValue(true);
    _statusSub = _status?.lastValueStream.listen(_onStatus);
  }

  BluetoothCharacteristic _char(BluetoothService s, String uuid) =>
      s.characteristics.firstWhere(
        (c) => c.uuid == Guid(uuid),
        orElse: () =>
            throw StateError('Characteristic $uuid not found on device'),
      );

  void _onTelemetry(List<int> bytes) {
    try {
      final map = jsonDecode(utf8.decode(bytes)) as Map<String, dynamic>;
      _telemetryCtrl.add(Telemetry.fromJson(map));
    } catch (_) {
      // Ignore partial/oversized notification frames.
    }
  }

  void _onStatus(List<int> bytes) {
    _statusCtrl.add(utf8.decode(bytes));
  }

  Future<RifleConfig> readConfig() async {
    final c = _config;
    if (c == null) throw StateError('Not connected');
    final bytes = await c.read();
    final doc = jsonDecode(utf8.decode(bytes)) as Map<String, dynamic>;
    return RifleConfig.fromJson(doc);
  }

  Future<void> writeConfig(RifleConfig cfg) async {
    final c = _config;
    if (c == null) throw StateError('Not connected');
    await c.write(utf8.encode(jsonEncode(cfg.toJson())), withoutResponse: false);
  }

  Future<void> auth(int pin) async {
    await _command?.write(
      utf8.encode(jsonEncode({'cmd': 'auth', 'pin': pin})),
    );
  }

  Future<void> startCantCalibration() async {
    await _command?.write(utf8.encode(jsonEncode({'cmd': 'cant_cal'})));
  }

  Future<void> dispose() async {
    await _telemetrySub?.cancel();
    await _statusSub?.cancel();
    await _telemetryCtrl.close();
    await _statusCtrl.close();
    if (device.isConnected) {
      await device.disconnect();
    }
  }
}
