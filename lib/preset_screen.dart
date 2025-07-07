import 'package:flutter/material.dart';
import 'edit_preset_screen.dart';

class PresetScreen extends StatefulWidget {
  const PresetScreen({super.key});

  @override
  State<PresetScreen> createState() => _PresetScreenState();
}

class _PresetScreenState extends State<PresetScreen> {
  final List<String> presets = ['Preset 1', 'Preset 2', 'Preset 3'];
  final int maxPresets = 8;
  final List<String> attachedPresets = [];

    void _sendToESP32() {
    if (attachedPresets.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('No hay presets anexados para enviar'),
          duration: Duration(seconds: 2),
        ),
      );
      return;
    }

    // Simulación de envío
    print("Enviando a ESP32: $attachedPresets");

    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text('Presets enviados: ${attachedPresets.join(', ')}'),
        duration: const Duration(seconds: 3),
      ),
    );
  }

  void _addPreset() async {
    if (presets.length >= maxPresets) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('¡Límite de presets alcanzado!'),
          duration: Duration(seconds: 2),
        ),
      );
      return;
    }
    final TextEditingController controller = TextEditingController();

    String? result = await showDialog<String>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Nombre del nuevo preset'),
        content: TextField(
          controller: controller,
          decoration: const InputDecoration(hintText: 'Ej: Clean Delay'),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancelar'),
          ),
          TextButton(
            onPressed: () {
              String input = controller.text.trim();
              if (input.isNotEmpty) {
                Navigator.pop(context, input);
              }
            },
            child: const Text('Aceptar'),
          ),
        ],
      ),
    );

    if (result != null && result.isNotEmpty) {
      setState(() {
        presets.add(result);
      });

      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Preset "$result" añadido'),
          duration: const Duration(seconds: 2),
        ),
      );
    }
  }
  
  void _showPresetOptions(int index) {
    showModalBottomSheet(
      context: context,
      builder: (_) => SafeArea(
        child: Wrap(
          children: [
            ListTile(
              leading: const Icon(Icons.edit),
              title: const Text('Cambiar nombre'),
              onTap: () {
                Navigator.pop(context);
                _renamePreset(index);
              },
            ),
            ListTile(
              leading: const Icon(Icons.delete),
              title: const Text('Eliminar'),
              onTap: () {
                Navigator.pop(context);
                setState(() {
                  String removed = presets.removeAt(index);
                  attachedPresets.remove(removed); // si estaba anexado, quitarlo también
                  ScaffoldMessenger.of(context).showSnackBar(
                    SnackBar(
                      content: Text('Preset "$removed" eliminado'),
                      duration: const Duration(seconds: 2),
                    ),
                  );
                });
              },
            ),
            ListTile(
              leading: const Icon(Icons.send),
              title: const Text('Anexar para envío'),
              onTap: () {
                Navigator.pop(context);
                setState(() {
                  final preset = presets[index];
                  if (!attachedPresets.contains(preset)) {
                    attachedPresets.add(preset);
                    ScaffoldMessenger.of(context).showSnackBar(
                      SnackBar(
                        content: Text('"$preset" anexado para envío'),
                        duration: const Duration(seconds: 2),
                      ),
                    );
                  }
                });
              },
            ),
          ],
        ),
      ),
    );
  }

  void _renamePreset(int index) async {
    final TextEditingController controller =
        TextEditingController(text: presets[index]);

    String? newName = await showDialog<String>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Cambiar nombre del preset'),
        content: TextField(
          controller: controller,
          decoration: const InputDecoration(hintText: 'Nuevo nombre'),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancelar'),
          ),
          TextButton(
            onPressed: () {
              String input = controller.text.trim();
              if (input.isNotEmpty) {
                Navigator.pop(context, input);
              }
            },
            child: const Text('Guardar'),
          ),
        ],
      ),
    );

    if (newName != null && newName.isNotEmpty) {
      setState(() {
        presets[index] = newName;
      });

      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Preset renombrado a "$newName"'),
          duration: const Duration(seconds: 2),
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Conectado: MultiFX'),
      ),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Presets:',
              style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 16),
            Expanded(
              child: ListView.builder(
                itemCount: presets.length,
                itemBuilder: (context, index) {
                  return Card(
                    child: ListTile(
                      title: Text(presets[index]),
                      onTap: () {
                        Navigator.push(
                          context,
                          MaterialPageRoute(
                            builder: (context) => EditPresetScreen(presetName: presets[index]),
                          ),
                        );
                      },
                      onLongPress: () => _showPresetOptions(index),
                    ),
                  );
                },
              ),
            ),
            const SizedBox(height: 12),
            ElevatedButton.icon(
              onPressed: _addPreset,
              icon: const Icon(Icons.add),
              label: const Text('Añadir'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.deepPurple,
                foregroundColor: Colors.white,
              ),
            ),
            const SizedBox(height: 8),
            ElevatedButton.icon(
              onPressed: _sendToESP32,
              icon: const Icon(Icons.send),
              label: const Text('Enviar a ESP32'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.green,
                foregroundColor: Colors.white,
              ),
            ),
          ],
        ),
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: () {
          // Futuro: más opciones
        },
        child: const Icon(Icons.settings),
      ),
    );
  }
}
