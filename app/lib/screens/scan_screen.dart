import 'package:material_ui/material_ui.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../ble/gatt.dart';
import 'device_screen.dart';

/// Device discovery: scan for the BoreCore One, then connect.
class ScanScreen extends StatefulWidget {
  const ScanScreen({super.key});

  @override
  State<ScanScreen> createState() => _ScanScreenState();
}

class _ScanScreenState extends State<ScanScreen> {
  bool _scanning = false;
  String? _error;

  @override
  void initState() {
    super.initState();
    _ensureOn();
  }

  Future<void> _ensureOn() async {
    try {
      if (FlutterBluePlus.adapterStateNow != BluetoothAdapterState.on) {
        await FlutterBluePlus.turnOn();
      }
    } catch (e) {
      setState(() => _error = 'Bluetooth: $e');
    }
  }

  Future<void> _scan() async {
    setState(() {
      _scanning = true;
      _error = null;
    });
    try {
      await FlutterBluePlus.startScan(timeout: const Duration(seconds: 15));
    } catch (e) {
      setState(() => _error = 'Scan failed: $e');
    } finally {
      if (mounted) setState(() => _scanning = false);
    }
  }

  Future<void> _open(ScanResult r) async {
    await FlutterBluePlus.stopScan();
    if (!mounted) return;
    Navigator.of(context).push(MaterialPageRoute(
      builder: (_) => DeviceScreen(device: r.device),
    ));
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('BoreCore One')),
      body: Column(
        children: [
          Padding(
            padding: const EdgeInsets.all(16),
            child: FilledButton.icon(
              onPressed: _scanning ? null : _scan,
              icon: const Icon(Icons.bluetooth_searching),
              label: Text(_scanning ? 'Scanning…' : 'Scan'),
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
                final results = snapshot.data ?? FlutterBluePlus.lastScanResults;
                final devices = results
                    .where((r) =>
                        r.advertisementData.advName
                            .contains(Gatt.deviceName) ||
                        r.device.platformName.contains(Gatt.deviceName))
                    .toList();

                if (devices.isEmpty) {
                  return const Center(child: Text('No device found.\nTurn on BT on the device (menu → BT).'));
                }

                return ListView.builder(
                  itemCount: devices.length,
                  itemBuilder: (context, i) {
                    final r = devices[i];
                    final name = r.advertisementData.advName.isNotEmpty
                        ? r.advertisementData.advName
                        : r.device.platformName;
                    return ListTile(
                      leading: const Icon(Icons.bluetooth),
                      title: Text(name),
                      subtitle: Text('${r.rssi} dBm · ${r.device.remoteId}'),
                      trailing: const Icon(Icons.chevron_right),
                      onTap: () => _open(r),
                    );
                  },
                );
              },
            ),
          ),
        ],
      ),
    );
  }
}
