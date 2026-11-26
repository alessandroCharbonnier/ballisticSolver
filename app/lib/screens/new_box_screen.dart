import 'package:material_ui/material_ui.dart';

import '../app/app_scope.dart';
import '../controllers/boxes_controller.dart';
import '../widgets/simulator_badge.dart';

/// "New box" form (section 11.2): product (choose or type), lot (choose or
/// type), box code, and — for a new product — rows, columns and hinge edge.
class NewBoxScreen extends StatefulWidget {
  const NewBoxScreen({super.key});

  @override
  State<NewBoxScreen> createState() => _NewBoxScreenState();
}

class _NewBoxScreenState extends State<NewBoxScreen> {
  BoxesController? _controller;

  static const String _newValue = '__new__';
  static const _hingeEdges = ['top', 'bottom', 'left', 'right'];

  final _newProductCtrl = TextEditingController();
  final _newLotCtrl = TextEditingController();
  final _codeCtrl = TextEditingController();
  final _rowsCtrl = TextEditingController(text: '5');
  final _colsCtrl = TextEditingController(text: '10');

  String _productChoice = _newValue;
  String _lotChoice = _newValue;
  String _hingeEdge = 'top';

  List<String> _productNames = [];
  List<String> _lotNumbers = [];
  bool _ready = false;
  bool _saving = false;

  bool get _isNewProduct => _productChoice == _newValue;
  bool get _isNewLot => _isNewProduct || _lotChoice == _newValue;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    if (_controller == null) {
      _controller = BoxesController(AppScope.of(context));
      _load();
    }
  }

  @override
  void dispose() {
    _newProductCtrl.dispose();
    _newLotCtrl.dispose();
    _codeCtrl.dispose();
    _rowsCtrl.dispose();
    _colsCtrl.dispose();
    _controller?.dispose();
    super.dispose();
  }

  Future<void> _load() async {
    final controller = _controller!;
    final names = await controller.productNames();
    final lastProduct = await controller.lastProductName();
    final lastLot = await controller.lastLotNumber();
    final code = await controller.nextFreeBoxCode();
    if (!mounted) return;
    setState(() {
      _productNames = names;
      _productChoice =
          lastProduct != null && names.contains(lastProduct)
              ? lastProduct
              : _newValue;
      _newLotCtrl.text = lastLot ?? '';
      _codeCtrl.text = code;
      _ready = true;
    });
    await _refreshLots();
  }

  Future<void> _refreshLots() async {
    final controller = _controller;
    if (controller == null) return;
    if (_isNewProduct) {
      if (mounted) setState(() => _lotNumbers = []);
      return;
    }
    final numbers = await controller.lotNumbersFor(_productChoice);
    if (!mounted) return;
    setState(() {
      _lotNumbers = numbers;
      if (_lotChoice != _newValue && !numbers.contains(_lotChoice)) {
        _lotChoice = _newValue;
      }
    });
  }

  Future<void> _save() async {
    final controller = _controller!;
    final product = _isNewProduct
        ? _newProductCtrl.text.trim()
        : _productChoice.trim();
    final lot = _isNewLot ? _newLotCtrl.text.trim() : _lotChoice.trim();
    final code = _codeCtrl.text.trim();

    if (product.isEmpty) {
      _show('Type a product name.');
      return;
    }
    if (lot.isEmpty) {
      _show('Type a lot number.');
      return;
    }
    if (code.isEmpty) {
      _show('Type a box code.');
      return;
    }

    var rows = 5;
    var cols = 10;
    if (_isNewProduct) {
      rows = int.tryParse(_rowsCtrl.text.trim()) ?? 0;
      cols = int.tryParse(_colsCtrl.text.trim()) ?? 0;
      if (rows < 1 || cols < 1) {
        _show('Rows and columns must be at least 1.');
        return;
      }
    }

    if (await controller.boxCodeExists(code)) {
      _show('Box code $code already exists.');
      return;
    }

    setState(() => _saving = true);
    try {
      final boxId = await controller.createBox(
        productName: product,
        lotNumber: lot,
        code: code,
        rows: rows,
        cols: cols,
        hingeEdge: _hingeEdge,
      );
      if (mounted) Navigator.of(context).pop(boxId);
    } catch (e) {
      if (mounted) {
        setState(() => _saving = false);
        _show('Could not create the box: $e');
      }
    }
  }

  void _show(String message) {
    ScaffoldMessenger.of(context)
        .showSnackBar(SnackBar(content: Text(message)));
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('New box'), actions: const [SimulatorBadge()]),
      body: Center(
        child: ConstrainedBox(
          constraints: const BoxConstraints(maxWidth: 600),
          child: ListView(
            padding: const EdgeInsets.all(16),
            children: [
              if (!_ready)
                const Center(child: CircularProgressIndicator())
              else ...[
                DropdownButtonFormField<String>(
                  initialValue: _productChoice,
                  decoration: const InputDecoration(labelText: 'Product'),
                  items: [
                    for (final name in _productNames)
                      DropdownMenuItem(value: name, child: Text(name)),
                    const DropdownMenuItem(
                      value: _newValue,
                      child: Text('+ New product...'),
                    ),
                  ],
                  onChanged: (value) {
                    setState(() => _productChoice = value ?? _newValue);
                    _refreshLots();
                  },
                ),
                if (_isNewProduct) ...[
                  const SizedBox(height: 16),
                  TextField(
                    controller: _newProductCtrl,
                    decoration: const InputDecoration(labelText: 'Product name'),
                  ),
                  const SizedBox(height: 16),
                  Row(
                    children: [
                      Expanded(
                        child: TextField(
                          controller: _rowsCtrl,
                          keyboardType: TextInputType.number,
                          decoration: const InputDecoration(labelText: 'Rows'),
                        ),
                      ),
                      const SizedBox(width: 16),
                      Expanded(
                        child: TextField(
                          controller: _colsCtrl,
                          keyboardType: TextInputType.number,
                          decoration:
                              const InputDecoration(labelText: 'Columns'),
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 16),
                  DropdownButtonFormField<String>(
                    initialValue: _hingeEdge,
                    decoration: const InputDecoration(labelText: 'Hinge edge'),
                    items: [
                      for (final edge in _hingeEdges)
                        DropdownMenuItem(value: edge, child: Text(edge)),
                    ],
                    onChanged: (value) =>
                        setState(() => _hingeEdge = value ?? 'top'),
                  ),
                ],
                const SizedBox(height: 16),
                if (!_isNewProduct)
                  DropdownButtonFormField<String>(
                    initialValue: _lotChoice,
                    decoration: const InputDecoration(labelText: 'Lot number'),
                    items: [
                      for (final number in _lotNumbers)
                        DropdownMenuItem(value: number, child: Text(number)),
                      const DropdownMenuItem(
                        value: _newValue,
                        child: Text('+ New lot...'),
                      ),
                    ],
                    onChanged: (value) =>
                        setState(() => _lotChoice = value ?? _newValue),
                  )
                else ...[
                  const SizedBox(height: 16),
                  TextField(
                    controller: _newLotCtrl,
                    decoration: const InputDecoration(labelText: 'Lot number'),
                  ),
                ],
                const SizedBox(height: 16),
                TextField(
                  controller: _codeCtrl,
                  decoration: const InputDecoration(
                    labelText: 'Box code',
                    helperText: 'Write this code on the real box',
                  ),
                ),
                const SizedBox(height: 24),
                FilledButton(
                  onPressed: _saving ? null : _save,
                  child: Text(_saving ? 'Saving...' : 'Create box'),
                ),
              ],
            ],
          ),
        ),
      ),
    );
  }
}
