import 'package:flutter/material.dart';
import 'package:flutter_bluetooth_serial/flutter_bluetooth_serial.dart';
import 'preset_screen.dart';

class BluetoothScreen extends StatefulWidget {
  const BluetoothScreen({super.key});

  @override
  State<BluetoothScreen> createState() => _BluetoothScreenState();
}

class _BluetoothScreenState extends State<BluetoothScreen> {
  List<BluetoothDevice> bondedDevices = [];

  @override
  void initState() {
    super.initState();
    getBondedDevices();
  }

  Future<void> getBondedDevices() async {
    List<BluetoothDevice> devices = [];

    try {
      devices = await FlutterBluetoothSerial.instance.getBondedDevices();
    } catch (e) {
      print("Error obteniendo dispositivos emparejados: $e");
    }

    setState(() {
      bondedDevices = devices;
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Dispositivos emparejados')),
      body: Column(
        children: [
          Expanded(
            child: bondedDevices.isEmpty
                ? const Center(child: Text('No hay dispositivos emparejados'))
                : ListView.builder(
                    itemCount: bondedDevices.length,
                    itemBuilder: (context, index) {
                      final device = bondedDevices[index];
                      return ListTile(
                        title: Text(device.name ?? 'Sin nombre'),
                        subtitle: Text(device.address),
                        onTap: () {
                          ScaffoldMessenger.of(context).showSnackBar(
                            SnackBar(content: Text('Seleccionaste ${device.name}')),
                          );
                        },
                      );
                    },
                  ),
          ),
          Padding(
            padding: const EdgeInsets.all(16.0),
            child: ElevatedButton(
              onPressed: () {
                Navigator.push(
                  context,
                  MaterialPageRoute(builder: (context) => const PresetScreen()),
                );
              },
              child: const Text('Continuar'),
            ),
          ),
        ],
      ),
    );
  }
}
