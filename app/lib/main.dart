import 'package:material_ui/material_ui.dart';

import 'app/app_scope.dart';
import 'app/app_services.dart';
import 'screens/home_screen.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  final services = await AppServices.create();
  runApp(BallisticSentinelApp(initialServices: services));
}

class BallisticSentinelApp extends StatefulWidget {
  const BallisticSentinelApp({super.key, required this.initialServices});

  final AppServices initialServices;

  @override
  State<BallisticSentinelApp> createState() => _BallisticSentinelAppState();
}

class _BallisticSentinelAppState extends State<BallisticSentinelApp> {
  final _navigatorKey = GlobalKey<NavigatorState>();
  late AppServices _services;

  @override
  void initState() {
    super.initState();
    _services = widget.initialServices;
  }

  Future<void> _rebuildServices() async {
    final old = _services;
    final fresh = await AppServices.create();
    if (!mounted) return;
    setState(() => _services = fresh);
    // Return to Home, then release the old services.
    _navigatorKey.currentState?.popUntil((route) => route.isFirst);
    await old.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AppScope(
      services: _services,
      onRebuild: _rebuildServices,
      child: MaterialApp(
        title: 'Ballistic Sentinel',
        debugShowCheckedModeBanner: false,
        navigatorKey: _navigatorKey,
        theme: ThemeData(
          colorScheme: ColorScheme.fromSeed(
            seedColor: const Color(0xFF2E7D32),
            brightness: Brightness.dark,
          ),
          useMaterial3: true,
        ),
        home: const HomeScreen(),
      ),
    );
  }
}
