import 'package:material_ui/material_ui.dart';

import '../app/app_scope.dart';
import '../controllers/boxes_controller.dart';
import '../data/boxes_repository.dart';
import '../widgets/simulator_badge.dart';
import 'measure_screen.dart';
import 'new_box_screen.dart';

/// Boxes list (section 11.2).
class BoxesScreen extends StatefulWidget {
  const BoxesScreen({super.key});

  @override
  State<BoxesScreen> createState() => _BoxesScreenState();
}

class _BoxesScreenState extends State<BoxesScreen> {
  BoxesController? _controller;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    if (_controller == null) {
      _controller = BoxesController(AppScope.of(context));
      _controller!.load();
    }
  }

  @override
  void dispose() {
    _controller?.dispose();
    super.dispose();
  }

  Future<void> _newBox() async {
    final controller = _controller!;
    final boxId = await Navigator.of(context).push<int>(
      MaterialPageRoute(builder: (_) => const NewBoxScreen()),
    );
    if (boxId != null && mounted) {
      await controller.load();
      if (!mounted) return;
      Navigator.of(context).push(
        MaterialPageRoute(builder: (_) => MeasureScreen(boxId: boxId)),
      );
    }
  }

  Future<void> _onLongPress(BoxListItem item) async {
    final hide = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text('Hide box ${item.box.code}?'),
        content: const Text('The data stays in the database.'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx, false),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('Hide box'),
          ),
        ],
      ),
    );
    if (hide == true && mounted) {
      final error = await _controller!.hideBox(item);
      if (error != null && mounted) {
        ScaffoldMessenger.of(context)
            .showSnackBar(SnackBar(content: Text(error)));
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Boxes'), actions: const [SimulatorBadge()]),
      floatingActionButton: FloatingActionButton.extended(
        onPressed: _newBox,
        icon: const Icon(Icons.add),
        label: const Text('New box'),
      ),
      body: ListenableBuilder(
        listenable: _controller!,
        builder: (context, _) {
          final controller = _controller!;
          if (controller.loading) {
            return const Center(child: CircularProgressIndicator());
          }
          if (controller.error != null) {
            return Center(child: Text(controller.error!));
          }
          if (controller.boxes.isEmpty) {
            return const Center(
              child: Text('No boxes yet.\nCreate one with "New box".'),
            );
          }
          return ListView.builder(
            itemCount: controller.boxes.length,
            itemBuilder: (context, i) {
              final item = controller.boxes[i];
              return ListTile(
                contentPadding:
                    const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                leading: const Icon(Icons.inventory_2_outlined),
                title: Text('${item.box.code}  -  ${item.productName}'),
                subtitle: Text(
                  'Lot ${item.lotNumber}  -  measured '
                  '${item.measuredCount}/${item.totalSlots}  -  '
                  'fired ${item.firedCount}',
                ),
                trailing: const Icon(Icons.chevron_right),
                onTap: () => Navigator.of(context).push(
                  MaterialPageRoute(
                    builder: (_) => MeasureScreen(boxId: item.box.id!),
                  ),
                ),
                onLongPress: () => _onLongPress(item),
              );
            },
          );
        },
      ),
    );
  }
}
