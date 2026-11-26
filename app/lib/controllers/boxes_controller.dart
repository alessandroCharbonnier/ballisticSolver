/// ChangeNotifier for the Boxes screen (list, create, hide).
library;

import 'package:flutter/foundation.dart';

import '../app/app_services.dart';
import '../data/boxes_repository.dart';
import '../domain/constants.dart';
import '../domain/models.dart';

class BoxesController extends ChangeNotifier {
  BoxesController(this.services);

  final AppServices services;

  List<BoxListItem> boxes = [];
  bool loading = true;
  String? error;

  Future<void> load() async {
    loading = true;
    error = null;
    notifyListeners();
    try {
      boxes = await services.boxes.listVisibleBoxes();
    } catch (e) {
      error = '$e';
    }
    loading = false;
    notifyListeners();
  }

  Future<String> nextFreeBoxCode() => services.boxes.nextFreeBoxCode();

  Future<bool> boxCodeExists(String code) => services.boxes.boxCodeExists(code);

  Future<List<String>> productNames() async =>
      (await services.boxes.listProducts()).map((p) => p.name).toList();

  Future<List<String>> lotNumbersFor(String productName) async {
    final productId = await services.boxes.findProductIdByName(productName);
    if (productId == null) return const [];
    return (await services.boxes.listLotsForProduct(productId))
        .map((l) => l.lotNumber)
        .toList();
  }

  Future<String?> lastProductName() async {
    final id = await services.settings.getInt(AppConstants.keyLastProductId);
    if (id == null) return null;
    final product = await services.boxes.getProduct(id);
    return product?.name;
  }

  Future<String?> lastLotNumber() async {
    final id = await services.settings.getInt(AppConstants.keyLastLotId);
    if (id == null) return null;
    final lot = await services.boxes.getLot(id);
    return lot?.lotNumber;
  }

  /// Create the product (if new), lot (if new) and box with its empty
  /// cartridges. Returns the new box id.
  Future<int> createBox({
    required String productName,
    required String lotNumber,
    required String code,
    required int rows,
    required int cols,
    required String hingeEdge,
  }) async {
    final now = services.clock.now();

    final existing = await services.boxes.findProductIdByName(productName);
    late final int productId;
    late final int actualRows;
    late final int actualCols;

    if (existing == null) {
      productId = await services.boxes.insertProduct(
        Product(
          name: productName,
          rows: rows,
          cols: cols,
          hingeEdge: hingeEdge,
        ),
        now: now,
      );
      actualRows = rows;
      actualCols = cols;
    } else {
      final product = (await services.boxes.getProduct(existing))!;
      productId = existing;
      actualRows = product.rows;
      actualCols = product.cols;
    }

    var lotId = await services.boxes.findLotId(productId, lotNumber);
    lotId ??= await services.boxes.insertLot(
      Lot(productId: productId, lotNumber: lotNumber),
      now: now,
    );

    final boxId = await services.boxes.createBox(
      lotId: lotId,
      code: code,
      rows: actualRows,
      cols: actualCols,
      now: now,
    );

    await services.settings.set(
        AppConstants.keyLastProductId, productId.toString());
    await services.settings.set(AppConstants.keyLastLotId, lotId.toString());
    await services.settings.set(AppConstants.keyLastBoxId, boxId.toString());

    return boxId;
  }

  /// Hide a box. Returns an error message, or null on success.
  Future<String?> hideBox(BoxListItem item) async {
    if (await services.boxes.hasCartridgeInMagazine(item.box.id!)) {
      return 'Cannot hide a box while one of its cartridges is in the magazine.';
    }
    await services.boxes.hideBox(item.box.id!);
    await load();
    return null;
  }
}
