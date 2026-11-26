/// Decimal parsing for number inputs (rule M2).
///
/// The phone is set to French, so a comma or a dot is accepted as the decimal
/// separator. Any character that is not a digit, a comma or a dot is rejected.
library;

double? parseDecimal(String input) {
  final s = input.trim();
  if (s.isEmpty) return null;

  final buffer = StringBuffer();
  var separators = 0;

  for (final rune in s.runes) {
    final ch = String.fromCharCode(rune);
    if (ch == ',' || ch == '.') {
      separators++;
      if (separators > 1) return null; // "1,2.3" is not a number
      buffer.write('.');
    } else if (ch.codeUnitAt(0) >= 0x30 && ch.codeUnitAt(0) <= 0x39) {
      buffer.write(ch);
    } else {
      return null; // anything else is rejected
    }
  }

  final parsed = double.tryParse(buffer.toString());
  return parsed;
}
