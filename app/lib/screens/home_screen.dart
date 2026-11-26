import 'package:material_ui/material_ui.dart';

import '../widgets/simulator_badge.dart';
import 'boxes_screen.dart';
import 'data_screen.dart';
import 'fire_screen.dart';
import 'settings_screen.dart';

/// Top-level hub: Boxes, Fire, Data, with a gear to Settings.
class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Ballistic Sentinel'),
        actions: [
          const SimulatorBadge(),
          IconButton(
            tooltip: 'Settings',
            icon: const Icon(Icons.settings),
            onPressed: () => Navigator.of(context).push(
              MaterialPageRoute(builder: (_) => const SettingsScreen()),
            ),
          ),
        ],
      ),
      body: Center(
        child: ConstrainedBox(
          constraints: const BoxConstraints(maxWidth: 720),
          child: ListView(
            padding: const EdgeInsets.all(16),
            children: [
              _entry(
                context,
                icon: Icons.inventory_2_outlined,
                title: 'Boxes',
                subtitle: 'Measure cartridges, box by box',
                onTap: () => Navigator.of(context).push(
                  MaterialPageRoute(builder: (_) => const BoxesScreen()),
                ),
              ),
              _entry(
                context,
                icon: Icons.my_location,
                title: 'Fire',
                subtitle: 'Load a magazine and capture radar shots',
                onTap: () => Navigator.of(context).push(
                  MaterialPageRoute(builder: (_) => const FireScreen()),
                ),
              ),
              _entry(
                context,
                icon: Icons.storage,
                title: 'Data',
                subtitle: 'Review sessions, export CSV, backups',
                onTap: () => Navigator.of(context).push(
                  MaterialPageRoute(builder: (_) => const DataScreen()),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _entry(
    BuildContext context, {
    required IconData icon,
    required String title,
    required String subtitle,
    required VoidCallback onTap,
  }) {
    return Card(
      margin: const EdgeInsets.only(bottom: 16),
      child: ListTile(
        contentPadding: const EdgeInsets.symmetric(horizontal: 24, vertical: 20),
        leading: Icon(icon, size: 40),
        title: Text(title, style: Theme.of(context).textTheme.headlineSmall),
        subtitle: Text(subtitle),
        trailing: const Icon(Icons.chevron_right),
        onTap: onTap,
      ),
    );
  }
}
