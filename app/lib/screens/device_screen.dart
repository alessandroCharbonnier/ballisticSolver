import 'dart:async';

import 'package:flutter/services.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:material_ui/material_ui.dart';

import '../ble/gatt.dart';
import '../ble/sentinel_device.dart';

/// Connected-device screen: live dashboard + config editor, gated by the
/// device PIN for writes.
class DeviceScreen extends StatefulWidget {
  const DeviceScreen({super.key, required this.device});

  final BluetoothDevice device;

  @override
  State<DeviceScreen> createState() => _DeviceScreenState();
}

class _DeviceScreenState extends State<DeviceScreen> {
  SentinelDevice? _dev;
  bool _connecting = true;
  String? _error;
  bool _authed = false;

  RifleConfig? _config;
  Telemetry _telemetry = Telemetry();
  String _lastStatus = '';

  StreamSubscription<Telemetry>? _telSub;
  StreamSubscription<String>? _statSub;

  // Config form controllers (filled after read).
  final _bc = TextEditingController();
  final _mv = TextEditingController();
  final _weight = TextEditingController();
  final _caliber = TextEditingController();
  final _sh = TextEditingController();
  final _twist = TextEditingController();
  final _zero = TextEditingController();
  final _tempF = TextEditingController();
  final _pressInhg = TextEditingController();
  final _humidity = TextEditingController();
  String _dragTable = 'G7';
  String _corrUnit = 'MOA';
  final _stageNameCtrl = TextEditingController();

  static const _dragTables = ['G1', 'G7', 'G2', 'G5', 'G6', 'G8', 'GI', 'GS', 'RA4'];
  static const _corrUnits = ['MOA', 'SMOA', 'MRAD', 'CM', 'CLICKS'];

  @override
  void initState() {
    super.initState();
    _connect();
  }

  Future<void> _connect() async {
    setState(() {
      _connecting = true;
      _error = null;
    });
    try {
      final d = SentinelDevice(widget.device);
      await d.connectAndDiscover();
      _telSub = d.telemetry.listen((t) {
        if (mounted) setState(() => _telemetry = t);
      });
      _statSub = d.status.listen(_onStatus);
      final cfg = await d.readConfig();
      if (!mounted) return;
      setState(() {
        _dev = d;
        _config = cfg;
        _connecting = false;
      });
      _fillForm(cfg);
    } catch (e) {
      if (!mounted) return;
      setState(() {
        _connecting = false;
        _error = '$e';
      });
    }
  }

  void _onStatus(String s) {
    if (!mounted) return;
    setState(() => _lastStatus = s);
    if (s == 'authed') {
      setState(() => _authed = true);
    } else if (s == 'bad pin') {
      setState(() => _authed = false);
      ScaffoldMessenger.of(context)
          .showSnackBar(const SnackBar(content: Text('Wrong PIN')));
    }
  }

  void _fillForm(RifleConfig c) {
    _bc.text = c.bc.toStringAsFixed(3);
    _mv.text = c.mv.toStringAsFixed(0);
    _weight.text = c.weight.toStringAsFixed(1);
    _caliber.text = c.caliber.toStringAsFixed(3);
    _sh.text = c.sh.toStringAsFixed(2);
    _twist.text = c.twist.toStringAsFixed(1);
    _zero.text = c.zero.toStringAsFixed(0);
    _tempF.text = c.manual.tempF.toStringAsFixed(1);
    _pressInhg.text = c.manual.pressureInhg.toStringAsFixed(2);
    _humidity.text = c.manual.humidity.toStringAsFixed(0);
    _dragTable = c.dragTable;
    _corrUnit = c.corrUnit;
  }

  Future<void> _promptPin() async {
    final ctrl = TextEditingController();
    final pin = await showDialog<String>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Device PIN'),
        content: TextField(
          controller: ctrl,
          autofocus: true,
          keyboardType: TextInputType.number,
          maxLength: 6,
          inputFormatters: [FilteringTextInputFormatter.digitsOnly],
          decoration: const InputDecoration(labelText: '4-digit PIN (on OLED)'),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(ctx, ctrl.text),
            child: const Text('Unlock'),
          ),
        ],
      ),
    );
    if (pin != null && pin.isNotEmpty) {
      await _dev?.auth(int.parse(pin));
    }
  }

  Future<void> _save() async {
    final dev = _dev;
    final c = _config;
    if (dev == null || c == null) return;

    c.bc = double.tryParse(_bc.text) ?? c.bc;
    c.mv = double.tryParse(_mv.text) ?? c.mv;
    c.weight = double.tryParse(_weight.text) ?? c.weight;
    c.caliber = double.tryParse(_caliber.text) ?? c.caliber;
    c.sh = double.tryParse(_sh.text) ?? c.sh;
    c.twist = double.tryParse(_twist.text) ?? c.twist;
    c.zero = double.tryParse(_zero.text) ?? c.zero;
    c.dragTable = _dragTable;
    c.corrUnit = _corrUnit;
    c.manual.tempF = double.tryParse(_tempF.text) ?? c.manual.tempF;
    c.manual.pressureInhg =
        double.tryParse(_pressInhg.text) ?? c.manual.pressureInhg;
    c.manual.humidity = double.tryParse(_humidity.text) ?? c.manual.humidity;

    try {
      await dev.writeConfig(c);
      if (mounted) {
        ScaffoldMessenger.of(context)
            .showSnackBar(const SnackBar(content: Text('Config saved')));
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context)
            .showSnackBar(SnackBar(content: Text('Save failed: $e')));
      }
    }
  }

  Future<void> _startCantCal() async {
    await _dev?.startCantCalibration();
  }

  void _addStage() {
    final c = _config;
    final name = _stageNameCtrl.text.trim();
    if (c == null || name.isEmpty) return;
    setState(() {
      c.stages.add(StageTarget(
        name: name,
        distance: int.tryParse(_zero.text) ?? 100,
      ));
      _stageNameCtrl.clear();
    });
  }

  @override
  void dispose() {
    _telSub?.cancel();
    _statSub?.cancel();
    _dev?.dispose();
    for (final c in [_bc, _mv, _weight, _caliber, _sh, _twist, _zero, _tempF, _pressInhg, _humidity, _stageNameCtrl]) {
      c.dispose();
    }
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    if (_connecting) {
      return const Scaffold(
        body: Center(child: CircularProgressIndicator()),
      );
    }
    if (_error != null) {
      return Scaffold(
        appBar: AppBar(title: const Text('Connection failed')),
        body: Center(
          child: Padding(
            padding: const EdgeInsets.all(24),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                Text(_error!, textAlign: TextAlign.center),
                const SizedBox(height: 16),
                FilledButton(onPressed: _connect, child: const Text('Retry')),
              ],
            ),
          ),
        ),
      );
    }

    return DefaultTabController(
      length: 2,
      child: Scaffold(
        appBar: AppBar(
          title: Text(widget.device.platformName.isEmpty
              ? 'Ballistic Sentinel'
              : widget.device.platformName),
          actions: [
            IconButton(
              tooltip: 'Disconnect',
              icon: const Icon(Icons.link_off),
              onPressed: () => Navigator.of(context).pop(),
            ),
          ],
          bottom: const TabBar(tabs: [Tab(text: 'Live'), Tab(text: 'Config')]),
        ),
        body: TabBarView(
          children: [_buildDashboard(), _buildConfig()],
        ),
      ),
    );
  }

  // ── Live dashboard ────────────────────────────────────────────────────────

  Widget _buildDashboard() {
    final t = _telemetry;
    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        _statusBanner(),
        const SizedBox(height: 8),
        _card('Weather', [
          _row('Temp', t.weatherLive
              ? '${t.tempF.toStringAsFixed(1)} °F'
              : '${t.tempF.toStringAsFixed(1)} °F (manual)'),
          _row('Pressure', '${t.pressureInhg.toStringAsFixed(2)} inHg'),
          _row('Humidity', '${t.humidity.toStringAsFixed(0)} %'),
        ]),
        _card('Rifle', [
          _row('Cant', '${t.cantDeg.toStringAsFixed(1)} °'),
          _row('Heading', '${t.headingDeg.toStringAsFixed(0)} °'),
          _row('Distance', '${t.distance.toStringAsFixed(0)} ${t.staged ? 'yd (stage)' : 'yd'}'),
        ]),
        _card('Correction', [
          _row('Vertical', t.vCorr.toStringAsFixed(2)),
          _row('Horizontal', t.hCorr.toStringAsFixed(2)),
        ]),
        _card('Battery', [
          _row('Voltage', t.batteryPct >= 0 ? '${t.batteryV.toStringAsFixed(2)} V' : '—'),
          _row('Charge', t.batteryPct >= 0 ? '${t.batteryPct.toStringAsFixed(0)} %' : '—'),
        ]),
        FilledButton.icon(
          onPressed: _authed ? _startCantCal : null,
          icon: const Icon(Icons.straighten),
          label: const Text('Start cant calibration'),
        ),
      ],
    );
  }

  Widget _statusBanner() {
    final authed = _authed;
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: authed ? Colors.green.shade900 : Colors.orange.shade900,
        borderRadius: BorderRadius.circular(8),
      ),
      child: Row(
        children: [
          Icon(authed ? Icons.lock_open : Icons.lock,
              color: Colors.white),
          const SizedBox(width: 8),
          Expanded(
            child: Text(
              _lastStatus.isEmpty
                  ? (authed ? 'Authenticated' : 'Locked — PIN needed to edit')
                  : _lastStatus,
              style: const TextStyle(color: Colors.white),
            ),
          ),
        ],
      ),
    );
  }

  Widget _card(String title, List<Widget> rows) {
    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(title, style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 8),
            ...rows,
          ],
        ),
      ),
    );
  }

  Widget _row(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label),
          Text(value, style: Theme.of(context).textTheme.bodyLarge),
        ],
      ),
    );
  }

  // ── Config editor ─────────────────────────────────────────────────────────

  Widget _buildConfig() {
    final c = _config;
    if (c == null) return const Center(child: Text('No config'));
    final locked = !_authed;

    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        if (locked)
          Card(
            color: Colors.orange.shade900,
            child: ListTile(
              leading: const Icon(Icons.lock, color: Colors.white),
              title: const Text('Locked', style: TextStyle(color: Colors.white)),
              subtitle: const Text('Enter PIN to edit', style: TextStyle(color: Colors.white70)),
              trailing: FilledButton(
                onPressed: _promptPin,
                child: const Text('Unlock'),
              ),
            ),
          ),
        _field('BC', _bc),
        _dropdown('Drag table', _dragTable, _dragTables, (v) => setState(() => _dragTable = v!)),
        _field('Muzzle velocity (fps)', _mv),
        _field('Bullet weight (gr)', _weight),
        _field('Caliber (in)', _caliber),
        _field('Sight height (in)', _sh),
        _field('Twist (in)', _twist),
        _field('Zero range (yd)', _zero),
        _dropdown('Correction unit', _corrUnit, _corrUnits, (v) => setState(() => _corrUnit = v!)),
        _section('Manual weather fallback'),
        _field('Temp (°F)', _tempF),
        _field('Pressure (inHg)', _pressInhg),
        _field('Humidity (%)', _humidity),
        _section('Stages (${c.stages.length})'),
        for (var i = 0; i < c.stages.length; i++)
          ListTile(
            dense: true,
            leading: const Icon(Icons.flag),
            title: Text(c.stages[i].name),
            trailing: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                Text('${c.stages[i].distance} yd'),
                IconButton(
                  icon: const Icon(Icons.delete_outline),
                  onPressed: locked ? null : () => setState(() => c.stages.removeAt(i)),
                ),
              ],
            ),
          ),
        Row(
          children: [
            Expanded(
              child: TextField(
                controller: _stageNameCtrl,
                enabled: !locked,
                decoration: const InputDecoration(labelText: 'Stage name'),
              ),
            ),
            IconButton(
              tooltip: 'Add stage',
              icon: const Icon(Icons.add),
              onPressed: locked ? null : _addStage,
            ),
          ],
        ),
        const SizedBox(height: 16),
        FilledButton.icon(
          onPressed: locked ? _promptPin : _save,
          icon: Icon(locked ? Icons.lock : Icons.save),
          label: Text(locked ? 'Unlock to save' : 'Save to device'),
        ),
      ],
    );
  }

  Widget _section(String title) => Padding(
        padding: const EdgeInsets.only(top: 16, bottom: 4),
        child: Text(title, style: Theme.of(context).textTheme.titleSmall),
      );

  Widget _field(String label, TextEditingController ctrl) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: TextField(
        controller: ctrl,
        enabled: _authed,
        keyboardType: const TextInputType.numberWithOptions(decimal: true),
        decoration: InputDecoration(labelText: label, isDense: true),
      ),
    );
  }

  Widget _dropdown(
      String label, String value, List<String> items, ValueChanged<String?> onChanged) {
    return DropdownButtonFormField<String>(
      initialValue: value,
      decoration: InputDecoration(labelText: label, isDense: true),
      items: [for (final i in items) DropdownMenuItem(value: i, child: Text(i))],
      onChanged: _authed ? onChanged : null,
    );
  }
}
