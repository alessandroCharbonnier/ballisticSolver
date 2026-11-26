/// Bluetooth radar source (section 10.2).
///
/// Recognises the FX True Ballistic radar by its remote id (setting
/// `radar_remote_id`, compared without regard to case) or by the advertised
/// name `vXfS3c6k`. Connects to the FX data service, subscribes to the shot
/// characteristic on `onValueReceived` (never `lastValueStream`, which replays
/// the previous value after a reconnect), and reconnects by itself when the
/// link drops.
library;

import 'dart:async';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../data/settings_repository.dart';
import '../domain/constants.dart';
import '../domain/radar_packet.dart';
import 'radar_source.dart';

class BluetoothRadarSource implements RadarSource {
  BluetoothRadarSource({
    required this.settings,
    required this.pauseScan,
    required this.resumeScan,
  });

  final SettingsRepository settings;
  final Future<void> Function() pauseScan;
  final Future<void> Function() resumeScan;

  final _shots = StreamController<RadarShot>.broadcast();
  final _link = StreamController<RadarLinkState>.broadcast();

  RadarLinkState _state = RadarLinkState.searching;
  String _remoteId = AppConstants.defaultRadarRemoteId;
  BluetoothDevice? _device;
  BluetoothCharacteristic? _shotChar;
  StreamSubscription<List<int>>? _charSub;
  StreamSubscription<BluetoothConnectionState>? _connSub;
  bool _connecting = false;

  @override
  Stream<RadarShot> get shots => _shots.stream;

  @override
  Stream<RadarLinkState> get linkState => _link.stream;

  RadarLinkState get state => _state;

  @override
  Future<void> start() async {
    final configured = await settings.get(AppConstants.keyRadarRemoteId);
    if (configured != null && configured.isNotEmpty) {
      _remoteId = configured;
    }
    _setState(RadarLinkState.searching);
  }

  @override
  Future<void> stop() async {
    await _charSub?.cancel();
    _charSub = null;
    await _connSub?.cancel();
    _connSub = null;
    try {
      await _device?.disconnect();
    } catch (_) {}
    _device = null;
    _shotChar = null;
    _setState(RadarLinkState.searching);
  }

  /// Called by the scanner with every batch of scan results.
  void onScanResults(List<ScanResult> results) {
    if (_connecting) return;
    if (_state != RadarLinkState.searching) return;

    final found = _findRadar(results);
    if (found != null) {
      _connect(found);
    }
  }

  ScanResult? _findRadar(List<ScanResult> results) {
    for (final result in results) {
      final name = result.advertisementData.advName;
      final id = result.device.remoteId.str;
      if (name == AppConstants.radarAdvName ||
          id.toLowerCase() == _remoteId.toLowerCase()) {
        return result;
      }
    }
    return null;
  }

  Future<void> _connect(ScanResult result) async {
    _connecting = true;
    final device = result.device;
    _setState(RadarLinkState.connecting);

    try {
      await pauseScan();

      await device.connect(
        license: License.nonprofit,
        autoConnect: false,
        mtu: 512,
      );
      await device.discoverServices();

      final service = device.servicesList.firstWhere(
        (s) => s.uuid == Guid(AppConstants.radarServiceUuid),
        orElse: () =>
            throw StateError('FX data service not found - not a radar?'),
      );
      _shotChar = service.characteristics.firstWhere(
        (c) => c.uuid == Guid(AppConstants.radarShotCharUuid),
        orElse: () => throw StateError('Shot characteristic not found'),
      );

      await _shotChar!.setNotifyValue(true);
      // onValueReceived never replays the previous value on a new listen.
      _charSub = _shotChar!.onValueReceived.listen(_onValue);

      _connSub = device.connectionState.listen((connectionState) {
        if (connectionState == BluetoothConnectionState.disconnected) {
          _onLinkDropped();
        }
      });

      _device = device;
      _setState(RadarLinkState.connected);
    } catch (_) {
      try {
        await device.disconnect();
      } catch (_) {}
      _shotChar = null;
      _charSub?.cancel();
      _charSub = null;
      _setState(RadarLinkState.disconnected);
      _setState(RadarLinkState.searching);
    } finally {
      _connecting = false;
      await resumeScan();
    }
  }

  void _onValue(List<int> bytes) {
    final packet = decodeRadarPacket(bytes);
    if (packet == null) return;
    _shots.add(RadarShot(
      freqHz: packet.freqHz,
      bc: packet.bc,
      velocityMps: packet.velocityMps,
    ));
  }

  void _onLinkDropped() {
    _charSub?.cancel();
    _charSub = null;
    _connSub?.cancel();
    _connSub = null;
    _shotChar = null;
    _device = null;
    _setState(RadarLinkState.disconnected);
    _setState(RadarLinkState.searching);
  }

  void _setState(RadarLinkState state) {
    _state = state;
    if (!_link.isClosed) {
      _link.add(state);
    }
  }
}
