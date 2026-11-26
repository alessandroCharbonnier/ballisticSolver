/// Magazine stack (section 11.4).
///
/// Ten fixed rows; position 10 at the top, position 1 at the bottom. A filled
/// row shows the position and where the cartridge came from. The top filled
/// row is marked "NEXT". The whole stack is one tap target: a tap unloads the
/// top.
library;

import 'package:material_ui/material_ui.dart';

import '../data/magazine_repository.dart';

class MagazineStack extends StatelessWidget {
  const MagazineStack({
    super.key,
    required this.entries,
    required this.onTap,
  });

  /// Cartridges in the magazine, from the bottom (position 1) to the top.
  final List<MagazineEntry> entries;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    final byPosition = {for (final e in entries) e.position: e};
    final topPosition = entries.isEmpty ? 0 : entries.last.position;

    return InkWell(
      onTap: onTap,
      borderRadius: BorderRadius.circular(8),
      child: Column(
        children: [
          for (var position = 10; position >= 1; position--)
            _row(context, position, byPosition[position],
                position == topPosition),
        ],
      ),
    );
  }

  Widget _row(
    BuildContext context,
    int position,
    MagazineEntry? entry,
    bool isTop,
  ) {
    final filled = entry != null;
    final outlineColor = Theme.of(context).colorScheme.outline;
    final fillColor = isTop
        ? const Color(0xFF2E7D32)
        : const Color(0xFF455A64);

    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 2),
      child: Container(
        height: 34,
        padding: const EdgeInsets.symmetric(horizontal: 10),
        alignment: Alignment.centerLeft,
        decoration: BoxDecoration(
          color: filled ? fillColor : Colors.transparent,
          border: Border.all(color: filled ? fillColor : outlineColor),
          borderRadius: BorderRadius.circular(6),
        ),
        child: Row(
          children: [
            SizedBox(
              width: 22,
              child: Text(
                '$position',
                style: TextStyle(
                  fontWeight: FontWeight.bold,
                  color: filled ? Colors.white : outlineColor,
                ),
              ),
            ),
            if (filled)
              Text(
                '${entry.position} - ${entry.originLabel}',
                style: const TextStyle(color: Colors.white, fontSize: 13),
              ),
            const Spacer(),
            if (isTop)
              const Text(
                'NEXT',
                style: TextStyle(
                  color: Colors.white,
                  fontWeight: FontWeight.bold,
                  fontSize: 11,
                ),
              ),
          ],
        ),
      ),
    );
  }
}
