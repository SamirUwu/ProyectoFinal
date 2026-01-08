import 'package:flutter/material.dart';
import 'edit_preset_screen.dart';
import 'package:hive/hive.dart';
import 'dart:convert'; 
import 'network_service.dart';
import 'main.dart';

class PresetScreen extends StatefulWidget {
  const PresetScreen({super.key});

  @override
  State<PresetScreen> createState() => _PresetScreenState();
}

class _PresetScreenState extends State<PresetScreen> {
  final List<String> presets = [];
  final int maxPresets = 8;
  final List<String> attachedPresets = [];

  void _loadPresetsFromHive() {
    final box = Hive.box('preset_list');
    final savedPresets = box.get('list');

    if (savedPresets is List) {
      setState(() {
        presets
          ..clear()
          ..addAll(List<String>.from(savedPresets));
      });
    }
    //debugPrint('PRESETS EN MEMORIA: $presets');
  }


  @override
  void initState() {
    super.initState();
    _loadPresetsFromHive();
    NetworkService.connect();
  }

  void _sendPresetWithMode(String presetName, String mode) {
    final dataBox = Hive.box('preset_data');
    final data = dataBox.get(presetName);

    if (data == null) {
      debugPrint('No hay datos para $presetName');
      return;
    }

    final Map<String, Map<String, double>> cleanEffects = {};

    data.forEach((effectName, params) {
      final Map<String, double> cleanParams = {};

      (params as Map).forEach((k, v) {
        cleanParams[k.toString()] = (v as num).toDouble();
      });

      cleanEffects[effectName.toString()] = cleanParams;
    });

    final payload = {
      "mode": mode,
      "name": presetName,
      "effects": cleanEffects,
    };

        NetworkService.sendJson(payload);
        final jsonString = jsonEncode(payload);
        debugPrint('JSON → ESP32: $jsonString');

        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Preset "$presetName" enviado')),
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
        final box = Hive.box('preset_list');
        box.put('list', presets);
        //debugPrint('LISTA DE PRESETS GUARDADA: $presets');
      });

      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Preset "$result" añadido'),
          duration: const Duration(seconds: 2),
        ),
      );
    }
  }

  void _deletePreset(int index) {
    setState(() {
      final listBox = Hive.box('preset_list');
      final dataBox = Hive.box('preset_data');

      final removed = presets.removeAt(index);
      attachedPresets.remove(removed);

      dataBox.delete(removed);
      listBox.put('list', presets);

      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Preset "$removed" eliminado')),
      );
    });
  }

  
  void _showPresetOptions(int index) {

    showModalBottomSheet(
      context: context,
      builder: (_) => SafeArea(
        child: Wrap(
          children: [
            ListTile(
              leading: const Icon(Icons.tune),
              title: const Text('Editar efecto'),
              onTap: () async {
                Navigator.pop(context);

                final presetName = presets[index];

                await Navigator.push(
                  context,
                  MaterialPageRoute(
                    builder: (context) => EditPresetScreen(presetName: presetName),
                  ),
                );  

                _sendPresetWithMode(presetName, "create");
              },
            ),

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
                _deletePreset(index);
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
        final listBox = Hive.box('preset_list');
        final dataBox = Hive.box('preset_data');

        final oldName = presets[index];
        presets[index] = newName!;

        final presetData = dataBox.get(oldName);
        if (presetData != null) {
          dataBox.delete(oldName);
          dataBox.put(newName, presetData);
        }

        listBox.put('list', presets);
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
                        final presetName = presets[index];
                        _sendPresetWithMode(presetName, "change");
                      },

                      trailing: IconButton(
                        icon: const Icon(Icons.settings),
                        onPressed: () {
                          _showPresetOptions(index);
                        },
                      ),
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
          ],
        ),
      ),
      floatingActionButton: FloatingActionButton(
        child: const Icon(Icons.settings),
        onPressed: () {
          showModalBottomSheet(
            context: context,
            shape: const RoundedRectangleBorder(
              borderRadius: BorderRadius.vertical(top: Radius.circular(20)),
            ),
            builder: (context) {
              final appState = MyApp.of(context);

              return Padding(
                padding: const EdgeInsets.all(20),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    const Text(
                      'Modo oscuro',
                      style: TextStyle(fontSize: 18),
                    ),
                    Switch(
                      value: appState.isDark,
                      onChanged: (value) {
                        appState.toggleTheme(value);
                      },
                    ),
                  ],
                ),
              );
            },
          );
        },
      ),
    );
  }
}
