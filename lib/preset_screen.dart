import 'package:flutter/material.dart';
import 'edit_preset_screen.dart';
import 'package:hive/hive.dart';


class PresetScreen extends StatefulWidget {
  const PresetScreen({super.key});

  @override
  State<PresetScreen> createState() => _PresetScreenState();
}

class _PresetScreenState extends State<PresetScreen> {
  final List<String> presets = ['Preset 1', 'Preset 2', 'Preset 3'];
  final int maxPresets = 8;
  final List<String> attachedPresets = [];

  void _loadPresetsFromHive() {
    final box = Hive.box('presets');
    final savedPresets = box.get('__preset_list__');

    if (savedPresets is List && savedPresets.isNotEmpty) {
      setState(() {
        presets
          ..clear()
          ..addAll(List<String>.from(savedPresets));
      });
    } else {
      debugPrint('Hive devolvió lista vacía, mantengo presets en memoria');
    }

    debugPrint('PRESETS EN MEMORIA: $presets');
  }




  @override
  void initState() {
    super.initState();
    _loadPresetsFromHive();
  }

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

    if (result?.isNotEmpty == true) {
      setState(() {
        presets.add(result!);
        final box = Hive.box('presets');
        box.put('__preset_list__', presets);
        debugPrint('LISTA DE PRESETS GUARDADA: $presets');
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
                  final box = Hive.box('presets');
                  final removed = presets.removeAt(index);

                  attachedPresets.remove(removed);
                  box.delete(removed); // borra datos del preset
                  box.put('__preset_list__', presets);
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

    if (newName?.isNotEmpty == true) {
      setState(() {
        final box = Hive.box('presets');

        final oldName = presets[index];
        presets[index] = newName!;

        // mover datos del preset si existían
        final presetData = box.get(oldName);
        if (presetData != null) {
          box.delete(oldName);
          box.put(newName, presetData);
        }

        // guardar lista actualizada
        box.put('__preset_list__', presets);
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
                      onTap: () async {
                        await Navigator.push(
                          context,
                          MaterialPageRoute(
                            builder: (context) => EditPresetScreen(presetName: presets[index]),
                          ),
                        );
                        _loadPresetsFromHive();
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
