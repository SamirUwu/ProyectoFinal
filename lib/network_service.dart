import 'dart:convert';
import 'dart:io';
import 'package:flutter/foundation.dart';

class NetworkService {
  static Socket? _socket;

  static Future<void> connect() async {
  try {
    debugPrint("🔌 Intentando conectar...");
    _socket = await Socket.connect('10.199.202.233', 5000)
        .timeout(const Duration(seconds: 5));
    debugPrint("🟢 Conectado correctamente");
  } catch (e) {
    debugPrint("🔴 Error de conexión: $e");
  }
}


  static void sendJson(Map<String, dynamic> data) {
    if (_socket == null) {
      debugPrint("⚠️ No hay conexión");
      return;
    }

    final message = jsonEncode(data);
    debugPrint("📤 APP → $message");

    _socket!.write(message + "\n"); 
  }
}
