import 'package:material_ui/material_ui.dart';

import 'radar_screen.dart';
import 'scan_screen.dart';

/// Top-level hub: pick the ballistic device or the FX radar.
class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Ballistic Sentinel')),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          _card(
            context,
            icon: Icons.bluetooth,
            title: 'Ballistic Sentinel',
            subtitle: 'Connect to the device: config, live dashboard',
            onTap: () => Navigator.of(context).push(
              MaterialPageRoute(builder: (_) => const ScanScreen()),
            ),
          ),
          _card(
            context,
            icon: Icons.radar,
            title: 'FX True Ballistic radar',
            subtitle: 'Capture shots: velocity + BC',
            onTap: () => Navigator.of(context).push(
              MaterialPageRoute(builder: (_) => const RadarScreen()),
            ),
          ),
        ],
      ),
    );
  }

  Widget _card(
    BuildContext context, {
    required IconData icon,
    required String title,
    required String subtitle,
    required VoidCallback onTap,
  }) {
    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      child: ListTile(
        contentPadding: const EdgeInsets.all(16),
        leading: Icon(icon, size: 40),
        title: Text(title, style: Theme.of(context).textTheme.titleLarge),
        subtitle: Text(subtitle),
        trailing: const Icon(Icons.chevron_right),
        onTap: onTap,
      ),
    );
  }
}
