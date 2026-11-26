/// ChangeNotifier for the Fire screen (sections 9.4, 9.5, 9.6 and 11.4).
///
/// Load, unload, shot and undo are processed one at a time, in arrival order,
/// through a single queue (rule F5).
library;

import 'dart:async';

import 'package:flutter/foundation.dart' show ChangeNotifier;
import 'dart:ui' show Color;

import '../app/app_services.dart';
import '../data/boxes_repository.dart';
import '../data/magazine_repository.dart';
import '../data/shot_repository.dart';
import '../domain/constants.dart';
import '../domain/models.dart';
import '../domain/weather_tracker.dart';
import '../sources/radar_source.dart';
import '../sources/sim_radar_source.dart';
import '../sources/sim_weather_source.dart';
import '../widgets/box_grid.dart';

enum FirePhase { loading, askBarrelCleaned, active }

/// Serialises asynchronous tasks (rule F5).
class AsyncQueue {
  Future<void> _tail = Future<void>.value();

  Future<T> add<T>(Future<T> Function() task) {
    final result = _tail.then((_) => task());
    _tail = result.then<void>((_) {}, onError: (_) {});
    return result;
  }
}

class FireController extends ChangeNotifier {
  FireController(this.services);

  final AppServices services;

  final _queue = AsyncQueue();
  final WeatherTracker weather = WeatherTracker();

  FirePhase phase = FirePhase.loading;
  Session? session;

  List<BoxListItem> boxes = [];
  BoxListItem? selectedBox;
  Product? product;
  List<Cartridge> cartridges = [];
  List<MagazineEntry> magazine = [];

  RadarLinkState radarState = RadarLinkState.searching;
  Shot? lastShot;
  String? lastShotLabel;
  bool lastShotAssigned = false;
  int unassignedCount = 0;

  bool offerManualTemp = false;
  bool simDongleOn = false;

  StreamSubscription<RadarShot>? _radarSub;
  StreamSubscription<RadarLinkState>? _linkSub;
  StreamSubscription<WeatherReading>? _weatherSub;
  Timer? _manualTempTimer;
  bool _manualTempOffered = false;
  bool _sourcesStarted = false;

  bool get simulatorOn => services.simulatorOn;
  bool get magazineFull => magazine.length >= AppConstants.magazineCapacity;
  double? get manualTemp => session?.manualTempC;
  int get magazineCount => magazine.length;

  // ── lifecycle ─────────────────────────────────────────────────────────────

  Future<void> initialize() async {
    phase = FirePhase.loading;
    notifyListeners();

    boxes = await services.boxes.listVisibleBoxes();

    final lastBoxId =
        await services.settings.getInt(AppConstants.keyLastBoxId);
    BoxListItem? selected;
    if (lastBoxId != null) {
      for (final box in boxes) {
        if (box.box.id == lastBoxId) {
          selected = box;
          break;
        }
      }
    }
    selected ??= boxes.isEmpty ? null : boxes.first;
    selectedBox = selected;
    if (selected != null) {
      await _loadBoxContents();
    }

    final rifle = await services.sessions.activeRifle();
    if (rifle == null) {
      phase = FirePhase.askBarrelCleaned;
      notifyListeners();
      return;
    }

    final now = services.clock.now();
    final existing = await services.sessions.findSessionForDay(rifle.id!, now);
    if (existing != null) {
      // S1: continue today's session.
      session = existing;
      await _activateSession();
    } else {
      // S1: ask the cleaning question before creating a session.
      phase = FirePhase.askBarrelCleaned;
      notifyListeners();
    }
  }

  /// S1: create the session with the given cleaning answer, then start the
  /// sources (S2).
  Future<void> answerCleaning(bool cleaned) async {
    final rifle = await services.sessions.activeRifle();
    if (rifle == null) return;
    session = await services.sessions.createSession(
      rifleId: rifle.id!,
      barrelCleaned: cleaned,
      now: services.clock.now(),
    );
    await _activateSession();
  }

  Future<void> _activateSession() async {
    if (!_sourcesStarted) {
      await services.startSources();
      _sourcesStarted = true;
      _subscribeSources();
    }
    simDongleOn = simulatorOn;
    phase = FirePhase.active;
    notifyListeners();
    await _refreshAfterMutation();
    _scheduleManualTempOffer();
  }

  void _subscribeSources() {
    _radarSub = services.radarSource.shots.listen(_onRadarShot);
    _linkSub = services.radarSource.linkState.listen((state) {
      radarState = state;
      notifyListeners();
    });
    _weatherSub = services.weatherSource.readings.listen(_onWeather);
  }

  void _unsubscribeSources() {
    _radarSub?.cancel();
    _linkSub?.cancel();
    _weatherSub?.cancel();
    _radarSub = _linkSub = _weatherSub = null;
  }

  /// Leaving the Fire screen: stop the scanner and sources, update the
  /// session's `last_active_at` and write an automatic backup when new shots
  /// arrived (section 12.2).
  Future<void> leave() async {
    _manualTempTimer?.cancel();
    _manualTempTimer = null;
    _unsubscribeSources();
    await services.stopSources();
    _sourcesStarted = false;
    final current = session;
    if (current != null) {
      await services.sessions.updateLastActive(
        current.id!,
        services.clock.now(),
      );
      await services.backup.autoBackupIfNeeded(sessionId: current.id!);
    }
  }

  // ── box selection ─────────────────────────────────────────────────────────

  Future<void> selectBox(BoxListItem item) async {
    selectedBox = item;
    await _loadBoxContents();
    notifyListeners();
  }

  Future<void> _loadBoxContents() async {
    final box = selectedBox;
    if (box == null) {
      product = null;
      cartridges = [];
      return;
    }
    product = await services.boxes.productForBox(box.box.id!);
    cartridges = await services.boxes.getCartridges(box.box.id!);
  }

  // ── mutations (all through the queue, rule F5) ────────────────────────────

  Future<LoadResult> load(int cartridgeId) async {
    final result =
        await _queue.add(() => services.magazine.load(cartridgeId));
    await _refreshAfterMutation();
    return result;
  }

  Future<void> unloadTop() async {
    await _queue.add(() => services.magazine.unloadTop());
    await _refreshAfterMutation();
  }

  Future<UndoOutcome> undo() async {
    final current = session;
    if (current == null) {
      return const UndoOutcome(status: UndoStatus.nothingToUndo);
    }
    final outcome = await _queue.add(() => services.shots.undoLast(
          sessionId: current.id!,
          now: services.clock.now(),
        ));
    await _refreshAfterMutation();
    return outcome;
  }

  // ── radar and weather ─────────────────────────────────────────────────────

  void _onRadarShot(RadarShot shot) {
    _queue.add(() async {
      await _recordShot(shot);
      await _refreshAfterMutation();
    });
  }

  Future<void> _recordShot(RadarShot shot) async {
    final current = session;
    if (current == null) return; // Sources only run once a session exists.

    final now = services.clock.now();
    final weatherForShot = weather.resolve(now, current.manualTempC);
    final outcome = await services.recorder.record(
      sessionId: current.id!,
      freqHz: shot.freqHz,
      bc: shot.bc,
      weather: weatherForShot,
      now: now,
    );
    lastShot = outcome.shot;
    lastShotLabel = outcome.cartridgeLabel;
    lastShotAssigned = outcome.assigned;
  }

  void _onWeather(WeatherReading reading) {
    weather.add(reading);
    notifyListeners();
  }

  // ── manual temperature (rule W6) ──────────────────────────────────────────

  void _scheduleManualTempOffer() {
    _manualTempTimer?.cancel();
    _manualTempTimer = Timer(AppConstants.manualTempOfferDelay, () {
      final current = session;
      if (current == null || _manualTempOffered) return;
      final now = services.clock.now();
      if (!weather.isCurrent(now) && current.manualTempC == null) {
        _manualTempOffered = true;
        offerManualTemp = true;
        notifyListeners();
      }
    });
  }

  Future<void> saveManualTemp(double value) async {
    final current = session;
    if (current == null) return;
    await services.sessions.setManualTemp(current.id!, value);
    session = await services.sessions.getSession(current.id!);
    offerManualTemp = false;
    notifyListeners();
  }

  void skipManualTemp() {
    offerManualTemp = false;
    notifyListeners();
  }

  // ── simulator (section 12.4) ──────────────────────────────────────────────

  void simulateShot() {
    final radar = services.radarSource;
    if (radar is SimRadarSource) {
      radar.simulateShot();
    }
  }

  void setSimDongle(bool on) {
    simDongleOn = on;
    final source = services.weatherSource;
    if (source is SimWeatherSource) {
      if (on) {
        unawaited(source.start());
      } else {
        unawaited(source.stop());
      }
    }
    notifyListeners();
  }

  // ── refresh helpers ───────────────────────────────────────────────────────

  Future<void> _refreshAfterMutation() async {
    await _refreshMagazine();
    await _refreshCartridges();
    await _refreshUnassigned();
    notifyListeners();
  }

  Future<void> _refreshMagazine() async {
    magazine = await services.magazine.contents();
  }

  Future<void> _refreshCartridges() async {
    final box = selectedBox;
    if (box == null) return;
    cartridges = await services.boxes.getCartridges(box.box.id!);
  }

  Future<void> _refreshUnassigned() async {
    final current = session;
    if (current == null) {
      unassignedCount = 0;
      return;
    }
    final shots = await services.shots.shotsForSession(current.id!);
    unassignedCount =
        shots.where((s) => s.cartridgeId == null && !s.voided).length;
  }

  // ── grid ──────────────────────────────────────────────────────────────────

  List<BoxGridSlot> buildSlots() {
    return [
      for (final cartridge in cartridges)
        _slotFor(cartridge),
    ];
  }

  BoxGridSlot _slotFor(Cartridge cartridge) {
    if (cartridge.state == 'fired') {
      return BoxGridSlot(
        label: cartridge.weightG?.toStringAsFixed(2) ?? '',
        color: const Color(0xFF1B5E20),
        filled: true,
        isCircle: true,
        canTap: false,
      );
    }
    if (cartridge.state == 'magazine') {
      return BoxGridSlot(
        label: '${cartridge.magPosition}',
        color: const Color(0xFF2E7D32),
        filled: true,
        isCircle: true,
        canTap: false,
      );
    }
    if (cartridge.isMeasured) {
      return BoxGridSlot(
        label: cartridge.weightG?.toStringAsFixed(2) ?? '',
        color: const Color(0xFF607D8B),
        filled: false,
        isCircle: true,
        canTap: true,
        onTap: () => load(cartridge.id!),
      );
    }
    return BoxGridSlot(
      label: '',
      color: const Color(0xFF455A64),
      filled: false,
      isCircle: true,
      dimmed: true,
      canTap: false,
    );
  }

  @override
  void dispose() {
    _manualTempTimer?.cancel();
    _unsubscribeSources();
    super.dispose();
  }
}
