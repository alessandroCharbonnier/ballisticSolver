import 'dart:io';

import 'package:flutter_test/flutter_test.dart';

import 'package:ballistic_sentinel/data/app_database.dart';
import 'package:ballistic_sentinel/data/backup_service.dart';
import 'package:ballistic_sentinel/domain/weather_tracker.dart';

import 'helpers.dart';

const _none = WeatherForShot(source: 'none');

void main() {
  test('T6b backup round trip restores the data at backup time', () async {
    ensureSqfliteFfi();
    final dir = await Directory.systemTemp.createTemp('sentinel_backup_test');
    final dbPath = '${dir.path}${Platform.pathSeparator}sentinel.db';

    final ctx = TestContext(AppDatabase(dbPath));
    await ctx.db.database;
    final box = await createBoxFixture(ctx);
    await measure(ctx, box.cartridges[0]);
    await ctx.magazine.load(box.cartridges[0].id!);
    final session = await ctx.sessions.createSession(
      rifleId: await ctx.activeRifleId(),
      barrelCleaned: true,
      now: DateTime.utc(2026, 11, 26, 12, 0, 0),
    );
    await ctx.recorder.record(
      sessionId: session.id!,
      freqHz: 51200,
      bc: 0.1197,
      weather: _none,
      now: DateTime.utc(2026, 11, 26, 12, 0, 1),
    );

    final backup =
        BackupService(db: ctx.db, dbPath: dbPath, documentsPath: dir.path);
    final backupPath = await backup.makeBackup();

    // Change the data: record a second shot.
    await ctx.recorder.record(
      sessionId: session.id!,
      freqHz: 51200,
      bc: 0.1197,
      weather: _none,
      now: DateTime.utc(2026, 11, 26, 12, 0, 11),
    );
    expect((await ctx.shots.shotsForSession(session.id!)).length, 2);

    // Import the backup: the second shot is gone again.
    await backup.importBackup(backupPath);
    final restored = await ctx.shots.shotsForSession(session.id!);
    expect(restored.length, 1);
    expect(restored.single.shotIndex, 1);

    await ctx.db.close();
    await dir.delete(recursive: true);
  });

  test('T6b a file that is not a backup is rejected and changes nothing',
      () async {
    ensureSqfliteFfi();
    final dir = await Directory.systemTemp.createTemp('sentinel_backup_test');
    final dbPath = '${dir.path}${Platform.pathSeparator}sentinel.db';

    final ctx = TestContext(AppDatabase(dbPath));
    await ctx.db.database;
    final box = await createBoxFixture(ctx);
    await measure(ctx, box.cartridges[0]);

    final notBackup = File('${dir.path}${Platform.pathSeparator}not_backup.txt');
    await notBackup.writeAsString('not a database');

    final backup =
        BackupService(db: ctx.db, dbPath: dbPath, documentsPath: dir.path);
    final error = await backup.validateBackupFile(notBackup.path);
    expect(error, isNotNull);
    expect(error, contains('not a Ballistic Sentinel backup'));

    // Nothing changed: the box and its cartridge are still there.
    expect((await ctx.boxes.getCartridges(box.boxId)).length, 50);

    await ctx.db.close();
    await dir.delete(recursive: true);
  });
}
