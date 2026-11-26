import 'package:flutter_test/flutter_test.dart';

import 'package:ballistic_sentinel/domain/clock.dart';
import 'package:ballistic_sentinel/domain/models.dart';
import 'package:ballistic_sentinel/domain/weather_tracker.dart';

void main() {
  group('T5b weather tracker', () {
    test('keeps only the latest reading (W1)', () {
      final tracker = WeatherTracker();
      final t0 = DateTime.utc(2026, 11, 26, 12, 0, 0);
      tracker.add(WeatherReading(
        tempC: 20, pressureHpa: 1000, humidityPct: 40, receivedAt: t0,
      ));
      tracker.add(WeatherReading(
        tempC: 21.4, pressureHpa: 1013, humidityPct: 45, receivedAt: t0,
      ));
      expect(tracker.latest!.tempC, 21.4);
      expect(tracker.latest!.pressureHpa, 1013);
    });

    test('a reading is current at 30 seconds (W2)', () {
      final tracker = WeatherTracker();
      final t0 = DateTime.utc(2026, 11, 26, 12, 0, 0);
      tracker.add(WeatherReading(
        tempC: 21.4, pressureHpa: 1013, humidityPct: 45, receivedAt: t0,
      ));
      expect(tracker.isCurrent(t0.add(const Duration(seconds: 30))), isTrue);
    });

    test('a reading is not current just after 30 seconds (W2)', () {
      final tracker = WeatherTracker();
      final t0 = DateTime.utc(2026, 11, 26, 12, 0, 0);
      tracker.add(WeatherReading(
        tempC: 21.4, pressureHpa: 1013, humidityPct: 45, receivedAt: t0,
      ));
      expect(
        tracker.isCurrent(
            t0.add(const Duration(seconds: 30, milliseconds: 1))),
        isFalse,
      );
    });

    test('a shot gets the current reading with source dongle (W3)', () {
      final tracker = WeatherTracker();
      final clock = FixedClock(DateTime.utc(2026, 11, 26, 12, 0, 0));
      tracker.add(WeatherReading(
        tempC: 21.4,
        pressureHpa: 1013,
        humidityPct: 45,
        receivedAt: clock.now(),
      ));
      final weather = tracker.resolve(clock.now(), null);
      expect(weather.source, 'dongle');
      expect(weather.tempC, 21.4);
      expect(weather.pressureHpa, 1013);
      expect(weather.humidityPct, 45);
    });

    test('with no reading and a manual temperature the source is manual (W3)',
        () {
      final tracker = WeatherTracker();
      final weather =
          tracker.resolve(DateTime.utc(2026, 11, 26, 12, 0, 0), 18.0);
      expect(weather.source, 'manual');
      expect(weather.tempC, 18.0);
      expect(weather.pressureHpa, isNull);
      expect(weather.humidityPct, isNull);
    });

    test('with neither a reading nor a manual temperature the source is none',
        () {
      final tracker = WeatherTracker();
      final weather =
          tracker.resolve(DateTime.utc(2026, 11, 26, 12, 0, 0), null);
      expect(weather.source, 'none');
      expect(weather.tempC, isNull);
    });
  });
}
