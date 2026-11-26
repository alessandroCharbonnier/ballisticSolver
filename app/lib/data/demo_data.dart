/// Seeds the demo database (section 12.4).
///
/// When `sentinel_demo.db` is created it is filled with one product "Demo",
/// one lot "DEMO" and one box `D01` whose 50 cartridges all have measurements
/// (weight between 3.30 and 3.40 g, rim thickness between 1.00 and 1.08 mm).
library;

import 'dart:math';

import '../domain/constants.dart';
import 'app_database.dart';

Future<void> seedDemoDatabase(AppDatabase appDb) async {
  final db = await appDb.database;

  final existing = await db.rawQuery('SELECT COUNT(*) AS n FROM boxes');
  if (((existing.first['n'] as num?)?.toInt() ?? 0) > 0) return;

  final now = DateTime.now().millisecondsSinceEpoch;
  final random = Random(42);

  await db.transaction((txn) async {
    final productId = await txn.insert('products', {
      'name': AppConstants.demoProductName,
      'rows': 5,
      'cols': 10,
      'hinge_edge': 'top',
      'created_at': now,
    });
    final lotId = await txn.insert('lots', {
      'product_id': productId,
      'lot_number': AppConstants.demoLotNumber,
      'created_at': now,
    });
    final boxId = await txn.insert('boxes', {
      'lot_id': lotId,
      'code': AppConstants.demoBoxCode,
      'hidden': 0,
      'created_at': now,
    });

    final batch = txn.batch();
    for (var slot = 1; slot <= 50; slot++) {
      final weight = AppConstants.demoMinWeightG +
          random.nextDouble() *
              (AppConstants.demoMaxWeightG - AppConstants.demoMinWeightG);
      final rim = AppConstants.demoMinRimMm +
          random.nextDouble() *
              (AppConstants.demoMaxRimMm - AppConstants.demoMinRimMm);
      batch.insert('cartridges', {
        'box_id': boxId,
        'slot': slot,
        'weight_g': weight,
        'rim_thickness_mm': rim,
        'measured_at': now,
        'state': 'box',
      });
    }
    await batch.commit(noResult: true);
  });
}
