/// A single decoded FX True Ballistic radar shot.
///
/// Packet layout (9 bytes, see re/RADAR_PROTOCOL.md):
///   [0x0B][0x00][freq u16 BE][BC float32 BE][0x05]
class ShotRecord {
  final int freqHz;
  final double ballisticCoefficient;
  final double velocityMps;
  final double velocityFps;
  final DateTime timestamp;

  ShotRecord({
    required this.freqHz,
    required this.ballisticCoefficient,
    required this.velocityMps,
    required this.velocityFps,
    required this.timestamp,
  });

  /// Doppler frequency → velocity factor (24.097 GHz K-band).
  static const double kFreqToMps = 160.647;
  static const double kMpsToFps = 3.280839895;

  @override
  String toString() =>
      '${velocityFps.toStringAsFixed(1)} fps / ${velocityMps.toStringAsFixed(1)} m/s · BC ${ballisticCoefficient.toStringAsFixed(4)}';
}
