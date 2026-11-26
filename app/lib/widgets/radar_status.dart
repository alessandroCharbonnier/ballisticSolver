/// Radar link status (section 11.4).
library;

import 'package:material_ui/material_ui.dart';

import '../sources/radar_source.dart';

class RadarStatus extends StatelessWidget {
  const RadarStatus({super.key, required this.state});

  final RadarLinkState state;

  @override
  Widget build(BuildContext context) {
    final (label, color) = switch (state) {
      RadarLinkState.connected => ('Radar connected', Colors.green),
      RadarLinkState.connecting => ('Connecting to radar', Colors.amber),
      RadarLinkState.searching => ('Searching for radar', Colors.amber),
      RadarLinkState.disconnected => ('Radar disconnected', Colors.red),
    };
    return Row(
      children: [
        Container(
          width: 12,
          height: 12,
          decoration: BoxDecoration(color: color, shape: BoxShape.circle),
        ),
        const SizedBox(width: 8),
        Text(label),
      ],
    );
  }
}
