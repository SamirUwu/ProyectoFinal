import 'dart:convert';
import 'dart:io';
import 'package:flutter/foundation.dart';

class NetworkService {
  static const String _host = "192.168.1.150";  
  static const int _port = 5000;

  static Socket? _socket;
  static bool _running = false;

  static Future<void> startAutoConnect() async {
    if (_running) return;
    _running = true;

    while (_running) {
      if (_socket == null) {
        await _connectOnce();
      }
      await Future.delayed(const Duration(seconds: 3));
    }
  }

  static Future<void> _connectOnce() async {
    try {
      debugPrint("🔌 Conectando a $_host:$_port...");

      _socket = await Socket.connect(
        _host,
        _port,
        timeout: const Duration(seconds: 5),
      );

      debugPrint("🟢 Conectado");

      _socket!.listen(
        (_) {},
        onDone: () {
          debugPrint("🔌 Conexión cerrada");
          _socket?.destroy();
          _socket = null;
        },
        onError: (error) {
          debugPrint("💀 Error: $error");
          _socket?.destroy();
          _socket = null;
        },
        cancelOnError: true,
      );

    } catch (e) {
      debugPrint("🔴 No se pudo conectar: $e");
      _socket = null;
    }
  }

  static void sendJson(Map<String, dynamic> data) {
    if (_socket == null) {
      debugPrint("⚠️ Sin conexión");
      return;
    }
    try {
      final message = jsonEncode(data);
      debugPrint("📤 → $message");
      _socket!.write("$message\n");
    } catch (e) {
      debugPrint("💀 Error enviando: $e");
      _socket = null;
    }
  }

  static void stop() {
    _running = false;
    _socket?.destroy();
    _socket = null;
  }
}