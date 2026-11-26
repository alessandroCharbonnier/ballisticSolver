/// Radar source interface (section 10).
///
/// A radar source delivers a stream of shots (frequency, BC) and a stream of
/// link states, and has start and stop. The Fire controller only knows this
/// interface; a Bluetooth version and a simulated version implement it.
library;

enum RadarLinkState { searching, connecting, connected, disconnected }

class RadarShot {
  final int freqHz;
  final double bc;
  final double velocityMps;

  const RadarShot({
    required this.freqHz,
    required this.bc,
    required this.velocityMps,
  });
}

abstract class RadarSource {
  Stream<RadarShot> get shots;
  Stream<RadarLinkState> get linkState;

  Future<void> start();
  Future<void> stop();
}
