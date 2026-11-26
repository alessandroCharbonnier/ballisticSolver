import 'package:flutter_test/flutter_test.dart';

import 'package:borecore_app/domain/weather_tracker.dart';

import 'helpers.dart';

const _none = WeatherForShot(source: 'none');

void main() {
  late TestContext ctx;

  setUp(() async {
    ctx = TestContext(await openTestDatabase());
  });

  tearDown(() async => ctx.db.close());

  Future<int> newSession({
    required bool barrelCleaned,
    required DateTime now,
  }) async {
    final session = await ctx.sessions.createSession(
      rifleId: await ctx.activeRifleId(),
      barrelCleaned: barrelCleaned,
      now: now,
    );
    return session.id!;
  }

  group('T4 session counters', () {
    test('a cleaned session starts at 1 and cold bore is only the first shot',
        () async {
      final sessionId = await newSession(
        barrelCleaned: true,
        now: DateTime.utc(2026, 11, 26, 12, 0, 0),
      );

      final first = await ctx.recorder.record(
        sessionId: sessionId,
        freqHz: 51200,
        bc: 0.1197,
        weather: _none,
        now: DateTime.utc(2026, 11, 26, 12, 0, 1),
      );
      expect(first.shot.shotsSinceClean, 1);
      expect(first.shot.coldBore, isTrue);
      expect(first.shot.timeSinceLastShotS, isNull);
      expect(first.shot.shotIndex, 1);

      final second = await ctx.recorder.record(
        sessionId: sessionId,
        freqHz: 51200,
        bc: 0.1197,
        weather: _none,
        now: DateTime.utc(2026, 11, 26, 12, 0, 11),
      );
      expect(second.shot.shotsSinceClean, 2);
      expect(second.shot.coldBore, isFalse);
      expect(second.shot.shotIndex, 2);
    });

    test('an uncleaned session continues from the previous session (S3)',
        () async {
      final firstSession = await newSession(
        barrelCleaned: true,
        now: DateTime.utc(2026, 11, 26, 12, 0, 0),
      );
      await ctx.recorder.record(
        sessionId: firstSession,
        freqHz: 51200,
        bc: 0.1197,
        weather: _none,
        now: DateTime.utc(2026, 11, 26, 12, 0, 1),
      );
      await ctx.recorder.record(
        sessionId: firstSession,
        freqHz: 51200,
        bc: 0.1197,
        weather: _none,
        now: DateTime.utc(2026, 11, 26, 12, 0, 11),
      );

      // Next day, barrel not cleaned: clean_base carries over (2).
      final secondSession = await newSession(
        barrelCleaned: false,
        now: DateTime.utc(2026, 11, 27, 12, 0, 0),
      );
      final next = await ctx.recorder.record(
        sessionId: secondSession,
        freqHz: 51200,
        bc: 0.1197,
        weather: _none,
        now: DateTime.utc(2026, 11, 27, 12, 0, 1),
      );
      expect(next.shot.shotsSinceClean, 3);
      // S5: cold bore is the first shot of the session.
      expect(next.shot.coldBore, isTrue);
      expect(next.shot.shotIndex, 1); // new session restarts the index
    });

    test('time since last shot comes from the clock (S6)', () async {
      final sessionId = await newSession(
        barrelCleaned: true,
        now: DateTime.utc(2026, 11, 26, 12, 0, 0),
      );
      await ctx.recorder.record(
        sessionId: sessionId,
        freqHz: 51200,
        bc: 0.1197,
        weather: _none,
        now: DateTime.utc(2026, 11, 26, 12, 0, 0),
      );
      final next = await ctx.recorder.record(
        sessionId: sessionId,
        freqHz: 51200,
        bc: 0.1197,
        weather: _none,
        now: DateTime.utc(2026, 11, 26, 12, 0, 25),
      );
      expect(next.shot.timeSinceLastShotS, closeTo(25.0, 1e-9));
    });

    test('a voided shot is skipped by the counters and shot_index is never '
        'reused (F7, S7)', () async {
      final sessionId = await newSession(
        barrelCleaned: true,
        now: DateTime.utc(2026, 11, 26, 12, 0, 0),
      );

      await ctx.recorder.record(
        sessionId: sessionId,
        freqHz: 51200,
        bc: 0.1197,
        weather: _none,
        now: DateTime.utc(2026, 11, 26, 12, 0, 1),
      );
      await ctx.recorder.record(
        sessionId: sessionId,
        freqHz: 51200,
        bc: 0.1197,
        weather: _none,
        now: DateTime.utc(2026, 11, 26, 12, 0, 11),
      );

      // Void the second shot.
      await ctx.shots.undoLast(
        sessionId: sessionId,
        now: DateTime.utc(2026, 11, 26, 12, 0, 20),
      );

      final next = await ctx.recorder.record(
        sessionId: sessionId,
        freqHz: 51200,
        bc: 0.1197,
        weather: _none,
        now: DateTime.utc(2026, 11, 26, 12, 0, 21),
      );

      // The voided shot is skipped: 2 active shots (1 + this one).
      expect(next.shot.shotsSinceClean, 2);
      // Time since last is relative to the first shot (21 - 1 = 20 s).
      expect(next.shot.timeSinceLastShotS, closeTo(20.0, 1e-9));
      // The index continues from 2, so this shot is index 3 (never reused).
      expect(next.shot.shotIndex, 3);
    });
  });
}
