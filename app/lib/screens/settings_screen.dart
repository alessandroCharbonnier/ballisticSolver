import 'dart:async';

import 'package:material_ui/material_ui.dart';

import '../app/app_scope.dart';
import '../app/app_services.dart';
import '../controllers/settings_controller.dart';
import '../sources/radar_source.dart';
import '../widgets/simulator_badge.dart';

/// Settings screen (section 11.6).
class SettingsScreen extends StatefulWidget {
  const SettingsScreen({super.key});

  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  SettingsController? _controller;
  final _rifleCtrl = TextEditingController();
  final _radarCtrl = TextEditingController();

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    if (_controller == null) {
      _controller = SettingsController(AppScope.of(context));
      _load();
    }
  }

  Future<void> _load() async {
    await _controller!.load();
    if (!mounted) return;
    _rifleCtrl.text = _controller!.rifleName;
    _radarCtrl.text = _controller!.radarRemoteId;
    setState(() {});
  }

  @override
  void dispose() {
    _rifleCtrl.dispose();
    _radarCtrl.dispose();
    _controller?.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final controller = _controller!;
    return Scaffold(
      appBar: AppBar(title: const Text('Settings'), actions: const [SimulatorBadge()]),
      body: ListenableBuilder(
        listenable: controller,
        builder: (context, _) {
          if (controller.loading) {
            return const Center(child: CircularProgressIndicator());
          }
          return Center(
            child: ConstrainedBox(
              constraints: const BoxConstraints(maxWidth: 640),
              child: ListView(
                padding: const EdgeInsets.all(16),
                children: [
                  TextField(
                    controller: _rifleCtrl,
                    decoration: const InputDecoration(
                      labelText: 'Rifle name',
                      border: OutlineInputBorder(),
                    ),
                    onSubmitted: (value) => controller.saveRifleName(value),
                  ),
                  const SizedBox(height: 16),
                  TextField(
                    controller: _radarCtrl,
                    decoration: const InputDecoration(
                      labelText: 'Radar id',
                      border: OutlineInputBorder(),
                    ),
                    onSubmitted: (value) => controller.saveRadarId(value),
                  ),
                  const SizedBox(height: 16),
                  Card(
                    child: ListTile(
                      leading: const Icon(Icons.sensors),
                      title: Text('Dongle: '
                          '${controller.dongleRemoteId ?? 'none yet'}'),
                      trailing: TextButton(
                        onPressed: controller.dongleRemoteId == null
                            ? null
                            : () async {
                                await controller.forgetDongle();
                                if (!context.mounted) return;
                                ScaffoldMessenger.of(context).showSnackBar(
                                    const SnackBar(
                                        content: Text('Dongle forgotten')));
                              },
                        child: const Text('Forget dongle'),
                      ),
                    ),
                  ),
                  const SizedBox(height: 24),
                  SwitchListTile(
                    title: const Text('Simulator'),
                    subtitle: const Text(
                        'Use the demo database and simulated radar and dongle'),
                    value: controller.simulatorOn,
                    onChanged: (value) async {
                      await controller.setSimulator(value);
                      if (!context.mounted) return;
                      await AppScope.rebuildOf(context)();
                    },
                  ),
                  const SizedBox(height: 24),
                  if (!controller.simulatorOn)
                    _BluetoothCheckPanel(services: AppScope.of(context)),
                  const SizedBox(height: 24),
                  Text('App version ${controller.appVersion}',
                      style: Theme.of(context).textTheme.bodySmall),
                ],
              ),
            ),
          );
        },
      ),
    );
  }
}

/// The Bluetooth check (section 11.6): starts the scanner and both sources and
/// shows the radar link state, the last shot and the last reading with its
/// age. Saves nothing.
class _BluetoothCheckPanel extends StatefulWidget {
  const _BluetoothCheckPanel({required this.services});

  final AppServices services;

  @override
  State<_BluetoothCheckPanel> createState() => _BluetoothCheckPanelState();
}

class _BluetoothCheckPanelState extends State<_BluetoothCheckPanel> {
  bool _running = false;
  RadarLinkState _radarState = RadarLinkState.searching;
  RadarShot? _lastShot;
  String _lastReading = 'No reading yet';
  DateTime? _lastReadingAt;
  Timer? _ticker;

  StreamSubscription<RadarLinkState>? _linkSub;
  StreamSubscription<RadarShot>? _shotSub;
  StreamSubscription<dynamic>? _readingSub;

  Future<void> _start() async {
    setState(() => _running = true);
    final services = widget.services;

    _linkSub = services.radarSource.linkState.listen((s) {
      if (mounted) setState(() => _radarState = s);
    });
    _shotSub = services.radarSource.shots.listen((s) {
      if (mounted) setState(() => _lastShot = s);
    });
    _readingSub = services.weatherSource.readings.listen((r) {
      if (mounted) {
        setState(() {
          _lastReading =
              '${r.tempC.toStringAsFixed(1)} C  '
              '${r.pressureHpa.toStringAsFixed(0)} hPa  '
              '${r.humidityPct.toStringAsFixed(0)} %';
          _lastReadingAt = r.receivedAt;
        });
      }
    });

    await services.radarSource.start();
    await services.weatherSource.start();
    await services.scanner?.start();

    _ticker = Timer.periodic(const Duration(seconds: 1), (_) {
      if (mounted) setState(() {});
    });
  }

  Future<void> _stop() async {
    _ticker?.cancel();
    _ticker = null;
    await _linkSub?.cancel();
    await _shotSub?.cancel();
    await _readingSub?.cancel();
    _linkSub = _shotSub = _readingSub = null;

    final services = widget.services;
    await services.scanner?.stop();
    await services.radarSource.stop();
    await services.weatherSource.stop();

    if (mounted) setState(() => _running = false);
  }

  @override
  void dispose() {
    _ticker?.cancel();
    _linkSub?.cancel();
    _shotSub?.cancel();
    _readingSub?.cancel();
    super.dispose();
  }

  String get _ageText {
    final at = _lastReadingAt;
    if (at == null) return '';
    final age = DateTime.now().difference(at);
    return '${age.inSeconds} s ago';
  }

  Color _stateColor(RadarLinkState state) {
    switch (state) {
      case RadarLinkState.connected:
        return Colors.green;
      case RadarLinkState.connecting:
      case RadarLinkState.searching:
        return Colors.amber;
      case RadarLinkState.disconnected:
        return Colors.red;
    }
  }

  String _stateLabel(RadarLinkState state) {
    switch (state) {
      case RadarLinkState.connected:
        return 'Radar connected';
      case RadarLinkState.connecting:
        return 'Connecting to radar';
      case RadarLinkState.searching:
        return 'Searching for radar';
      case RadarLinkState.disconnected:
        return 'Radar disconnected';
    }
  }

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Row(
              children: [
                Text('Bluetooth check',
                    style: Theme.of(context).textTheme.titleMedium),
                const Spacer(),
                FilledButton(
                  onPressed: _running ? _stop : _start,
                  child: Text(_running ? 'Stop' : 'Start check'),
                ),
              ],
            ),
            const SizedBox(height: 12),
            Row(
              children: [
                Container(
                  width: 12,
                  height: 12,
                  decoration: BoxDecoration(
                    color: _stateColor(_radarState),
                    shape: BoxShape.circle,
                  ),
                ),
                const SizedBox(width: 8),
                Text(_stateLabel(_radarState)),
              ],
            ),
            const SizedBox(height: 8),
            Text('Last shot: '
                '${_lastShot == null ? '-' : '${_lastShot!.velocityMps.toStringAsFixed(1)} m/s, BC ${_lastShot!.bc.toStringAsFixed(4)}'}'),
            const SizedBox(height: 8),
            Text('Last reading: $_lastReading'
                '${_ageText.isEmpty ? '' : '  ($_ageText)'}'),
          ],
        ),
      ),
    );
  }
}
