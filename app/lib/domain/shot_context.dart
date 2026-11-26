/// Shot-context calculator (rules S4 to S7), a pure function of the earlier
/// shots of the session plus the session's clean base.
library;

import 'models.dart';

/// The per-shot values derived from the session's history.
class ShotContext {
  final int shotsSinceClean;
  final bool coldBore;
  final double? timeSinceLastShotS;
  final int shotIndex;

  const ShotContext({
    required this.shotsSinceClean,
    required this.coldBore,
    required this.timeSinceLastShotS,
    required this.shotIndex,
  });
}

/// Compute the context of a new shot.
///
/// [earlierShots] holds every earlier shot of the session in firing order,
/// voided shots included. [firedAt] is when the new shot arrived.
///
/// - S4: `shots_since_clean = clean_base + earlier non-voided shots + 1`.
/// - S5: `cold_bore` is true for the first non-voided shot of the session.
/// - S6: `time_since_last_shot_s` is the seconds since the previous non-voided
///       shot; empty for the first shot.
/// - S7: `shot_index` continues from the highest index seen (voided included),
///       so indexes are never reused.
/// - F7: voided shots are skipped by the counters.
ShotContext computeShotContext({
  required int cleanBase,
  required List<Shot> earlierShots,
  required DateTime firedAt,
}) {
  final nonVoided = earlierShots.where((s) => !s.voided).toList();
  final int activeCount = nonVoided.length;

  final bool coldBore = activeCount == 0;

  final double? timeSinceLastShotS = activeCount == 0
      ? null
      : firedAt.difference(nonVoided.last.firedAt!).inMicroseconds / 1000000.0;

  var shotIndex = 1;
  for (final shot in earlierShots) {
    if (shot.shotIndex >= shotIndex) shotIndex = shot.shotIndex + 1;
  }

  return ShotContext(
    shotsSinceClean: cleanBase + activeCount + 1,
    coldBore: coldBore,
    timeSinceLastShotS: timeSinceLastShotS,
    shotIndex: shotIndex,
  );
}
