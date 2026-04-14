import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'network_service.dart';
import 'preset_screen.dart';
import 'widgets/network_debug_overlay.dart';

class NetworkSettingsScreen extends StatefulWidget {
  final bool isFromPresets;
  const NetworkSettingsScreen({super.key, this.isFromPresets = false});

  @override
  State<NetworkSettingsScreen> createState() => _NetworkSettingsScreenState();
}

class _NetworkSettingsScreenState extends State<NetworkSettingsScreen> {
  late TextEditingController _hostController;

  @override
  void initState() {
    super.initState();
    _hostController = TextEditingController(text: NetworkService.host);
  }

  @override
  void dispose() {
    _hostController.dispose();
    super.dispose();
  }

  void _saveHost() {
    final newHost = _hostController.text.trim();
    if (newHost.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('La IP no puede estar vacía')),
      );
      return;
    }

    final ipRegex = RegExp(r'^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$');
    if (!ipRegex.hasMatch(newHost)) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Formato de IP inválido')),
      );
      return;
    }

    // ✅ ESTO FALTABA — sin esto el host nunca cambia
    NetworkService.setHost(newHost);

    if (widget.isFromPresets) {
      Navigator.pop(context);
    } else {
      Navigator.pushReplacement(
        context,
        MaterialPageRoute(builder: (context) => const PresetScreen()),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return NetworkDebugOverlay(
      child: Scaffold(
        appBar: AppBar(
          title: const Text('Configuración de Red'),
        ),
        body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Dirección IP del servidor',
              style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 8),
            const Text(
              'Ingresa la IP del dispositivo al que deseas conectarte',
              style: TextStyle(color: Colors.grey),
            ),
            const SizedBox(height: 24),
            Container(
              decoration: BoxDecoration(
                color: const Color.fromARGB(255, 128, 92, 194),
                borderRadius: BorderRadius.circular(12),
              ),
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text(
                    'IP Address',
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.bold,
                      color: Colors.white,
                    ),
                  ),
                  const SizedBox(height: 12),
                  TextField(
                    controller: _hostController,
                    keyboardType:
                        const TextInputType.numberWithOptions(decimal: true),
                    inputFormatters: [
                      FilteringTextInputFormatter.allow(RegExp(r'[\d.]')),
                    ],
                    style: const TextStyle(fontSize: 24, color: Colors.white),
                    decoration: InputDecoration(
                      hintText: '192.168.1.150',
                      hintStyle:
                          TextStyle(color: Colors.white.withOpacity(0.5)),
                      filled: true,
                      fillColor: const Color.fromARGB(255, 85, 77, 100),
                      border: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(8),
                        borderSide: BorderSide.none,
                      ),
                    ),
                  ),
                ],
              ),
            ),
            const Spacer(),
            SizedBox(
              width: double.infinity,
              height: 52,
              child: ElevatedButton(
                onPressed: _saveHost,
                style: ElevatedButton.styleFrom(
                  backgroundColor: Colors.deepPurple,
                  foregroundColor: Colors.white,
                ),
                child: const Text('GUARDAR', style: TextStyle(fontSize: 18)),
              ),
            ),
            ],
          ),
        ),
      ),   // cierra Scaffold
    );     // cierra NetworkDebugOverlay
  }
}