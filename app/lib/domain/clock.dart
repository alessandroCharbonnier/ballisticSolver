/// A source of "now".
///
/// Production uses the system clock; tests pass a fixed clock so time-based
/// rules (weather staleness, session counters) are deterministic.
library;

abstract class Clock {
  DateTime now();
}

/// The real system clock.
class SystemClock implements Clock {
  const SystemClock();

  @override
  DateTime now() => DateTime.now();
}

/// A clock pinned to a fixed instant, with a helper to move it forward.
class FixedClock implements Clock {
  FixedClock(this.value);

  DateTime value;

  @override
  DateTime now() => value;

  void advance(Duration duration) {
    value = value.add(duration);
  }
}
