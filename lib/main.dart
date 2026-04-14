import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:hive_flutter/hive_flutter.dart';
import 'welcome_screen.dart';
import 'network_service.dart';
import 'widgets/network_debug_overlay.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Hive.initFlutter();
  await Hive.openBox('preset_list');
  await Hive.openBox('preset_data');
  NetworkService.startAutoConnect();

  var settingsBox = Hive.box('preset_data');
  bool savedTheme = settingsBox.get('darkMode', defaultValue: false);
  runApp(MyApp(initialDark: savedTheme));
}

class MyApp extends StatefulWidget {
  final bool initialDark;

  const MyApp({super.key, required this.initialDark});

  static _MyAppState of(BuildContext context) =>
      context.findAncestorStateOfType<_MyAppState>()!;

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  late bool isDark;

  @override
  void initState() {
    super.initState();
    isDark = widget.initialDark;
  }

  void toggleTheme(bool value) {
    setState(() => isDark = value);
    Hive.box('preset_data').put('darkMode', value);
  }

  @override
  Widget build(BuildContext context) {
    // El overlay solo aparece en debug builds (flutter run), 
    // desaparece solo en release (flutter build apk --release)
    final home = kDebugMode
        ? NetworkDebugOverlay(child: const WelcomeScreen())
        : const WelcomeScreen();

    return MaterialApp(
      title: 'MultiFX',
      theme: isDark
          ? ThemeData.dark(useMaterial3: true)
          : ThemeData(
              colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
              useMaterial3: true,
            ),
      home: home,
    );
  }
}