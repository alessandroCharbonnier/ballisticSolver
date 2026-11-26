import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';

import 'package:borecore_app/domain/radar_packet.dart';

List<int> _packet(int freq, double bc) {
  final b = ByteData(9);
  b.setUint8(0, 0x0B);
  b.setUint8(1, 0x00);
  b.setUint8(2, (freq >> 8) & 0xFF);
  b.setUint8(3, freq & 0xFF);
  b.setFloat32(4, bc, Endian.big);
  b.setUint8(8, 0x05);
  return b.buffer.asUint8List();
}

void main() {
  group('T1 radar packet decoding', () {
    test('valid packet gives the right frequency, BC and velocity', () {
      final packet = decodeRadarPacket(_packet(51200, 0.1197));
      expect(packet, isNotNull);
      expect(packet!.freqHz, 51200);
      expect(packet.bc, closeTo(0.1197, 1e-6));
      expect(packet.velocityMps, closeTo(51200 / 160.647, 1e-6));
    });

    test('all rows of the protocol table decode to the expected values', () {
      // (freq, bc) pairs from re/RADAR_PROTOCOL.md.
      const rows = [
        (51651, 0.1200),
        (51536, 0.1336),
        (52290, 0.1403),
        (50974, 0.1349),
        (51418, 0.1345),
        (52338, 0.1410),
      ];
      for (final (freq, bc) in rows) {
        final packet = decodeRadarPacket(_packet(freq, bc));
        expect(packet, isNotNull, reason: 'freq $freq');
        expect(packet!.freqHz, freq);
        expect(packet.bc, closeTo(bc, 1e-6));
        expect(packet.velocityMps, closeTo(freq / 160.647, 1e-6));
      }
    });

    test('wrong length is rejected', () {
      expect(decodeRadarPacket(_packet(51200, 0.1197).sublist(0, 8)), isNull);
      expect(decodeRadarPacket([..._packet(51200, 0.1197), 0]), isNull);
    });

    test('wrong first byte is rejected', () {
      final bytes = _packet(51200, 0.1197);
      bytes[0] = 0x0A;
      expect(decodeRadarPacket(bytes), isNull);
    });

    test('wrong last byte is rejected', () {
      final bytes = _packet(51200, 0.1197);
      bytes[8] = 0x04;
      expect(decodeRadarPacket(bytes), isNull);
    });

    test('zero frequency is rejected', () {
      expect(decodeRadarPacket(_packet(0, 0.1197)), isNull);
    });
  });
}
