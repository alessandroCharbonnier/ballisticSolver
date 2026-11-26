import 'package:file_picker/file_picker.dart';
import 'package:material_ui/material_ui.dart';
import 'package:share_plus/share_plus.dart';

import '../app/app_scope.dart';
import '../controllers/data_controller.dart';
import '../data/session_repository.dart';
import '../widgets/simulator_badge.dart';
import 'session_detail_screen.dart';

/// Data screen (section 11.5): export, backup, import and the session list.
class DataScreen extends StatefulWidget {
  const DataScreen({super.key});

  @override
  State<DataScreen> createState() => _DataScreenState();
}

class _DataScreenState extends State<DataScreen> {
  DataController? _controller;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    if (_controller == null) {
      _controller = DataController(AppScope.of(context));
      _controller!.load();
    }
  }

  @override
  void dispose() {
    _controller?.dispose();
    super.dispose();
  }

  Future<void> _share(String path) async {
    await SharePlus.instance.share(ShareParams(files: [XFile(path)]));
  }

  Future<void> _exportAll() async {
    try {
      final path = await _controller!.exportAllCsv();
      if (mounted) await _share(path);
    } catch (e) {
      _snack('Export failed: $e');
    }
  }

  Future<void> _exportBackup() async {
    try {
      final path = await _controller!.exportBackup();
      if (mounted) await _share(path);
    } catch (e) {
      _snack('Export failed: $e');
    }
  }

  Future<void> _import() async {
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Import backup?'),
        content: const Text(
            'This replaces all data in the app with the content of the '
            'backup. A backup of the current data is made first.'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx, false),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('Import'),
          ),
        ],
      ),
    );
    if (confirmed != true || !mounted) return;

    final files = await FilePicker.pickFiles();
    final path = files.isEmpty ? null : files.first.path;
    if (path == null) return;

    final error = await _controller!.validateBackup(path);
    if (error != null) {
      _snack(error);
      return;
    }

    await _controller!.importBackup(path);
    if (!mounted) return;
    _snack('Backup imported.');
    Navigator.of(context).popUntil((route) => route.isFirst);
  }

  void _snack(String message) {
    ScaffoldMessenger.of(context)
        .showSnackBar(SnackBar(content: Text(message)));
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Data'),
        actions: const [SimulatorBadge()],
      ),
      body: ListenableBuilder(
        listenable: _controller!,
        builder: (context, _) {
          final controller = _controller!;
          if (controller.loading) {
            return const Center(child: CircularProgressIndicator());
          }
          return Center(
            child: ConstrainedBox(
              constraints: const BoxConstraints(maxWidth: 900),
              child: Column(
                children: [
                  Padding(
                    padding: const EdgeInsets.all(16),
                    child: Wrap(
                      spacing: 12,
                      runSpacing: 12,
                      children: [
                        FilledButton.icon(
                          onPressed: _exportAll,
                          icon: const Icon(Icons.file_download),
                          label: const Text('Export all shots (CSV)'),
                        ),
                        FilledButton.icon(
                          onPressed: _exportBackup,
                          icon: const Icon(Icons.backup),
                          label: const Text('Export full backup'),
                        ),
                        OutlinedButton.icon(
                          onPressed: _import,
                          icon: const Icon(Icons.file_upload),
                          label: const Text('Import backup'),
                        ),
                      ],
                    ),
                  ),
                  Expanded(
                    child: controller.sessions.isEmpty
                        ? const Center(child: Text('No sessions yet.'))
                        : ListView.builder(
                            itemCount: controller.sessions.length,
                            itemBuilder: (context, i) =>
                                _sessionTile(controller.sessions[i]),
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

  Widget _sessionTile(SessionSummary summary) {
    final started = summary.session.startedAt;
    return ListTile(
      contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      leading: const Icon(Icons.calendar_today),
      title: Text(_format(started)),
      subtitle: Text(
        'Shots ${summary.shotCount}  -  unassigned '
        '${summary.unassignedCount}  -  '
        'barrel ${summary.session.barrelCleaned ? 'cleaned' : 'not cleaned'}',
      ),
      trailing: const Icon(Icons.chevron_right),
      onTap: () => Navigator.of(context).push(
        MaterialPageRoute(
          builder: (_) => SessionDetailScreen(sessionId: summary.session.id!),
        ),
      ),
    );
  }

  String _format(DateTime? time) {
    if (time == null) return '';
    final local = time.toLocal();
    String two(int n) => n.toString().padLeft(2, '0');
    return '${local.year}-${two(local.month)}-${two(local.day)} '
        '${two(local.hour)}:${two(local.minute)}';
  }
}
