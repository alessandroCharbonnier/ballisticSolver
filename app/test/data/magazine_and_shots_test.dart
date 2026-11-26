import 'package:flutter_test/flutter_test.dart';

import 'package:borecore_app/data/magazine_repository.dart';
import 'package:borecore_app/data/shot_repository.dart';
import 'package:borecore_app/domain/weather_tracker.dart';

import 'helpers.dart';

const _none = WeatherForShot(source: 'none');

void main() {
  late TestContext ctx;
  late BoxFixture box;

  setUp(() async {
    ctx = TestContext(await openTestDatabase());
    box = await createBoxFixture(ctx);
  });

  tearDown(() async => ctx.db.close());

  group('T2 shot assignment and magazine rules', () {
    test('a shot goes to the top cartridge (F2)', () async {
      // Measure and load three cartridges: positions 1, 2, 3.
      for (var i = 0; i < 3; i++) {
        await measure(ctx, box.cartridges[i]);
        expect(await ctx.magazine.load(box.cartridges[i].id!),
            LoadResult.loaded);
      }

      final sessionId = await ctx.sessions.createSession(
        rifleId: await ctx.activeRifleId(),
        barrelCleaned: true,
        now: DateTime.utc(2026, 11, 26, 12, 0, 0),
      ).then((s) => s.id!);

      final outcome = await ctx.recorder.record(
        sessionId: sessionId,
        freqHz: 51200,
        bc: 0.1197,
        weather: _none,
        now: DateTime.utc(2026, 11, 26, 12, 0, 1),
      );

      expect(outcome.assigned, isTrue);
      expect(outcome.shot.cartridgeId, box.cartridges[2].id);
      expect(outcome.shot.magPosition, 3);

      final topCartridge = await ctx.boxes.getCartridge(box.cartridges[2].id!);
      expect(topCartridge!.state, 'fired');
      expect(await ctx.magazine.count(), 2);
    });

    test('unloading returns the top to its slot (L3)', () async {
      for (var i = 0; i < 2; i++) {
        await measure(ctx, box.cartridges[i]);
        await ctx.magazine.load(box.cartridges[i].id!);
      }

      final result = await ctx.magazine.unloadTop();
      expect(result.unloaded, isTrue);
      expect(result.entry!.cartridge.id, box.cartridges[1].id);

      final cartridge = await ctx.boxes.getCartridge(box.cartridges[1].id!);
      expect(cartridge!.state, 'box');
      expect(cartridge.magPosition, isNull);
    });

    test('a shot with an empty magazine is unassigned (F3)', () async {
      final sessionId = await ctx.sessions.createSession(
        rifleId: await ctx.activeRifleId(),
        barrelCleaned: true,
        now: DateTime.utc(2026, 11, 26, 12, 0, 0),
      ).then((s) => s.id!);

      final outcome = await ctx.recorder.record(
        sessionId: sessionId,
        freqHz: 51200,
        bc: 0.1197,
        weather: _none,
        now: DateTime.utc(2026, 11, 26, 12, 0, 1),
      );

      expect(outcome.assigned, isFalse);
      expect(outcome.shot.cartridgeId, isNull);
      expect(outcome.shot.magPosition, isNull);
    });

    test('undo returns the cartridge to the top (F6)', () async {
      await measure(ctx, box.cartridges[0]);
      await ctx.magazine.load(box.cartridges[0].id!);

      final sessionId = await ctx.sessions.createSession(
        rifleId: await ctx.activeRifleId(),
        barrelCleaned: true,
        now: DateTime.utc(2026, 11, 26, 12, 0, 0),
      ).then((s) => s.id!);

      await ctx.recorder.record(
        sessionId: sessionId,
        freqHz: 51200,
        bc: 0.1197,
        weather: _none,
        now: DateTime.utc(2026, 11, 26, 12, 0, 1),
      );

      final outcome = await ctx.shots.undoLast(
        sessionId: sessionId,
        now: DateTime.utc(2026, 11, 26, 12, 0, 2),
      );

      expect(outcome.status, UndoStatus.done);

      final cartridge = await ctx.boxes.getCartridge(box.cartridges[0].id!);
      expect(cartridge!.state, 'magazine');
      expect(cartridge.magPosition, 1);
    });

    test('undo is refused with a full magazine (F6)', () async {
      // Measure 11 cartridges.
      for (var i = 0; i < 11; i++) {
        await measure(ctx, box.cartridges[i]);
      }
      // Load 10 (positions 1..10).
      for (var i = 0; i < 10; i++) {
        await ctx.magazine.load(box.cartridges[i].id!);
      }

      final sessionId = await ctx.sessions.createSession(
        rifleId: await ctx.activeRifleId(),
        barrelCleaned: true,
        now: DateTime.utc(2026, 11, 26, 12, 0, 0),
      ).then((s) => s.id!);

      // Fire one shot: the top (position 10) is attached and leaves.
      await ctx.recorder.record(
        sessionId: sessionId,
        freqHz: 51200,
        bc: 0.1197,
        weather: _none,
        now: DateTime.utc(2026, 11, 26, 12, 0, 1),
      );

      // Load one more cartridge: the magazine is full again (10).
      expect(await ctx.magazine.load(box.cartridges[10].id!),
          LoadResult.loaded);

      final outcome = await ctx.shots.undoLast(
        sessionId: sessionId,
        now: DateTime.utc(2026, 11, 26, 12, 0, 2),
      );

      expect(outcome.status, UndoStatus.refusedMagazineFull);
      expect(await ctx.magazine.count(), 10);
    });

    test('a load is refused for an unmeasured cartridge (L2)', () async {
      final result = await ctx.magazine.load(box.cartridges[0].id!);
      expect(result, LoadResult.notMeasured);
    });

    test('a load is refused for an 11th cartridge (L1, L2)', () async {
      for (var i = 0; i < 10; i++) {
        await measure(ctx, box.cartridges[i]);
        expect(await ctx.magazine.load(box.cartridges[i].id!),
            LoadResult.loaded);
      }
      await measure(ctx, box.cartridges[10]);
      expect(await ctx.magazine.load(box.cartridges[10].id!), LoadResult.full);
    });
  });
}
