/// Inherited widget that exposes [AppServices] to every screen.
library;

import 'package:material_ui/material_ui.dart';

import 'app_services.dart';

class AppScope extends InheritedWidget {
  const AppScope({
    super.key,
    required this.services,
    required this.onRebuild,
    required super.child,
  });

  final AppServices services;

  /// Recreates [AppServices] and returns to Home (used when the simulator is
  /// toggled).
  final Future<void> Function() onRebuild;

  static AppServices of(BuildContext context) {
    final scope = context.dependOnInheritedWidgetOfExactType<AppScope>();
    assert(scope != null, 'AppScope not found in the widget tree');
    return scope!.services;
  }

  static Future<void> Function() rebuildOf(BuildContext context) {
    final scope = context.dependOnInheritedWidgetOfExactType<AppScope>();
    assert(scope != null, 'AppScope not found in the widget tree');
    return scope!.onRebuild;
  }

  @override
  bool updateShouldNotify(AppScope oldWidget) =>
      services != oldWidget.services || onRebuild != oldWidget.onRebuild;
}
