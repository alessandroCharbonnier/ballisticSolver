/// FX True Ballistic radar client (BLE central, UART-bridge protocol).
///
/// The radar's BLE module (Adafruit Bluefruit LE / nRF51822) exposes a Nordic
/// UART service plus an FX data service. Shot velocity + BC arrive as 9-byte
/// notifications on characteristic `00001624`.
library;

import 'dart:async';
import 'dart:typed_data';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../models/shot.dart';

class Radar {
  /// Advertised local name of the FX True Ballistic radar.
  static const String name = 'vXfS3c6k';

  /// Known random-static MAC (stable across reboots).
  static const String mac = 'd0:3a:c4:71:d4:ad';

  /// FX data service (contains the shot characteristic).
  static const String serviceUuid = '00001623-88ec-688c-644b-3fa706c0bb76';

  /// Shot notify characteristic.
  static const String shotCharUuid = '00001624-88ec-688c-644b-3fa706c0bb76';

  /// Opaque ASCII serial (ignored for logging).
  static const String serialCharUuid = '0000162b-88ec-688c-644b-3fa706c0bb76';
}

class RadarClient {
  RadarClient(this.device);

  final BluetoothDevice device;

  BluetoothCharacteristic? _shotChar;
  StreamSubscription<List<int>>? _shotSub;

  final _shots = StreamController<ShotRecord>.broadcast();
  Stream<ShotRecord> get shots => _shots.stream;

  bool get isConnected => device.isConnected;

  Future<void> connectAndListen() async {
    await device.connect(
      license: License.nonprofit,
      autoConnect: false,
      mtu: 512,
    );
    await device.discoverServices();

    final service = device.servicesList.firstWhere(
      (s) => s.uuid == Guid(Radar.serviceUuid),
      orElse: () => throw StateError('FX data service not found — not a radar?'),
    );

    _shotChar = service.characteristics.firstWhere(
      (c) => c.uuid == Guid(Radar.shotCharUuid),
      orElse: () => throw StateError('Shot characteristic not found'),
    );

    await _shotChar!.setNotifyValue(true);
    _shotSub = _shotChar!.lastValueStream.listen(_onShot);
  }

  void _onShot(List<int> bytes) {
    final shot = decodeShot(bytes);
    if (shot != null) _shots.add(shot);
  }

  /// Decode a 9-byte shot notification; returns null for malformed frames.
  static ShotRecord? decodeShot(List<int> b) {
    if (b.length < 9 || b[0] != 0x0B || b[8] != 0x05) return null;

    final int freq = (b[2] << 8) | b[3];

    // BC: big-endian IEEE754 float32.
    final ByteData data = Uint8List.fromList(b.sublist(4, 8)).buffer.asByteData();
    final double bc = data.getFloat32(0, Endian.big);

    final double vmps = freq / ShotRecord.kFreqToMps;
    return ShotRecord(
      freqHz: freq,
      ballisticCoefficient: bc,
      velocityMps: vmps,
      velocityFps: vmps * ShotRecord.kMpsToFps,
      timestamp: DateTime.now(),
    );
  }

  Future<void> dispose() async {
    await _shotSub?.cancel();
    await _shots.close();
    if (device.isConnected) {
      await device.disconnect();
    }
  }
}
