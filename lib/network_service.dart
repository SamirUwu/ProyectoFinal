import 'dart:convert';
import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:multicast_dns/multicast_dns.dart';

class NetworkService {
  static Socket? _socket;
  static bool _connecting = false;

  static Future<void> startAutoConnect() async {
    if (_connecting) return;

    _connecting = true;

    while (true) {
      if (_socket == null) {
        await _connectOnce();
      }

      await Future.delayed(const Duration(seconds: 3));
    }
  }

  static Future<void> _connectOnce() async {
    try {
      debugPrint("🔎 Buscando servidor...");

      final client = MDnsClient();
      await client.start();

      InternetAddress? targetAddress;
      int? targetPort;

      await for (final PtrResourceRecord ptr
          in client.lookup<PtrResourceRecord>(
              ResourceRecordQuery.serverPointer('_guitarfx._tcp.local'))) {

        await for (final SrvResourceRecord srv
            in client.lookup<SrvResourceRecord>(
                ResourceRecordQuery.service(ptr.domainName))) {

          targetPort = srv.port;

          await for (final IPAddressResourceRecord ip
              in client.lookup<IPAddressResourceRecord>(
                  ResourceRecordQuery.addressIPv4(srv.target))) {

            targetAddress = ip.address;
            break;
          }
        }
      }

      client.stop();

      if (targetAddress == null) {
        debugPrint("❌ No se encontró el servidor");
        return;
      }

      debugPrint("🎯 Encontrado en $targetAddress:$targetPort");

      _socket = await Socket.connect(
        targetAddress,
        targetPort!,
        timeout: const Duration(seconds: 5),
      );

      debugPrint("🟢 Conectado correctamente");
      _socket!.listen(
      (_) {},

      onDone: () {
        debugPrint("🔌 Conexión cerrada");
        _socket?.destroy();
        _socket = null;
      },

      onError: (error) {
        debugPrint("💀 Error de socket: $error");
        _socket?.destroy();
        _socket = null;
      },

      cancelOnError: true,
    );

    } catch (e) {
      debugPrint("🔴 Error de conexión: $e");
      _socket = null;
    }
  }

  static void sendJson(Map<String, dynamic> data) {
    if (_socket == null) {
      debugPrint("⚠️ No hay conexión");
      return;
    }

    try {
      final message = jsonEncode(data);
      debugPrint("📤 APP → $message");

      _socket!.write("$message\n");

    } catch (e) {
      debugPrint("💀 Socket muerto, reconectando...");
      _socket = null;
    }
  }
}
