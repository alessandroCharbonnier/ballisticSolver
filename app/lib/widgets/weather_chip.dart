/// Weather line (section 11.4).
///
/// Shows the current reading, or red "No weather data" with a manual
/// temperature when one is set. Tapping opens the manual temperature dialog.
library;

import 'package:material_ui/material_ui.dart';

import '../domain/models.dart';

class WeatherChip extends StatelessWidget {
  const WeatherChip({
    super.key,
    required this.reading,
    required this.isCurrent,
    required this.manualTemp,
    required this.onTap,
  });

  final WeatherReading? reading;
  final bool isCurrent;
  final double? manualTemp;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    final current = isCurrent ? reading : null;
    final String text;
    final Color color;

    if (current != null) {
      text = '${current.tempC.toStringAsFixed(1)} C  '
          '${current.pressureHpa.toStringAsFixed(0)} hPa  '
          '${current.humidityPct.toStringAsFixed(0)} %';
      color = Theme.of(context).colorScheme.onSurface;
    } else {
      final manual = manualTemp;
      text = manual == null
          ? 'No weather data'
          : 'No weather data  -  manual ${manual.toStringAsFixed(1)} C';
      color = Colors.redAccent;
    }

    return InkWell(
      onTap: onTap,
      borderRadius: BorderRadius.circular(8),
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
        decoration: BoxDecoration(
          color: color.withValues(alpha: 0.08),
          borderRadius: BorderRadius.circular(8),
        ),
        child: Row(
          children: [
            Icon(Icons.thermostat, size: 18, color: color),
            const SizedBox(width: 8),
            Expanded(child: Text(text, style: TextStyle(color: color))),
          ],
        ),
      ),
    );
  }
}
