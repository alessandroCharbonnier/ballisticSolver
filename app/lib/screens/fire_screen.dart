import 'dart:async';

import 'package:flutter/services.dart';
import 'package:material_ui/material_ui.dart';
import 'package:wakelock_plus/wakelock_plus.dart';

import '../app/app_scope.dart';
import '../controllers/fire_controller.dart';
import '../data/shot_repository.dart';
import '../domain/decimal_input.dart';
import '../widgets/box_grid.dart';
import '../widgets/magazine_stack.dart';
import '../widgets/radar_status.dart';
import '../widgets/simulator_badge.dart';
import '../widgets/weather_chip.dart';

/// Fire screen (section 11.4).
class FireScreen extends StatefulWidget {
  const FireScreen({super.key});

  @override
  State<FireScreen> createState() => _FireScreenState();
}

class _FireScreenState extends State<FireScreen> {
  FireController? _controller;
  bool _cleaningDialogShown = false;
  bool _manualTempOfferShown = false;
  Timer? _ticker;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    if (_controller == null) {
      _controller = FireController(AppScope.of(context));
      _controller!.addListener(_onControllerChanged);
      _controller!.initialize();
      WakelockPlus.enable();
      // Refresh once a second so the weather line turns red 30 s after the
      // last reading (rule W2).
      _ticker = Timer.periodic(const Duration(seconds: 1), (_) {
        if (mounted) setState(() {});
      });
    }
  }

  @override
  void dispose() {
    _ticker?.cancel();
    _ticker = null;
    final controller = _controller;
    _controller = null;
    if (controller != null) {
      controller.removeListener(_onControllerChanged);
      unawaited(controller.leave());
      controller.dispose();
    }
    WakelockPlus.disable();
    super.dispose();
  }

  void _onControllerChanged() {
    final controller = _controller;
    if (controller == null || !mounted) return;

    if (controller.phase == FirePhase.askBarrelCleaned &&
        !_cleaningDialogShown) {
      _cleaningDialogShown = true;
      _askCleaning();
    }
    if (controller.offerManualTemp && !_manualTempOfferShown) {
      _manualTempOfferShown = true;
      _openManualTempDialog();
    }
  }

  Future<void> _askCleaning() async {
    final answer = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Barrel cleaned since last session?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx, false),
            child: const Text('No'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('Yes'),
          ),
        ],
      ),
    );
    if (!mounted) return;
    if (answer == null) {
      // S1: leaving the question without answering closes the Fire screen.
      Navigator.of(context).pop();
      return;
    }
    await _controller?.answerCleaning(answer);
  }

  Future<void> _openManualTempDialog() async {
    final ctrl = TextEditingController();
    final result = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('No weather dongle found'),
        content: TextField(
          controller: ctrl,
          autofocus: true,
          keyboardType: const TextInputType.numberWithOptions(decimal: true),
          inputFormatters: [
            FilteringTextInputFormatter.allow(RegExp(r'[0-9.,]')),
          ],
          decoration: const InputDecoration(labelText: 'Temperature (C)'),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx, false),
            child: const Text('Skip'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('Save'),
          ),
        ],
      ),
    );
    if (!mounted) return;
    if (result == true) {
      final value = parseDecimal(ctrl.text);
      if (value != null) {
        await _controller?.saveManualTemp(value);
      } else {
        _controller?.skipManualTemp();
      }
    } else {
      _controller?.skipManualTemp();
    }
    ctrl.dispose();
  }

  Future<void> _confirmUndo() async {
    final ok = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Cancel the last shot?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx, false),
            child: const Text('Keep it'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('Undo'),
          ),
        ],
      ),
    );
    if (ok != true || !mounted) return;
    final outcome = await _controller?.undo();
    if (!mounted) return;
    if (outcome != null) {
      if (outcome.status == UndoStatus.refusedMagazineFull) {
        _snack('Cannot undo: the magazine is full.');
      } else if (outcome.status == UndoStatus.nothingToUndo) {
        _snack('Nothing to undo.');
      }
    }
  }

  void _snack(String message) {
    ScaffoldMessenger.of(context)
        .showSnackBar(SnackBar(content: Text(message)));
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Fire'),
        actions: const [SimulatorBadge()],
      ),
      body: ListenableBuilder(
        listenable: _controller!,
        builder: (context, _) {
          final controller = _controller!;
          if (controller.phase != FirePhase.active) {
            return const Center(child: CircularProgressIndicator());
          }

          final isLandscape =
              MediaQuery.of(context).size.width >
              MediaQuery.of(context).size.height;

          final left = _buildLeft(controller);
          final right = _buildRight(controller);

          return SingleChildScrollView(
            padding: const EdgeInsets.all(16),
            child: isLandscape
                ? Row(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Expanded(flex: 2, child: left),
                      const SizedBox(width: 16),
                      Expanded(flex: 1, child: right),
                    ],
                  )
                : Column(
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [left, const SizedBox(height: 16), right],
                  ),
          );
        },
      ),
    );
  }

  Widget _buildLeft(FireController controller) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        DropdownButtonFormField<int>(
          initialValue: controller.selectedBox?.box.id,
          decoration: const InputDecoration(labelText: 'Box'),
          items: [
            for (final box in controller.boxes)
              DropdownMenuItem(
                value: box.box.id,
                child: Text('${box.box.code} - ${box.productName}'),
              ),
          ],
          onChanged: (boxId) {
            if (boxId == null) return;
            final box =
                controller.boxes.firstWhere((b) => b.box.id == boxId);
            controller.selectBox(box);
          },
        ),
        const SizedBox(height: 12),
        if (controller.product != null)
          BoxGrid(
            rows: controller.product!.rows,
            cols: controller.product!.cols,
            hingeEdge: controller.product!.hingeEdge,
            slots: controller.buildSlots(),
          ),
      ],
    );
  }

  Widget _buildRight(FireController controller) {
    final lastShot = controller.lastShot;
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        RadarStatus(state: controller.radarState),
        const SizedBox(height: 8),
        WeatherChip(
          reading: controller.weather.latest,
          isCurrent:
              controller.weather.isCurrent(controller.services.clock.now()),
          manualTemp: controller.manualTemp,
          onTap: _openManualTempDialog,
        ),
        const SizedBox(height: 12),
        Card(
          color: const Color(0xFF1B5E20),
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text('LAST SHOT',
                    style: TextStyle(color: Colors.white70, fontSize: 12)),
                Text(
                  lastShot == null
                      ? '—'
                      : '${lastShot.velocityMps.toStringAsFixed(1)} m/s',
                  style: const TextStyle(
                      fontSize: 32,
                      fontWeight: FontWeight.bold,
                      color: Colors.white),
                ),
                if (lastShot != null)
                  Text(
                    'BC ${lastShot.bc.toStringAsFixed(4)}  -  '
                    '${controller.lastShotAssigned ? controller.lastShotLabel! : 'unassigned'}',
                    style: const TextStyle(color: Colors.white70),
                  ),
              ],
            ),
          ),
        ),
        const SizedBox(height: 12),
        MagazineStack(
          entries: controller.magazine,
          onTap: () => controller.unloadTop(),
        ),
        const SizedBox(height: 8),
        Row(
          children: [
            TextButton.icon(
              onPressed: controller.magazine.isEmpty ? null : _confirmUndo,
              icon: const Icon(Icons.undo),
              label: const Text('Undo'),
            ),
            const Spacer(),
            if (controller.unassignedCount > 0)
              Text('Unassigned: ${controller.unassignedCount}',
                  style: const TextStyle(color: Colors.orange)),
          ],
        ),
        if (controller.simulatorOn) ...[
          const SizedBox(height: 12),
          FilledButton.icon(
            onPressed: controller.simulateShot,
            icon: const Icon(Icons.speed),
            label: const Text('Simulate shot'),
          ),
          SwitchListTile(
            contentPadding: EdgeInsets.zero,
            title: const Text('Dongle on'),
            value: controller.simDongleOn,
            onChanged: controller.setSimDongle,
          ),
        ],
      ],
    );
  }
}
