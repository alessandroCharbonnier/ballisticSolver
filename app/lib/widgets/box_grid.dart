/// Reusable box grid (section 11.3, reused by Fire and phase 2).
///
/// The grid receives, for each slot, a label, a colour, a fill style and
/// whether it can be tapped, so screens stay in charge of their own meaning.
library;

import 'package:material_ui/material_ui.dart';

/// One cell of the grid.
class BoxGridSlot {
  final String label;
  final Color color;
  final bool filled;
  final bool canTap;
  final bool isCircle;
  final bool dimmed;
  final VoidCallback? onTap;

  const BoxGridSlot({
    this.label = '',
    required this.color,
    this.filled = false,
    this.canTap = false,
    this.isCircle = false,
    this.dimmed = false,
    this.onTap,
  });
}

class BoxGrid extends StatelessWidget {
  const BoxGrid({
    super.key,
    required this.rows,
    required this.cols,
    required this.hingeEdge,
    required this.slots,
  });

  final int rows;
  final int cols;

  /// `top`, `bottom`, `left` or `right` (which edge is the lid hinge).
  final String hingeEdge;

  /// One entry per slot, in slot order (index 0 is slot 1).
  final List<BoxGridSlot> slots;

  @override
  Widget build(BuildContext context) {
    final grid = Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        for (var r = 0; r < rows; r++)
          Row(
            children: [
              for (var c = 0; c < cols; c++)
                Expanded(
                  child: _cell(slots[r * cols + c]),
                ),
            ],
          ),
      ],
    );

    switch (hingeEdge) {
      case 'top':
        return Column(children: [_hingeBar(context), grid]);
      case 'bottom':
        return Column(children: [grid, _hingeBar(context)]);
      case 'left':
        return Row(children: [_hingeBar(context), Expanded(child: grid)]);
      case 'right':
        return Row(children: [Expanded(child: grid), _hingeBar(context)]);
      default:
        return Column(children: [_hingeBar(context), grid]);
    }
  }

  Widget _hingeBar(BuildContext context) {
    final color = Theme.of(context).colorScheme.outline;
    final isVertical = hingeEdge == 'left' || hingeEdge == 'right';
    return Container(
      margin: const EdgeInsets.all(2),
      padding: isVertical
          ? const EdgeInsets.symmetric(horizontal: 4, vertical: 12)
          : const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
      decoration: BoxDecoration(
        color: color.withValues(alpha: 0.15),
        borderRadius: BorderRadius.circular(6),
      ),
      child: Text(
        'HINGE',
        style: TextStyle(
          fontSize: 11,
          fontWeight: FontWeight.bold,
          letterSpacing: 1.2,
          color: color,
        ),
      ),
    );
  }

  Widget _cell(BoxGridSlot slot) {
    final textStyle = TextStyle(
      fontSize: 11,
      fontWeight: FontWeight.w600,
      color: slot.filled
          ? Colors.white
          : slot.color.withValues(alpha: 0.9),
    );
    final box = Container(
      alignment: Alignment.center,
      decoration: BoxDecoration(
        color: slot.filled ? slot.color : Colors.transparent,
        border: Border.all(
          color: slot.color,
          width: slot.filled ? 1 : 2,
        ),
        shape: slot.isCircle ? BoxShape.circle : BoxShape.rectangle,
        borderRadius: slot.isCircle ? null : BorderRadius.circular(6),
      ),
      child: Text(slot.label, style: textStyle, maxLines: 1),
    );
    final content = slot.isCircle ? AspectRatio(aspectRatio: 1, child: box) : box;
    return Padding(
      padding: const EdgeInsets.all(2),
      child: InkWell(
        onTap: slot.canTap ? slot.onTap : null,
        borderRadius: BorderRadius.circular(6),
        child: Center(
          child: Opacity(
            opacity: slot.dimmed ? 0.35 : 1.0,
            child: content,
          ),
        ),
      ),
    );
  }
}
