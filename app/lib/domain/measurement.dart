/// Measurement plausibility and box-median warnings (rule M3).
library;

import 'constants.dart';

class MeasurementWarnings {
  /// Median of a list of values (used for the box median warning).
  static double median(List<double> values) {
    if (values.isEmpty) return 0;
    final sorted = [...values]..sort();
    final n = sorted.length;
    if (n.isOdd) return sorted[n ~/ 2];
    return (sorted[n ~/ 2 - 1] + sorted[n ~/ 2]) / 2;
  }

  /// Evaluate warnings for a candidate measurement.
  ///
  /// [boxWeights] and [boxRimThicknesses] are the measured values of the other
  /// cartridges in the box (the slot being edited excluded).
  ///
  /// Warnings never block saving (rule M3).
  static List<String> evaluate({
    required double? weightG,
    required double? rimThicknessMm,
    required List<double> boxWeights,
    required List<double> boxRimThicknesses,
  }) {
    final warnings = <String>[];

    if (weightG != null &&
        (weightG < AppConstants.minWeightG || weightG > AppConstants.maxWeightG)) {
      warnings.add('Weight ${weightG.toStringAsFixed(2)} g is outside the '
          'plausible range (${AppConstants.minWeightG.toStringAsFixed(2)} to '
          '${AppConstants.maxWeightG.toStringAsFixed(2)} g).');
    }

    if (rimThicknessMm != null &&
        (rimThicknessMm < AppConstants.minRimThicknessMm ||
            rimThicknessMm > AppConstants.maxRimThicknessMm)) {
      warnings.add('Rim thickness ${rimThicknessMm.toStringAsFixed(2)} mm is '
          'outside the plausible range '
          '(${AppConstants.minRimThicknessMm.toStringAsFixed(2)} to '
          '${AppConstants.maxRimThicknessMm.toStringAsFixed(2)} mm).');
    }

    if (boxWeights.length >= AppConstants.medianMinMeasured && weightG != null) {
      final boxMedian = median(boxWeights);
      if ((weightG - boxMedian).abs() > AppConstants.medianWeightToleranceG) {
        warnings.add('Weight differs from the box median '
            '(${boxMedian.toStringAsFixed(2)} g) by more than '
            '${AppConstants.medianWeightToleranceG.toStringAsFixed(2)} g.');
      }
    }

    if (boxRimThicknesses.length >= AppConstants.medianMinMeasured &&
        rimThicknessMm != null) {
      final boxMedian = median(boxRimThicknesses);
      if ((rimThicknessMm - boxMedian).abs() >
          AppConstants.medianRimToleranceMm) {
        warnings.add('Rim thickness differs from the box median '
            '(${boxMedian.toStringAsFixed(2)} mm) by more than '
            '${AppConstants.medianRimToleranceMm.toStringAsFixed(2)} mm.');
      }
    }

    return warnings;
  }
}
