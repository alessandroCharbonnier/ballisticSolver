/// ChangeNotifier for the Measure screen (grid + entry dialog).
library;

import 'dart:ui' show Color;

import 'package:flutter/foundation.dart' show ChangeNotifier;

import '../app/app_services.dart';
import '../domain/constants.dart';
import '../domain/measurement.dart';
import '../domain/models.dart';
import '../widgets/box_grid.dart';

class MeasureController extends ChangeNotifier {
  MeasureController(this.services, this.boxId);

  final AppServices services;
  final int boxId;

  Box? box;
  Product? product;
  Lot? lot;
  List<Cartridge> cartridges = [];
  bool loading = true;

  String get boxCode => box?.code ?? '';
  int get cols => product?.cols ?? AppConstants.defaultCols;

  Future<void> load() async {
    loading = true;
    notifyListeners();
    box = await services.boxes.getBox(boxId);
    product = await services.boxes.productForBox(boxId);
    lot = await services.boxes.lotForBox(boxId);
    cartridges = await services.boxes.getCartridges(boxId);
    loading = false;
    notifyListeners();
  }

  /// The first unmeasured cartridge in reading order, or null.
  Cartridge? nextEmptyCartridge() {
    for (final cartridge in cartridges) {
      if (!cartridge.isMeasured) return cartridge;
    }
    return null;
  }

  /// Slots for the measure grid.
  List<BoxGridSlot> buildSlots(void Function(Cartridge) onTap) {
    final nextEmpty = nextEmptyCartridge();
    final theme = _slotColors();
    return [
      for (final cartridge in cartridges)
        _slotFor(cartridge, nextEmpty, theme, onTap),
    ];
  }

  _SlotColors _slotColors() => const _SlotColors(
        empty: Color(0xFF607D8B),
        next: Color(0xFFFFB300),
        measured: Color(0xFF2E7D32),
        fired: Color(0xFF1B5E20),
      );

  BoxGridSlot _slotFor(
    Cartridge cartridge,
    Cartridge? nextEmpty,
    _SlotColors colors,
    void Function(Cartridge) onTap,
  ) {
    if (cartridge.state == 'fired') {
      return BoxGridSlot(
        label: _weightLabel(cartridge),
        color: colors.fired,
        filled: true,
        canTap: true,
        onTap: () => onTap(cartridge),
      );
    }
    if (cartridge.isMeasured) {
      return BoxGridSlot(
        label: _weightLabel(cartridge),
        color: colors.measured,
        filled: true,
        canTap: true,
        onTap: () => onTap(cartridge),
      );
    }
    final isNext = nextEmpty != null && cartridge.id == nextEmpty.id;
    return BoxGridSlot(
      label: '',
      color: isNext ? colors.next : colors.empty,
      filled: false,
      canTap: true,
      onTap: () => onTap(cartridge),
    );
  }

  String _weightLabel(Cartridge cartridge) =>
      cartridge.weightG?.toStringAsFixed(2) ?? '';

  /// M3: warnings for a candidate value, without saving.
  Future<List<String>> warningsFor(
    Cartridge cartridge, {
    required double weightG,
    required double rimThicknessMm,
  }) async {
    final values = await services.boxes.boxMeasuredValues(
      boxId,
      excludeCartridgeId: cartridge.id,
    );
    return MeasurementWarnings.evaluate(
      weightG: weightG,
      rimThicknessMm: rimThicknessMm,
      boxWeights: values.weights,
      boxRimThicknesses: values.rims,
    );
  }

  /// M5: save or correct a measurement, then refresh.
  Future<void> save(
    Cartridge cartridge, {
    required double weightG,
    required double rimThicknessMm,
  }) async {
    await services.boxes.saveMeasurement(
      cartridgeId: cartridge.id!,
      weightG: weightG,
      rimThicknessMm: rimThicknessMm,
      now: services.clock.now(),
    );
    await load();
  }
}

class _SlotColors {
  final Color empty;
  final Color next;
  final Color measured;
  final Color fired;
  const _SlotColors({
    required this.empty,
    required this.next,
    required this.measured,
    required this.fired,
  });
}
