import 'package:flutter_test/flutter_test.dart';

import 'package:borecore_app/domain/measurement.dart';

void main() {
  group('M3 measurement warnings', () {
    test('a value in the plausible range gives no warning', () {
      final warnings = MeasurementWarnings.evaluate(
        weightG: 3.35,
        rimThicknessMm: 1.04,
        boxWeights: const [],
        boxRimThicknesses: const [],
      );
      expect(warnings, isEmpty);
    });

    test('an out-of-range weight gives a warning', () {
      final warnings = MeasurementWarnings.evaluate(
        weightG: 4.51,
        rimThicknessMm: 1.04,
        boxWeights: const [],
        boxRimThicknesses: const [],
      );
      expect(warnings, hasLength(1));
      expect(warnings.single, contains('plausible range'));
    });

    test('an out-of-range rim thickness gives a warning', () {
      final warnings = MeasurementWarnings.evaluate(
        weightG: 3.35,
        rimThicknessMm: 1.31,
        boxWeights: const [],
        boxRimThicknesses: const [],
      );
      expect(warnings, hasLength(1));
      expect(warnings.single, contains('plausible range'));
    });

    test('a value far from the box median gives a warning with 5+ measured',
        () {
      final warnings = MeasurementWarnings.evaluate(
        weightG: 3.90,
        rimThicknessMm: 1.04,
        boxWeights: const [3.30, 3.32, 3.34, 3.36, 3.38],
        boxRimThicknesses: const [1.00, 1.02, 1.04, 1.06, 1.08],
      );
      expect(warnings, hasLength(1));
      expect(warnings.single, contains('box median'));
    });

    test('the median warning needs at least 5 measured cartridges', () {
      final warnings = MeasurementWarnings.evaluate(
        weightG: 3.90,
        rimThicknessMm: 1.04,
        boxWeights: const [3.30, 3.32, 3.34, 3.36],
        boxRimThicknesses: const [1.00, 1.02, 1.04, 1.06],
      );
      expect(warnings, isEmpty);
    });
  });
}
