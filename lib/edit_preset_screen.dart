import 'package:flutter/material.dart';
import 'package:hive_flutter/hive_flutter.dart';
import 'edit_effect_screen.dart';

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
    //debugPrint('HIVE CONTENIDO COMPLETO: ${box.toMap()}');

    final saved = box.get(widget.presetName);

    //debugPrint('HIVE GET (${widget.presetName}): $saved');


    if (saved != null) {
      presetData = Map<String, Map<String, double>>.from(
        saved.map(
          (k, v) => MapEntry(k, Map<String, double>.from(v)),
        ),
      );
    }
    //debugPrint('PRESET CARGADO: $presetData');
  }

  @override
  void dispose() {
    final box = Hive.box('preset_data');
    box.put(widget.presetName, {
      for (final entry in presetData.entries)
        entry.key: Map<String, double>.from(entry.value)
    });

    //debugPrint('GUARDADO EN DISPOSE: $presetData');
    //debugPrint('HIVE EN EDIT PRESET: ${Hive.box('preset_data').toMap()}');
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
            //debugPrint('AUTO-GUARDADO PRESET: $presetData');
            Navigator.pop(context);
          },
        ),
        actions: [
          TextButton(
            onPressed: () {
              final box = Hive.box('preset_data');

              //debugPrint('ANTES DE GUARDAR: ${box.toMap()}');

              box.put(widget.presetName, {
                for (final entry in presetData.entries)
                  entry.key: Map<String, double>.from(entry.value)
              });
              //debugPrint('DESPUÉS DE GUARDAR: ${box.toMap()}');

              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(content: Text('Preset guardado')),
              );
            },
            child: const Text(
              "Guardar",
              style: TextStyle(color: Colors.white),
            ),
          )
        ],
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
            const SizedBox(height: 12),
            _buildEffectBlock(context, "Delay"),
            const SizedBox(height: 12),
            _buildEffectBlock(context, "Overdrive"),
            const SizedBox(height: 12),
            _buildEffectBlock(context, "Distortion"),
          ],
        ),
      ),
    );
  }

  Widget _buildEffectBlock(BuildContext context, String title) {
    return GestureDetector(
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
          //debugPrint('VALORES DEVUELTOS DE $title: $result');
          setState(() {
            presetData[title] = result;
          });
          //debugPrint('PRESET DATA ACTUALIZADO: $presetData');
        }
      },
      child: Container(
        height: 80,
        padding: const EdgeInsets.all(16),
        width: double.infinity,
        decoration: BoxDecoration(
          color: Colors.deepPurple[100],
          borderRadius: BorderRadius.circular(12),
        ),
        child: Text(title, style: const TextStyle(fontSize: 18)),
      ),
    );
  }
}
