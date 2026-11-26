import 'package:flutter_test/flutter_test.dart';

import 'package:ballistic_sentinel/domain/decimal_input.dart';

void main() {
  group('M2 decimal parser', () {
    test('a comma is accepted as the decimal separator', () {
      expect(parseDecimal('3,35'), closeTo(3.35, 1e-9));
    });

    test('a dot is accepted as the decimal separator', () {
      expect(parseDecimal('3.35'), closeTo(3.35, 1e-9));
    });

    test('an integer is accepted', () {
      expect(parseDecimal('42'), closeTo(42, 1e-9));
    });

    test('letters are rejected', () {
      expect(parseDecimal('3.3a'), isNull);
      expect(parseDecimal('abc'), isNull);
    });

    test('a second separator is rejected', () {
      expect(parseDecimal('1,2.3'), isNull);
    });

    test('empty input has no value', () {
      expect(parseDecimal(''), isNull);
    });
  });
}
