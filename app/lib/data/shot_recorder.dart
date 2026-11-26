/// Orchestrates recording a shot: computes the shot context from the
/// session's history (rules S4 to S7) and stores the shot (rules F1 to F4).
///
/// The Fire controller calls this through its single queue; tests call it
/// directly to prove the session counters end to end (test T4).
library;

import '../domain/shot_context.dart';
import '../domain/weather_tracker.dart';
import 'app_database.dart';
import 'session_repository.dart';
import 'shot_repository.dart';

class ShotRecorder {
  ShotRecorder({
    required this.db,
    required this.sessions,
    required this.shots,
  });

  final AppDatabase db;
  final SessionRepository sessions;
  final ShotRepository shots;

  Future<RecordShotOutcome> record({
    required int sessionId,
    required int freqHz,
    required double bc,
    required WeatherForShot weather,
    required DateTime now,
  }) async {
    final session = await sessions.getSession(sessionId);
    if (session == null) {
      throw StateError('Session $sessionId does not exist');
    }
    final earlierShots = await shots.shotsForSession(sessionId);
    final context = computeShotContext(
      cleanBase: session.cleanBase,
      earlierShots: earlierShots,
      firedAt: now,
    );
    return shots.recordShot(
      sessionId: sessionId,
      context: context,
      weather: weather,
      now: now,
      freqHz: freqHz,
      bc: bc,
    );
  }
}
