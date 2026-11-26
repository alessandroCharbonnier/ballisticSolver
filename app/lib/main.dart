import 'package:material_ui/material_ui.dart';

import 'screens/home_screen.dart';

void main() {
  runApp(const BallisticSentinelApp());
}

class BallisticSentinelApp extends StatelessWidget {
  const BallisticSentinelApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Ballistic Sentinel',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(
          seedColor: const Color(0xFF2E7D32),
          brightness: Brightness.dark,
        ),
        useMaterial3: true,
      ),
      home: const HomeScreen(),
    );
  }
}
