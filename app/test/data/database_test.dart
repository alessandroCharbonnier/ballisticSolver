import 'package:flutter_test/flutter_test.dart';

import 'package:ballistic_sentinel/domain/constants.dart';

import 'helpers.dart';

void main() {
  test('opens an in-memory database and seeds the rifle', () async {
    final ctx = TestContext(await openTestDatabase());

    final rifle = await ctx.sessions.activeRifle();
    expect(rifle, isNotNull);
    expect(rifle!.name, AppConstants.defaultRifleName);

    await ctx.db.close();
  });
}
