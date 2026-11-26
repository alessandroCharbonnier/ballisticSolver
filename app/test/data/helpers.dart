/// Shared helpers for data-layer tests.
library;

import 'package:sqflite_common_ffi/sqflite_ffi.dart';

import 'package:ballistic_sentinel/data/app_database.dart';
import 'package:ballistic_sentinel/data/boxes_repository.dart';
import 'package:ballistic_sentinel/data/magazine_repository.dart';
import 'package:ballistic_sentinel/data/session_repository.dart';
import 'package:ballistic_sentinel/data/shot_recorder.dart';
import 'package:ballistic_sentinel/data/shot_repository.dart';
import 'package:ballistic_sentinel/domain/models.dart';

bool _ffiInitialized = false;

/// Initialise the FFI database factory once per test run.
void ensureSqfliteFfi() {
  if (_ffiInitialized) return;
  sqfliteFfiInit();
  databaseFactory = databaseFactoryFfi;
  _ffiInitialized = true;
}

/// Open a fresh in-memory database (via sqflite_common_ffi).
Future<AppDatabase> openTestDatabase() async {
  ensureSqfliteFfi();
  final db = AppDatabase(inMemoryDatabasePath);
  await db.database; // force open
  return db;
}

class TestContext {
  TestContext(this.db);

  final AppDatabase db;
  late final BoxesRepository boxes = BoxesRepository(db);
  late final MagazineRepository magazine = MagazineRepository(db);
  late final SessionRepository sessions = SessionRepository(db);
  late final ShotRepository shots = ShotRepository(db);
  late final ShotRecorder recorder = ShotRecorder(
    db: db,
    sessions: sessions,
    shots: shots,
  );

  Future<int> activeRifleId() async =>
      (await sessions.activeRifle())!.id!;
}

class BoxFixture {
  final int productId;
  final int lotId;
  final int boxId;
  final List<Cartridge> cartridges;

  const BoxFixture({
    required this.productId,
    required this.lotId,
    required this.boxId,
    required this.cartridges,
  });
}

/// Create a product, lot, box and its empty cartridges.
Future<BoxFixture> createBoxFixture(
  TestContext ctx, {
  String productName = 'SK Rifle Match',
  String lotNumber = 'LOT1',
  String code = 'B01',
  int rows = 5,
  int cols = 10,
  DateTime? now,
}) async {
  final time = now ?? DateTime.utc(2026, 11, 26, 10, 0, 0);
  final productId = await ctx.boxes.insertProduct(
    Product(name: productName, rows: rows, cols: cols),
    now: time,
  );
  final lotId = await ctx.boxes.insertLot(
    Lot(productId: productId, lotNumber: lotNumber),
    now: time,
  );
  final boxId = await ctx.boxes.createBox(
    lotId: lotId,
    code: code,
    rows: rows,
    cols: cols,
    now: time,
  );
  final cartridges = await ctx.boxes.getCartridges(boxId);
  return BoxFixture(
    productId: productId,
    lotId: lotId,
    boxId: boxId,
    cartridges: cartridges,
  );
}

/// Measure a cartridge.
Future<void> measure(
  TestContext ctx,
  Cartridge cartridge, {
  double weightG = 3.35,
  double rimThicknessMm = 1.04,
  DateTime? now,
}) async {
  await ctx.boxes.saveMeasurement(
    cartridgeId: cartridge.id!,
    weightG: weightG,
    rimThicknessMm: rimThicknessMm,
    now: now ?? DateTime.utc(2026, 11, 26, 10, 0, 0),
  );
}
