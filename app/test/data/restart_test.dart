import 'dart:io';

import 'package:flutter_test/flutter_test.dart';

import 'package:ballistic_sentinel/data/app_database.dart';

import 'helpers.dart';

void main() {
  test('T6a nothing is lost on restart (L6)', () async {
    ensureSqfliteFfi();

    final dir = await Directory.systemTemp.createTemp('sentinel_test');
    final path = '${dir.path}${Platform.pathSeparator}sentinel.db';

    var ctx = TestContext(AppDatabase(path));
    await ctx.db.database;
    final box = await createBoxFixture(ctx);
    for (var i = 0; i < 3; i++) {
      await measure(ctx, box.cartridges[i]);
      await ctx.magazine.load(box.cartridges[i].id!);
    }

    // Close the database (the app was "killed").
    await ctx.db.close();

    // Reopen it: the magazine has the same three, in the same order.
    ctx = TestContext(AppDatabase(path));
    await ctx.db.database;
    final contents = await ctx.magazine.contents();
    expect(contents.length, 3);
    expect(contents.map((e) => e.position).toList(), [1, 2, 3]);
    expect(contents.map((e) => e.cartridge.id).toList(), [
      box.cartridges[0].id,
      box.cartridges[1].id,
      box.cartridges[2].id,
    ]);

    await ctx.db.close();
    await dir.delete(recursive: true);
  });
}
