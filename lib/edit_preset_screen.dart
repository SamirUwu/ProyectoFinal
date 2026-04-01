import 'package:flutter/material.dart';
import 'package:hive_flutter/hive_flutter.dart';
import 'edit_effect_screen.dart';

const List<String> availableEffects = [
  'Delay',
  'Overdrive',
  'Chorus',
  'Flanger',
  'Wah',
  'Reverb',
  'Phaser',
  'PitchShifter',
];

// Parámetros default que matchean exactamente con main_window.py
const Map<String, Map<String, double>> effectDefaults = {
  'Overdrive':    {'GAIN': 0.5,    'TONE': 0.5,   'OUTPUT': 0.5},
  'Delay':        {'TIME': 300.0,  'FEEDBACK': 0.3, 'MIX': 0.15},
  'Wah':          {'FREQ': 1000.0, 'Q': 0.8,      'LEVEL': 1.0},
  'Flanger':      {'RATE': 0.5,    'DEPTH': 0.3,  'FEEDBACK': 0.2, 'MIX': 0.15},
  'Chorus':       {'RATE': 0.5,    'DEPTH': 0.3,  'FEEDBACK': 0.0, 'MIX': 0.15},
  'Phaser':       {'RATE': 0.5,    'DEPTH': 0.7,  'FEEDBACK': 0.3, 'MIX': 0.15},
  'PitchShifter': {'SEMITONES': 0.0, 'SEMITONES_B': 0.0, 'MIX_A': 1.0, 'MIX_B': 0.0, 'MIX': 0.5},
  'Reverb':       {'FEEDBACK': 0.6, 'LPFREQ': 8000.0, 'MIX': 0.15},
};

class EditPresetScreen extends StatefulWidget {
  final String presetName;
  final Map<String, double>? initialValues;
  const EditPresetScreen({super.key, required this.presetName, this.initialValues});

  @override
  State<EditPresetScreen> createState() => _EditPresetScreenState();
}

class _EditPresetScreenState extends State<EditPresetScreen> {
  
  Map<String, Map<String, double>> presetData = {};

  @override
  void initState() {
    super.initState();
    final box = Hive.box('preset_data');
    final saved = box.get(widget.presetName);
    if (saved != null) {
      presetData = Map<String, Map<String, double>>.from(
        saved.map(
          (k, v) => MapEntry(k, Map<String, double>.from(v)),
        ),
      );
    }
  }

  void _confirmDeleteEffect(String effect) {
    showDialog(
      context: context,
      builder: (_) => AlertDialog(
        title: const Text('Eliminar efecto'),
        content: Text('¿Seguro que quieres borrar "$effect"?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancelar'),
          ),
          TextButton(
            onPressed: () {
              setState(() {
                presetData.remove(effect);
              });
              Navigator.pop(context);
            },
            child: const Text('Borrar'),
          ),
        ],
      ),
    );
  }

  void _onReorder(int oldIndex, int newIndex) {
    setState(() {
      if (newIndex > oldIndex) newIndex--;
      final keys = presetData.keys.toList();
      final item = keys.removeAt(oldIndex);
      keys.insert(newIndex, item);
      final newMap = <String, Map<String, double>>{};
      for (final k in keys) {
        newMap[k] = presetData[k]!;
      }
      presetData = newMap;
    });
  }

  void _showAddEffectDialog() {
    final remaining = availableEffects
        .where((e) => !presetData.containsKey(e))
        .toList();

    if (remaining.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Todos los efectos ya están añadidos')),
      );
      return;
    }

    showModalBottomSheet(
      context: context,
      builder: (_) => ListView(
        children: remaining.map((effect) {
          return ListTile(
            title: Text(effect),
            onTap: () {
              Navigator.pop(context);
              setState(() {
                // FIX: usar defaults en vez de mapa vacío
                presetData[effect] = Map<String, double>.from(
                  effectDefaults[effect] ?? {},
                );
              });
            },
          );
        }).toList(),
      ),
    );
  }

  @override
  void dispose() {
    final box = Hive.box('preset_data');
    box.put(widget.presetName, {
      for (final entry in presetData.entries)
        entry.key: Map<String, double>.from(entry.value)
    });
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text("Editar Preset"),
        leading: IconButton(
          icon: const Icon(Icons.arrow_back),
          onPressed: () {
            final box = Hive.box('preset_data');
            box.put(widget.presetName, {
              for (final entry in presetData.entries)
                entry.key: Map<String, double>.from(entry.value)
            });
            Navigator.pop(context);
          },
        ),
      ),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              "Preset: ${widget.presetName}",
              style: const TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
            ),
            Expanded(
              child: ReorderableListView(
                onReorder: _onReorder,
                children: presetData.keys.map((effect) {
                  return Padding(
                    key: ValueKey(effect),
                    padding: const EdgeInsets.only(bottom: 10),
                    child: _buildEffectBlock(context, effect),
                  );
                }).toList(),
              ),
            ),
            const SizedBox(height: 12),
            ElevatedButton.icon(
              onPressed: _showAddEffectDialog,
              icon: const Icon(Icons.add),
              label: const Text('Añadir efecto'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.deepPurple,
                foregroundColor: Colors.white,
              ),
            ),
          ],  
        ),
      ),
    );
  }

  Widget _buildEffectBlock(BuildContext context, String title) {
    return Container(
      height: 80,
      padding: const EdgeInsets.symmetric(horizontal: 16),
      decoration: BoxDecoration(
        color: const Color.fromARGB(255, 128, 92, 194),
        borderRadius: BorderRadius.circular(12),
      ),
      child: Row(
        children: [
          Expanded(
            child: GestureDetector(
              onTap: () async {
                final result = await Navigator.push<Map<String, double>>(
                  context,
                  MaterialPageRoute(
                    builder: (context) => EditEffectScreen(
                      effectName: title,
                      initialValues: presetData[title],
                    ),
                  ),
                );
                if (result != null) {
                  setState(() {
                    presetData[title] = result;
                  });
                }
              },
              child: Text(title, style: const TextStyle(fontSize: 18)),
            ),
          ),
          IconButton(
            icon: const Icon(Icons.delete, color: Colors.white),
            onPressed: () => _confirmDeleteEffect(title),
          ),
          const Icon(Icons.drag_handle, color: Colors.white),
        ],
      ),
    );
  }
}