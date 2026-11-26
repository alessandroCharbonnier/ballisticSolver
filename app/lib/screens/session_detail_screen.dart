import 'package:flutter/services.dart';
import 'package:material_ui/material_ui.dart';
import 'package:share_plus/share_plus.dart';

import '../app/app_scope.dart';
import '../controllers/session_detail_controller.dart';
import '../data/shot_repository.dart';
import '../domain/decimal_input.dart';
import '../widgets/simulator_badge.dart';

/// Session detail (section 11.5): the shot table and two actions.
class SessionDetailScreen extends StatefulWidget {
  const SessionDetailScreen({super.key, required this.sessionId});

  final int sessionId;

  @override
  State<SessionDetailScreen> createState() => _SessionDetailScreenState();
}

class _SessionDetailScreenState extends State<SessionDetailScreen> {
  SessionDetailController? _controller;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    if (_controller == null) {
      _controller =
          SessionDetailController(AppScope.of(context), widget.sessionId);
      _controller!.load();
    }
  }

  @override
  void dispose() {
    _controller?.dispose();
    super.dispose();
  }

  Future<void> _export() async {
    try {
      final path = await _controller!.exportCsv();
      if (mounted) {
        await SharePlus.instance.share(ShareParams(files: [XFile(path)]));
      }
    } catch (e) {
      _snack('Export failed: $e');
    }
  }

  Future<void> _setWeather() async {
    final tempCtrl = TextEditingController();
    final pressureCtrl = TextEditingController();
    final humidityCtrl = TextEditingController();

    final saved = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Set weather for shots without data'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(
              controller: tempCtrl,
              autofocus: true,
              keyboardType:
                  const TextInputType.numberWithOptions(decimal: true),
              inputFormatters: [
                FilteringTextInputFormatter.allow(RegExp(r'[0-9.,]')),
              ],
              decoration: const InputDecoration(labelText: 'Temperature (C)'),
            ),
            TextField(
              controller: pressureCtrl,
              keyboardType:
                  const TextInputType.numberWithOptions(decimal: true),
              inputFormatters: [
                FilteringTextInputFormatter.allow(RegExp(r'[0-9.,]')),
              ],
              decoration: const InputDecoration(labelText: 'Pressure (hPa)'),
            ),
            TextField(
              controller: humidityCtrl,
              keyboardType:
                  const TextInputType.numberWithOptions(decimal: true),
              inputFormatters: [
                FilteringTextInputFormatter.allow(RegExp(r'[0-9.,]')),
              ],
              decoration: const InputDecoration(labelText: 'Humidity (%)'),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx, false),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('Save'),
          ),
        ],
      ),
    );

    if (saved != true || !mounted) return;

    final temp = parseDecimal(tempCtrl.text);
    if (temp == null) {
      _snack('Temperature is required.');
      return;
    }
    final pressure = parseDecimal(pressureCtrl.text);
    final humidity = parseDecimal(humidityCtrl.text);

    final updated = await _controller!.setWeatherForShotsWithoutData(
      tempC: temp,
      pressureHpa: pressure,
      humidityPct: humidity,
    );
    if (mounted) _snack('Updated $updated shots.');
  }

  void _snack(String message) {
    ScaffoldMessenger.of(context)
        .showSnackBar(SnackBar(content: Text(message)));
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Session'),
        actions: const [SimulatorBadge()],
      ),
      body: ListenableBuilder(
        listenable: _controller!,
        builder: (context, _) {
          final controller = _controller!;
          if (controller.loading) {
            return const Center(child: CircularProgressIndicator());
          }
          final shots = controller.shots;
          return Center(
            child: ConstrainedBox(
              constraints: const BoxConstraints(maxWidth: 1000),
              child: Column(
                children: [
                  Padding(
                    padding: const EdgeInsets.all(16),
                    child: Wrap(
                      spacing: 12,
                      runSpacing: 12,
                      children: [
                        FilledButton.icon(
                          onPressed: _export,
                          icon: const Icon(Icons.file_download),
                          label: const Text('Export this session (CSV)'),
                        ),
                        OutlinedButton.icon(
                          onPressed: _setWeather,
                          icon: const Icon(Icons.thermostat),
                          label:
                              const Text('Set weather for shots without data'),
                        ),
                      ],
                    ),
                  ),
                  Expanded(
                    child: SingleChildScrollView(
                      scrollDirection: Axis.horizontal,
                      child: DataTable(
                        columns: const [
                          DataColumn(label: Text('Index')),
                          DataColumn(label: Text('Time')),
                          DataColumn(label: Text('Cartridge')),
                          DataColumn(label: Text('Velocity (m/s)')),
                          DataColumn(label: Text('BC')),
                          DataColumn(label: Text('Temp (C)')),
                          DataColumn(label: Text('Note')),
                        ],
                        rows: [
                          for (final detail in shots)
                            DataRow(cells: [
                              DataCell(Text('${detail.shot.shotIndex}')),
                              DataCell(Text(_time(detail.shot.firedAt))),
                              DataCell(Text(_cartridgeLabel(detail))),
                              DataCell(Text(detail.shot.velocityMps
                                  .toStringAsFixed(1))),
                              DataCell(Text(detail.shot.bc.toStringAsFixed(4))),
                              DataCell(Text(detail.shot.tempC == null
                                  ? ''
                                  : detail.shot.tempC!.toStringAsFixed(1))),
                              DataCell(Text(_note(detail))),
                            ]),
                        ],
                      ),
                    ),
                  ),
                ],
              ),
            ),
          );
        },
      ),
    );
  }

  String _cartridgeLabel(ShotDetail detail) {
    if (detail.shot.cartridgeId == null) return 'unassigned';
    return detail.cartridgeLabel ?? '?';
  }

  String _note(ShotDetail detail) {
    if (detail.shot.voided) return 'voided';
    if (detail.shot.cartridgeId == null) return 'unassigned';
    return '';
  }

  String _time(DateTime? time) {
    if (time == null) return '';
    final local = time.toLocal();
    String two(int n) => n.toString().padLeft(2, '0');
    return '${two(local.hour)}:${two(local.minute)}:${two(local.second)}';
  }
}