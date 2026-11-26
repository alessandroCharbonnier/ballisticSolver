import 'package:flutter_test/flutter_test.dart';

import 'package:ballistic_sentinel/domain/slots.dart';

void main() {
  group('T3 slot numbering', () {
    test('slot 1 is R1 C1 on a 5 by 10 box', () {
      expect(SlotMath.positionLabel(1, 10), 'R1 C1');
    });

    test('slot 10 is R1 C10 on a 5 by 10 box', () {
      expect(SlotMath.positionLabel(10, 10), 'R1 C10');
    });

    test('slot 11 is R2 C1 on a 5 by 10 box', () {
      expect(SlotMath.positionLabel(11, 10), 'R2 C1');
    });

    test('slot 14 is R2 C4 on a 5 by 10 box', () {
      expect(SlotMath.positionLabel(14, 10), 'R2 C4');
    });

    test('slot 50 is R5 C10 on a 5 by 10 box', () {
      expect(SlotMath.positionLabel(50, 10), 'R5 C10');
    });

    test('box label is B01 R2 C4', () {
      expect(SlotMath.withBox('B01', 14, 10), 'B01 R2 C4');
    });

    test('a 10 by 5 layout numbers the other way', () {
      // 10 rows of 5 columns: slot 6 is R2 C1.
      expect(SlotMath.positionLabel(1, 5), 'R1 C1');
      expect(SlotMath.positionLabel(5, 5), 'R1 C5');
      expect(SlotMath.positionLabel(6, 5), 'R2 C1');
      expect(SlotMath.positionLabel(50, 5), 'R10 C5');
    });
  });
}
