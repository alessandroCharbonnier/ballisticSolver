/// CSV export (section 12.1).
///
/// One row per shot, voided and unassigned included, oldest first. Comma
/// separated, UTF-8, dot as decimal separator, one header row. A field that
/// contains a comma, a quote or a line break is wrapped in double quotes with
/// inner quotes doubled. Empty values are empty fields.
library;

/// The header row, in the exact order of section 12.1.
const List<String> csvHeader = [
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
];

/// One export row. Time strings are pre-formatted so the builder stays a pure
/// text transformation.
class CsvShotRow {
  final int sessionId;
  final int shotIndex;
  final int? magPosition;
  final String? boxCode;
  final int? slotInBox;
  final double? weightG;
  final double? rimThicknessMm;
  final double? oalMm;
  final String? lot;
  final int freqHz;
  final double bc;
  final double? tempC;
  final double? pressureHpa;
  final double? humidityPct;
  final bool coldBore;
  final int shotsSinceClean;
  final double? timeSinceLastShotS;
  final int timestampMs;
  final String firedAtUtc;
  final String firedAtLocal;
  final String weatherSource;
  final bool voided;

  const CsvShotRow({
    required this.sessionId,
    required this.shotIndex,
    this.magPosition,
    this.boxCode,
    this.slotInBox,
    this.weightG,
    this.rimThicknessMm,
    this.oalMm,
    this.lot,
    required this.freqHz,
    required this.bc,
    this.tempC,
    this.pressureHpa,
    this.humidityPct,
    required this.coldBore,
    required this.shotsSinceClean,
    this.timeSinceLastShotS,
    required this.timestampMs,
    required this.firedAtUtc,
    required this.firedAtLocal,
    required this.weatherSource,
    required this.voided,
  });

  List<String> toCells() => [
        sessionId.toString(),
        shotIndex.toString(),
        _int(magPosition),
        boxCode ?? '',
        _int(slotInBox),
        _num(weightG),
        _num(rimThicknessMm),
        _num(oalMm),
        lot ?? '',
        freqHz.toString(),
        bc.toString(),
        _num(tempC),
        _num(pressureHpa),
        _num(humidityPct),
        coldBore ? '1' : '0',
        shotsSinceClean.toString(),
        _num(timeSinceLastShotS),
        timestampMs.toString(),
        firedAtUtc,
        firedAtLocal,
        weatherSource,
        voided ? '1' : '0',
      ];

  static String _num(double? value) => value == null ? '' : value.toString();

  static String _int(int? value) => value == null ? '' : value.toString();
}

/// Build the full CSV document from rows.
String buildCsv(List<CsvShotRow> rows) {
  final buffer = StringBuffer();
  buffer.writeln(csvHeader.map(escapeCsvCell).join(','));
  for (final row in rows) {
    buffer.writeln(row.toCells().map(escapeCsvCell).join(','));
  }
  return buffer.toString();
}

/// Quote a cell if it contains a comma, quote or line break.
String escapeCsvCell(String cell) {
  final needsQuotes = cell.contains(',') ||
      cell.contains('"') ||
      cell.contains('\n') ||
      cell.contains('\r');
  if (!needsQuotes) return cell;
  return '"${cell.replaceAll('"', '""')}"';
}

String _pad2(int n) => n.toString().padLeft(2, '0');
String _pad3(int n) => n.toString().padLeft(3, '0');
String _pad4(int n) => n.toString().padLeft(4, '0');

/// `2026-11-26T14:03:22.123Z`
String formatFiredAtUtc(DateTime time) {
  final u = time.toUtc();
  return '${_pad4(u.year)}-${_pad2(u.month)}-${_pad2(u.day)}'
      'T${_pad2(u.hour)}:${_pad2(u.minute)}:${_pad2(u.second)}.'
      '${_pad3(u.millisecond)}Z';
}

/// `2026-11-26 15:03:22.123` in the phone's local time.
String formatFiredAtLocal(DateTime time) {
  final l = time.toLocal();
  return '${_pad4(l.year)}-${_pad2(l.month)}-${_pad2(l.day)} '
      '${_pad2(l.hour)}:${_pad2(l.minute)}:${_pad2(l.second)}.'
      '${_pad3(l.millisecond)}';
}
