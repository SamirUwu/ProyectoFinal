import 'dart:convert';
import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:multicast_dns/multicast_dns.dart';

class NetworkService {
  static Socket? _socket;

  static Future<void> connect() async {
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

    _socket!.write("$message\n");
  }
}
