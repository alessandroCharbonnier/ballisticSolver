/// Weather dongle payload decoder (section 10.3).
///
/// Manufacturer data for company id 0xFFFF is exactly 20 bytes, little-endian:
///   float temp_c (0), float pressure_hpa (4), float humidity_pct (8),
///   uint32 seq (12), uint32 uptime_ms (16).
library;

import 'dart:typed_data';

import 'constants.dart';

class DonglePayload {
  final double tempC;
  final double pressureHpa;
  final double humidityPct;
  final int sequence;
  final int uptimeMs;

  const DonglePayload({
    required this.tempC,
    required this.pressureHpa,
    required this.humidityPct,
    required this.sequence,
    required this.uptimeMs,
  });
}

/// Decode a 20-byte dongle payload.
///
/// Returns null for a wrong length or out-of-range values (section 10.3).
DonglePayload? decodeDonglePayload(List<int> bytes) {
  if (bytes.length != AppConstants.donglePayloadLength) return null;

  final ByteData data = Uint8List.fromList(bytes).buffer.asByteData();
  final double tempC = data.getFloat32(0, Endian.little);
  final double pressureHpa = data.getFloat32(4, Endian.little);
  final double humidityPct = data.getFloat32(8, Endian.little);
  final int sequence = data.getUint32(12, Endian.little);
  final int uptimeMs = data.getUint32(16, Endian.little);

  if (tempC < AppConstants.minTempC || tempC > AppConstants.maxTempC) return null;
  if (pressureHpa < AppConstants.minPressureHpa ||
      pressureHpa > AppConstants.maxPressureHpa) {
    return null;
  }
  if (humidityPct < AppConstants.minHumidityPct ||
      humidityPct > AppConstants.maxHumidityPct) {
    return null;
  }

  return DonglePayload(
    tempC: tempC,
    pressureHpa: pressureHpa,
    humidityPct: humidityPct,
    sequence: sequence,
    uptimeMs: uptimeMs,
  );
}
