import 'package:flutter_test/flutter_test.dart';

import 'package:ballistic_sentinel/domain/models.dart';
import 'package:ballistic_sentinel/domain/shot_context.dart';

Shot _shot(int index, bool voided, DateTime firedAt) => Shot(
      shotIndex: index,
      freqHz: 51200,
      bc: 0.12,
      firedAt: firedAt,
      voided: voided,
    );

void main() {
  group('shot context (rules S4 to S7)', () {
    test('a cleaned session starts at 1 and is cold bore (S4, S5)', () {
      final ctx = computeShotContext(
        cleanBase: 0,
        earlierShots: const [],
        firedAt: DateTime.utc(2026, 11, 26, 12, 0, 0),
      );
      expect(ctx.shotsSinceClean, 1);
      expect(ctx.coldBore, isTrue);
      expect(ctx.timeSinceLastShotS, isNull);
      expect(ctx.shotIndex, 1);
    });

    test('an uncleaned session continues from clean_base (S4)', () {
      final ctx = computeShotContext(
        cleanBase: 30,
        earlierShots: [
          _shot(1, false, DateTime.utc(2026, 11, 26, 12, 0, 0)),
        ],
        firedAt: DateTime.utc(2026, 11, 26, 12, 0, 10),
      );
      expect(ctx.shotsSinceClean, 32); // 30 + 1 earlier + 1
      expect(ctx.coldBore, isFalse);
    });

    test('cold bore is only on the first non-voided shot (S5, F7)', () {
      final ctx = computeShotContext(
        cleanBase: 0,
        earlierShots: [
          _shot(1, true, DateTime.utc(2026, 11, 26, 12, 0, 0)),
        ],
        firedAt: DateTime.utc(2026, 11, 26, 12, 0, 10),
      );
      // The earlier shot was voided, so this is still the cold-bore shot.
      expect(ctx.coldBore, isTrue);
      expect(ctx.shotsSinceClean, 1);
    });

    test('time since last shot comes from the fake clock (S6)', () {
      final ctx = computeShotContext(
        cleanBase: 0,
        earlierShots: [
          _shot(1, false, DateTime.utc(2026, 11, 26, 12, 0, 0)),
        ],
        firedAt: DateTime.utc(2026, 11, 26, 12, 0, 10),
      );
      expect(ctx.timeSinceLastShotS, closeTo(10.0, 1e-9));
    });

    test('a voided shot is skipped by the time-since-last counter (S6, F7)',
        () {
      final ctx = computeShotContext(
        cleanBase: 0,
        earlierShots: [
          _shot(1, false, DateTime.utc(2026, 11, 26, 12, 0, 0)),
          _shot(2, true, DateTime.utc(2026, 11, 26, 12, 0, 5)),
        ],
        firedAt: DateTime.utc(2026, 11, 26, 12, 0, 10),
      );
      expect(ctx.timeSinceLastShotS, closeTo(10.0, 1e-9));
      expect(ctx.shotsSinceClean, 2); // 0 + 1 active earlier + 1
    });

    test('shot_index is never reused even when a shot is voided (S7)', () {
      final ctx = computeShotContext(
        cleanBase: 0,
        earlierShots: [
          _shot(1, true, DateTime.utc(2026, 11, 26, 12, 0, 0)),
          _shot(2, true, DateTime.utc(2026, 11, 26, 12, 0, 5)),
        ],
        firedAt: DateTime.utc(2026, 11, 26, 12, 0, 10),
      );
      expect(ctx.shotIndex, 3);
    });
  });
}
