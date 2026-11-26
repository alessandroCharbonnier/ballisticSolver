/// Amber "SIMULATOR" word shown in every app bar when the simulator is on
/// (section 12.4).
library;

import 'package:material_ui/material_ui.dart';

import '../app/app_scope.dart';

class SimulatorBadge extends StatelessWidget {
  const SimulatorBadge({super.key});

  @override
  Widget build(BuildContext context) {
    if (!AppScope.of(context).simulatorOn) return const SizedBox.shrink();
    return const Center(
      child: Padding(
        padding: EdgeInsets.symmetric(horizontal: 10),
        child: Text(
          'SIMULATOR',
          style: TextStyle(
            color: Color(0xFFFFC107),
            fontWeight: FontWeight.bold,
            fontSize: 12,
            letterSpacing: 1,
          ),
        ),
      ),
    );
  }
}
