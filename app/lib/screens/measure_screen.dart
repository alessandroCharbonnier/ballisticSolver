import 'package:material_ui/material_ui.dart';
import 'package:flutter/services.dart';

import '../app/app_scope.dart';
import '../controllers/measure_controller.dart';
import '../domain/decimal_input.dart';
import '../domain/models.dart';
import '../domain/slots.dart';
import '../widgets/box_grid.dart';
import '../widgets/simulator_badge.dart';

/// Measure screen (section 11.3): the box grid plus the entry dialog.
class MeasureScreen extends StatefulWidget {
  const MeasureScreen({super.key, required this.boxId});

  final int boxId;

  @override
  State<MeasureScreen> createState() => _MeasureScreenState();
}

class _MeasureScreenState extends State<MeasureScreen> {
  MeasureController? _controller;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    if (_controller == null) {
      _controller = MeasureController(AppScope.of(context), widget.boxId);
      _controller!.load();
    }
  }

  @override
  void dispose() {
    _controller?.dispose();
    super.dispose();
  }

  void _openDialog(Cartridge cartridge) {
    showDialog<void>(
      context: context,
      barrierDismissible: false,
      builder: (_) => MeasureEntryDialog(
        controller: _controller!,
        initial: cartridge,
      ),
    );
  }

  void _measureNext() {
    final next = _controller!.nextEmptyCartridge();
    if (next != null) _openDialog(next);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: ListenableBuilder(
          listenable: _controller!,
          builder: (context, _) => Text(
            _controller!.box == null
                ? 'Measure'
                : 'Measure ${_controller!.boxCode}',
          ),
        ),
        actions: const [SimulatorBadge()],
      ),
      body: ListenableBuilder(
        listenable: _controller!,
        builder: (context, _) {
          final controller = _controller!;
          if (controller.loading) {
            return const Center(child: CircularProgressIndicator());
          }
          final isLandscape =
              MediaQuery.of(context).size.width >
              MediaQuery.of(context).size.height;

          final grid = BoxGrid(
            rows: controller.product!.rows,
            cols: controller.product!.cols,
            hingeEdge: controller.product!.hingeEdge,
            slots: controller.buildSlots(
              (cartridge) => _openDialog(cartridge),
            ),
          );

          final info = _buildInfo(controller);

          return SingleChildScrollView(
            padding: const EdgeInsets.all(16),
            child: isLandscape
                ? Row(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Expanded(flex: 2, child: grid),
                      const SizedBox(width: 16),
                      Expanded(flex: 1, child: info),
                    ],
                  )
                : Column(
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [grid, const SizedBox(height: 16), info],
                  ),
          );
        },
      ),
    );
  }

  Widget _buildInfo(MeasureController controller) {
    final measured = controller.cartridges
        .where((c) => c.isMeasured)
        .length;
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Text(
          controller.product!.name,
          style: Theme.of(context).textTheme.titleLarge,
        ),
        Text('Lot ${controller.lot!.lotNumber}'),
        Text('Measured $measured/${controller.cartridges.length}'),
        const SizedBox(height: 16),
        FilledButton.icon(
          onPressed: controller.nextEmptyCartridge() == null ? null : _measureNext,
          icon: const Icon(Icons.straighten),
          label: const Text('Measure next'),
        ),
      ],
    );
  }
}

/// Entry dialog for one slot (section 11.3). Stays open after Save and moves
/// to the next empty slot.
class MeasureEntryDialog extends StatefulWidget {
  const MeasureEntryDialog({
    super.key,
    required this.controller,
    required this.initial,
  });

  final MeasureController controller;
  final Cartridge initial;

  @override
  State<MeasureEntryDialog> createState() => _MeasureEntryDialogState();
}

class _MeasureEntryDialogState extends State<MeasureEntryDialog> {
  late Cartridge _current;
  final _weightCtrl = TextEditingController();
  final _rimCtrl = TextEditingController();
  final _weightFocus = FocusNode();

  bool _saving = false;
  bool _saveAnyway = false;
  List<String> _warnings = const [];

  @override
  void initState() {
    super.initState();
    _current = widget.initial;
    _fill(widget.initial);
  }

  @override
  void dispose() {
    _weightCtrl.dispose();
    _rimCtrl.dispose();
    _weightFocus.dispose();
    super.dispose();
  }

  void _fill(Cartridge cartridge) {
    _weightCtrl.text = cartridge.weightG?.toString() ?? '';
    _rimCtrl.text = cartridge.rimThicknessMm?.toString() ?? '';
  }

  String get _title {
    final cols = widget.controller.cols;
    return 'Slot ${_current.slot} - ${SlotMath.positionLabel(_current.slot, cols)}';
  }

  Future<void> _save() async {
    final weight = parseDecimal(_weightCtrl.text);
    final rim = parseDecimal(_rimCtrl.text);
    if (weight == null || rim == null) {
      setState(() => _warnings = const ['Enter a number for both values.']);
      return;
    }

    setState(() => _saving = true);
    final warnings = await widget.controller.warningsFor(
      _current,
      weightG: weight,
      rimThicknessMm: rim,
    );

    // M4: first Save with warnings only shows them; the second press saves.
    if (warnings.isNotEmpty && !_saveAnyway) {
      setState(() {
        _saving = false;
        _warnings = warnings;
        _saveAnyway = true;
      });
      return;
    }

    await widget.controller.save(
      _current,
      weightG: weight,
      rimThicknessMm: rim,
    );
    if (!mounted) return;

    final next = widget.controller.nextEmptyCartridge();
    if (next == null) {
      ScaffoldMessenger.of(context)
          .showSnackBar(const SnackBar(content: Text('Box complete')));
      Navigator.of(context).pop();
      return;
    }

    setState(() {
      _current = next;
      _fill(next);
      _warnings = const [];
      _saveAnyway = false;
      _saving = false;
    });
    _weightFocus.requestFocus();
  }

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: Text(_title),
      content: SizedBox(
        width: 360,
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(
              controller: _weightCtrl,
              focusNode: _weightFocus,
              autofocus: true,
              keyboardType: const TextInputType.numberWithOptions(decimal: true),
              inputFormatters: [
                FilteringTextInputFormatter.allow(RegExp(r'[0-9.,]')),
              ],
              decoration: const InputDecoration(labelText: 'Weight (g)'),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: _rimCtrl,
              keyboardType: const TextInputType.numberWithOptions(decimal: true),
              inputFormatters: [
                FilteringTextInputFormatter.allow(RegExp(r'[0-9.,]')),
              ],
              decoration:
                  const InputDecoration(labelText: 'Rim thickness (mm)'),
            ),
            if (_warnings.isNotEmpty) ...[
              const SizedBox(height: 12),
              for (final warning in _warnings)
                Padding(
                  padding: const EdgeInsets.only(top: 4),
                  child: Text(
                    warning,
                    style: TextStyle(color: Colors.orange.shade300),
                  ),
                ),
            ],
          ],
        ),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.of(context).pop(),
          child: const Text('Close'),
        ),
        FilledButton(
          onPressed: _saving ? null : _save,
          child: Text(_saveAnyway ? 'Save anyway' : 'Save'),
        ),
      ],
    );
  }
}
