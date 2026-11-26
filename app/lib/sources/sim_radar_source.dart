/// Simulated radar source (section 12.4).
///
/// Used for the simulator and for developing the Fire screen without a radar.
library;

import 'dart:async';
import 'dart:math';

import '../domain/constants.dart';
import 'radar_source.dart';

class SimRadarSource implements RadarSource {
  SimRadarSource({Random? random}) : _random = random ?? Random();

  final Random _random;
  final _shots = StreamController<RadarShot>.broadcast();
  final _link = StreamController<RadarLinkState>.broadcast();

  @override
  Stream<RadarShot> get shots => _shots.stream;

  @override
  Stream<RadarLinkState> get linkState => _link.stream;

  @override
  Future<void> start() async {
    _link.add(RadarLinkState.connected);
  }

  @override
  Future<void> stop() async {
    _link.add(RadarLinkState.disconnected);
  }

  /// Send one simulated shot with a frequency between 50900 and 52400 and a
  /// BC between 0.120 and 0.140 (section 12.4).
  void simulateShot() {
    final freqHz = AppConstants.simMinFreqHz +
        _random.nextInt(AppConstants.simMaxFreqHz - AppConstants.simMinFreqHz + 1);
    final bc = AppConstants.simMinBc +
        _random.nextDouble() * (AppConstants.simMaxBc - AppConstants.simMinBc);
    _shots.add(RadarShot(
      freqHz: freqHz,
      bc: bc,
      velocityMps: freqHz / AppConstants.freqToMps,
    ));
  }
}
