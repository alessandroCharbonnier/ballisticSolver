import 'dart:async';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:material_ui/material_ui.dart';

import '../models/shot.dart';
import '../radar/radar.dart';

/// FX True Ballistic radar: scan, connect, and log shots live.
class RadarScreen extends StatefulWidget {
  const RadarScreen({super.key});

  @override
  State<RadarScreen> createState() => _RadarScreenState();
}

class _RadarScreenState extends State<RadarScreen> {
  RadarClient? _client;
  bool _scanning = false;
  bool _connecting = false;
  String? _error;

  final List<ShotRecord> _shots = [];
  ShotRecord? _latest;
  StreamSubscription<ShotRecord>? _sub;

  @override
  void initState() {
    super.initState();
    _startScan();
  }

  Future<void> _startScan() async {
    setState(() {
      _scanning = true;
      _error = null;
    });
    try {
      if (FlutterBluePlus.adapterStateNow != BluetoothAdapterState.on) {
        await FlutterBluePlus.turnOn();
      }
      await FlutterBluePlus.startScan(timeout: const Duration(seconds: 15));
    } catch (e) {
      setState(() => _error = 'Scan failed: $e');
    } finally {
      if (mounted) setState(() => _scanning = false);
    }
  }

  Future<void> _connect(BluetoothDevice device) async {
    await FlutterBluePlus.stopScan();
    setState(() {
      _connecting = true;
      _error = null;
    });
    try {
      final c = RadarClient(device);
      await c.connectAndListen();
      _sub = c.shots.listen((s) {
        if (!mounted) return;
        setState(() {
          _shots.insert(0, s);
          _latest = s;
          if (_shots.length > 100) _shots.removeLast();
        });
      });
      if (!mounted) return;
      setState(() {
        _client = c;
        _connecting = false;
      });
    } catch (e) {
      if (!mounted) return;
      setState(() {
        _connecting = false;
        _error = 'Connect failed: $e';
      });
    }
  }

  Future<void> _disconnect() async {
    await _sub?.cancel();
    await _client?.dispose();
    if (!mounted) return;
    setState(() {
      _client = null;
      _shots.clear();
      _latest = null;
    });
    _startScan();
  }

  @override
  void dispose() {
    _sub?.cancel();
    _client?.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('FX Radar'),
        actions: [
          if (_client != null)
            IconButton(
              tooltip: 'Disconnect',
              icon: const Icon(Icons.link_off),
              onPressed: _disconnect,
            ),
        ],
      ),
      body: _client != null ? _buildLive() : _buildScan(),
    );
  }

  Widget _buildScan() {
    if (_connecting) {
      return const Center(child: CircularProgressIndicator());
    }
    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.all(16),
          child: FilledButton.icon(
            onPressed: _scanning ? null : _startScan,
            icon: const Icon(Icons.bluetooth_searching),
            label: Text(_scanning ? 'Scanning…' : 'Scan for radar'),
          ),
        ),
        if (_error != null)
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16),
            child: Text(_error!, style: TextStyle(color: Colors.red.shade300)),
          ),
        Expanded(
          child: StreamBuilder<List<ScanResult>>(
            stream: FlutterBluePlus.scanResults,
            builder: (context, snapshot) {
              final results =
                  snapshot.data ?? FlutterBluePlus.lastScanResults;
              final radars = results
                  .where((r) =>
                      r.advertisementData.advName == Radar.name ||
                      r.device.platformName == Radar.name ||
                      r.device.remoteId.str.toLowerCase() == Radar.mac)
                  .toList();

              if (radars.isEmpty) {
                return const Center(
                  child: Text('No radar found.\nPower on the FX radar.'),
                );
              }
              return ListView.builder(
                itemCount: radars.length,
                itemBuilder: (context, i) {
                  final r = radars[i];
                  return ListTile(
                    leading: const Icon(Icons.radar),
                    title: const Text('FX True Ballistic'),
                    subtitle: Text('${r.rssi} dBm · ${r.device.remoteId}'),
                    trailing: const Icon(Icons.chevron_right),
                    onTap: () => _connect(r.device),
                  );
                },
              );
            },
          ),
        ),
      ],
    );
  }

  Widget _buildLive() {
    final latest = _latest;
    return Column(
      children: [
        Container(
          width: double.infinity,
          margin: const EdgeInsets.all(16),
          padding: const EdgeInsets.all(24),
          decoration: BoxDecoration(
            color: Colors.green.shade900,
            borderRadius: BorderRadius.circular(12),
          ),
          child: Column(
            children: [
              const Text('LAST SHOT', style: TextStyle(color: Colors.white70)),
              const SizedBox(height: 8),
              Text(
                latest == null
                    ? '—'
                    : '${latest.velocityFps.toStringAsFixed(1)} fps',
                style: const TextStyle(
                  fontSize: 40,
                  fontWeight: FontWeight.bold,
                  color: Colors.white,
                ),
              ),
              if (latest != null)
                Text(
                  '${latest.velocityMps.toStringAsFixed(1)} m/s · BC ${latest.ballisticCoefficient.toStringAsFixed(4)}',
                  style: const TextStyle(color: Colors.white70),
                ),
            ],
          ),
        ),
        Row(
          children: [
            const SizedBox(width: 16),
            Text('Shots (${_shots.length})',
                style: Theme.of(context).textTheme.titleMedium),
            const Spacer(),
            TextButton(
              onPressed: () => setState(_shots.clear),
              child: const Text('Clear'),
            ),
            const SizedBox(width: 8),
          ],
        ),
        Expanded(
          child: _shots.isEmpty
              ? const Center(child: Text('Fire a shot to log it'))
              : ListView.builder(
                  itemCount: _shots.length,
                  itemBuilder: (context, i) {
                    final s = _shots[i];
                    return ListTile(
                      dense: true,
                      leading: const Icon(Icons.speed),
                      title: Text(
                          '${s.velocityFps.toStringAsFixed(1)} fps · ${s.velocityMps.toStringAsFixed(1)} m/s'),
                      subtitle: Text(
                          'BC ${s.ballisticCoefficient.toStringAsFixed(4)} · ${_fmt(s.timestamp)}'),
                    );
                  },
                ),
        ),
      ],
    );
  }

  String _fmt(DateTime t) {
    String p(int n) => n.toString().padLeft(2, '0');
    return '${p(t.hour)}:${p(t.minute)}:${p(t.second)}';
  }
}
