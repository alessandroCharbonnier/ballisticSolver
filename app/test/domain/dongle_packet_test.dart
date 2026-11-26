import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';

import 'package:ballistic_sentinel/domain/dongle_packet.dart';

List<int> _payload({
  double temp = 21.4,
  double pressure = 1013.25,
  double humidity = 45.5,
  int seq = 7,
  int uptime = 123456,
}) {
  final b = ByteData(20);
  b.setFloat32(0, temp, Endian.little);
  b.setFloat32(4, pressure, Endian.little);
  b.setFloat32(8, humidity, Endian.little);
  b.setUint32(12, seq, Endian.little);
  b.setUint32(16, uptime, Endian.little);
  return b.buffer.asUint8List();
}

void main() {
  group('T5a dongle decoding', () {
    test('valid 20-byte payload gives the right values', () {
      final payload = decodeDonglePayload(_payload());
      expect(payload, isNotNull);
      expect(payload!.tempC, closeTo(21.4, 1e-5));
      expect(payload.pressureHpa, closeTo(1013.25, 1e-4));
      expect(payload.humidityPct, closeTo(45.5, 1e-5));
      expect(payload.sequence, 7);
      expect(payload.uptimeMs, 123456);
    });

    test('wrong length is rejected', () {
      expect(decodeDonglePayload(_payload().sublist(0, 19)), isNull);
      expect(decodeDonglePayload([..._payload(), 0]), isNull);
    });

    test('out-of-range temperature is rejected', () {
      expect(decodeDonglePayload(_payload(temp: 100)), isNull);
      expect(decodeDonglePayload(_payload(temp: -41)), isNull);
    });

    test('out-of-range pressure is rejected', () {
      expect(decodeDonglePayload(_payload(pressure: 1200)), isNull);
      expect(decodeDonglePayload(_payload(pressure: 299)), isNull);
    });

    test('out-of-range humidity is rejected', () {
      expect(decodeDonglePayload(_payload(humidity: 101)), isNull);
      expect(decodeDonglePayload(_payload(humidity: -1)), isNull);
    });
  });
}
