/// FX True Ballistic radar shot packet decoder (section 10.2).
library;

import 'dart:typed_data';

import 'constants.dart';

class RadarShotPacket {
  final int freqHz;
  final double bc;
  final double velocityMps;

  const RadarShotPacket({
    required this.freqHz,
    required this.bc,
    required this.velocityMps,
  });
}

/// Decode a 9-byte shot notification.
///
///   [0x0B][0x00][freq u16 BE][BC float32 BE][0x05]
///
/// Returns null for a wrong length, a wrong first byte, a wrong last byte or a
/// zero frequency (section 10.2).
RadarShotPacket? decodeRadarPacket(List<int> bytes) {
  if (bytes.length != 9) return null;
  if (bytes[0] != 0x0B) return null;
  if (bytes[8] != 0x05) return null;

  final int freq = (bytes[2] << 8) | bytes[3];
  if (freq == 0) return null;

  final ByteData data = Uint8List.fromList(bytes.sublist(4, 8)).buffer.asByteData();
  final double bc = data.getFloat32(0, Endian.big);

  return RadarShotPacket(
    freqHz: freq,
    bc: bc,
    velocityMps: freq / AppConstants.freqToMps,
  );
}
