import 'dart:convert';
import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:hive/hive.dart';

// ─── Debug logger (solo activo en debug mode) ───────────────────────────────
class NetworkDebugLogger {
  static void Function(String level, String msg)? _callback;
  static void init(void Function(String level, String msg) cb) => _callback = cb;
  static void dispose() => _callback = null;
  static void log(String level, String msg) {
    if (kDebugMode) debugPrint('[$level] $msg');
    _callback?.call(level, msg);
  }
}

// ─── NetworkService ──────────────────────────────────────────────────────────
class NetworkService {
  static String _host = "";
  static const int _port = 5000;
  static Socket? _socket;
  static bool _running = false;

  static String get host => _host;
  static bool get isConnected => _socket != null;

  static void setHost(String newHost) {
    _host = newHost.trim();
    final box = Hive.box('preset_data');
    box.put('network_host', _host);
    NetworkDebugLogger.log('INFO', 'Host actualizado a "$_host"');
    _socket?.destroy();
    _socket = null;
  }

  static void loadHostFromStorage() {
    final box = Hive.box('preset_data');
    final saved = box.get('network_host');
    if (saved != null && saved is String && saved.trim().isNotEmpty) {
      _host = saved.trim();
      NetworkDebugLogger.log('INFO', 'Host cargado desde storage: "$_host"');
    } else {
      NetworkDebugLogger.log('WARN', 'No hay host guardado en storage');
    }
  }

  static Future<void> startAutoConnect() async {
    if (_running) return;
    _running = true;
    loadHostFromStorage();
    NetworkDebugLogger.log('INFO', 'startAutoConnect iniciado');

    while (_running) {
      if (_socket == null) {
        await _connectOnce();
      }
      await Future.delayed(const Duration(seconds: 3));
    }
  }

  static Future<void> _connectOnce() async {
    final h = _host.trim();

    if (h.isEmpty) {
      NetworkDebugLogger.log('ERR', 'host está vacío — configúralo en ajustes');
      return;
    }

    try {
      NetworkDebugLogger.log('INFO', 'Conectando a $h:$_port...');
      _socket = await Socket.connect(
        h,
        _port,
        timeout: const Duration(seconds: 5),
      );
      NetworkDebugLogger.log('OK', 'Conectado a $h:$_port');

      _socket!.listen(
        (_) {},
        onDone: () {
          NetworkDebugLogger.log('WARN', 'Conexión cerrada por el servidor');
          _socket?.destroy();
          _socket = null;
        },
        onError: (error) {
          NetworkDebugLogger.log('ERR', 'Error en socket: $error');
          _socket?.destroy();
          _socket = null;
        },
        cancelOnError: true,
      );
    } on SocketException catch (e) {
      NetworkDebugLogger.log('ERR', 'SocketException: ${e.message} (os: ${e.osError?.message})');
      _socket = null;
    } catch (e) {
      NetworkDebugLogger.log('ERR', 'Error inesperado: $e');
      _socket = null;
    }
  }

  static void sendJson(Map<String, dynamic> data) {
    if (_socket == null) {
      NetworkDebugLogger.log('WARN', 'sendJson ignorado — sin conexión');
      return;
    }
    try {
      final message = jsonEncode(data);
      NetworkDebugLogger.log('SEND', message);
      _socket!.write('$message\n');
    } catch (e) {
      NetworkDebugLogger.log('ERR', 'Error enviando JSON: $e');
      _socket = null;
    }
  }

  static void stop() {
    _running = false;
    _socket?.destroy();
    _socket = null;
    NetworkDebugLogger.log('INFO', 'NetworkService detenido');
  }
}