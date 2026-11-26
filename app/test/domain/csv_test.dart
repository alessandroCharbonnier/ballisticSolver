import 'package:flutter_test/flutter_test.dart';

import 'package:ballistic_sentinel/domain/csv.dart';

void main() {
  group('T6c CSV', () {
    test('the header matches section 12.1 exactly', () {
      expect(csvHeader, [
        'session_id',
        'shot_index',
        'mag_position',
        'box_id',
        'slot_in_box',
        'weight_g',
        'rim_thickness_mm',
        'oal_mm',
        'lot',
        'freq_hz',
        'bc',
        'temp_c',
        'pressure_hpa',
        'humidity_pct',
        'cold_bore',
        'shots_since_clean',
        'time_since_last_shot_s',
        'timestamp_ms',
        'fired_at_utc',
        'fired_at_local',
        'weather_source',
        'voided',
      ]);
    });

    test('a voided row is written correctly', () {
      final csv = buildCsv([
        CsvShotRow(
          sessionId: 1,
          shotIndex: 2,
          magPosition: 1,
          boxCode: 'B01',
          slotInBox: 14,
          weightG: 3.35,
          rimThicknessMm: 1.04,
          oalMm: null,
          lot: 'LOT1',
          freqHz: 51200,
          bc: 0.1197,
          tempC: 21.4,
          pressureHpa: 1013,
          humidityPct: 45,
          coldBore: false,
          shotsSinceClean: 2,
          timeSinceLastShotS: 30.5,
          timestampMs: 1764158582123,
          firedAtUtc: '2026-11-26T14:03:22.123Z',
          firedAtLocal: '2026-11-26 15:03:22.123',
          weatherSource: 'dongle',
          voided: true,
        ),
      ]);
      final lines = csv.trim().split('\n');
      expect(lines.length, 2);
      final cells = lines[1].split(',');
      expect(cells[0], '1'); // session_id
      expect(cells[1], '2'); // shot_index
      expect(cells[2], '1'); // mag_position
      expect(cells[3], 'B01'); // box_id
      expect(cells[4], '14'); // slot_in_box
      expect(cells[5], '3.35'); // weight_g (dot decimal)
      expect(cells[9], '51200'); // freq_hz
      expect(cells[10], '0.1197'); // bc (dot decimal)
      expect(cells[20], 'dongle'); // weather_source
      expect(cells[21], '1'); // voided
    });

    test('an unassigned row is written correctly', () {
      final csv = buildCsv([
        CsvShotRow(
          sessionId: 3,
          shotIndex: 5,
          magPosition: null,
          boxCode: null,
          slotInBox: null,
          weightG: null,
          rimThicknessMm: null,
          oalMm: null,
          lot: null,
          freqHz: 50974,
          bc: 0.1349,
          tempC: null,
          pressureHpa: null,
          humidityPct: null,
          coldBore: false,
          shotsSinceClean: 5,
          timeSinceLastShotS: 12.25,
          timestampMs: 1764158582000,
          firedAtUtc: '2026-11-26T14:03:22.000Z',
          firedAtLocal: '2026-11-26 15:03:22.000',
          weatherSource: 'none',
          voided: false,
        ),
      ]);
      final cells = csv.trim().split('\n')[1].split(',');
      expect(cells[2], ''); // mag_position empty
      expect(cells[3], ''); // box_id empty
      expect(cells[5], ''); // weight_g empty
      expect(cells[20], 'none');
      expect(cells[21], '0');
    });

    test('a value containing a comma is quoted', () {
      final csv = buildCsv([
        CsvShotRow(
          sessionId: 1,
          shotIndex: 1,
          magPosition: null,
          boxCode: null,
          slotInBox: null,
          weightG: null,
          rimThicknessMm: null,
          oalMm: null,
          lot: 'LOT,A',
          freqHz: 51200,
          bc: 0.1197,
          tempC: null,
          pressureHpa: null,
          humidityPct: null,
          coldBore: true,
          shotsSinceClean: 1,
          timeSinceLastShotS: null,
          timestampMs: 1,
          firedAtUtc: '2026-11-26T14:03:22.000Z',
          firedAtLocal: '2026-11-26 15:03:22.000',
          weatherSource: 'none',
          voided: false,
        ),
      ]);
      final line = csv.trim().split('\n')[1];
      expect(line, contains('"LOT,A"'));
    });

    test('decimals use a dot', () {
      final csv = buildCsv([
        CsvShotRow(
          sessionId: 1,
          shotIndex: 1,
          magPosition: null,
          boxCode: null,
          slotInBox: null,
          weightG: 3.35,
          rimThicknessMm: 1.04,
          oalMm: null,
          lot: null,
          freqHz: 51200,
          bc: 0.1197,
          tempC: null,
          pressureHpa: null,
          humidityPct: null,
          coldBore: true,
          shotsSinceClean: 1,
          timeSinceLastShotS: 12.5,
          timestampMs: 1,
          firedAtUtc: '2026-11-26T14:03:22.000Z',
          firedAtLocal: '2026-11-26 15:03:22.000',
          weatherSource: 'none',
          voided: false,
        ),
      ]);
      final text = csv.trim();
      expect(text, contains('3.35'));
      expect(text, contains('12.5'));
      expect(text, isNot(contains('3,35')));
    });
  });
}
