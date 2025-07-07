import 'package:flutter/material.dart';
import 'edit_effect_screen.dart';

class EditPresetScreen extends StatelessWidget {
  final String presetName;

  const EditPresetScreen({super.key, required this.presetName});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text("Editar Preset"),
        leading: IconButton(
          icon: const Icon(Icons.arrow_back),
          onPressed: () => Navigator.pop(context),
        ),
        actions: [
          TextButton(
            onPressed: () {
              // A futuro: Guardar preset
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
              "Preset: $presetName",
              style: const TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 20),
            _buildEffectBlock(context, "Effect 1"),
            const SizedBox(height: 12),
            _buildEffectBlock(context, "Effect 2"),
            const SizedBox(height: 12),
            _buildEffectBlock(context, "Effect 3"),
          ],
        ),
      ),
    );
  }

  Widget _buildEffectBlock(BuildContext context, String title) {
    return GestureDetector(
      onTap: () {
        Navigator.push(
          context,
          MaterialPageRoute(
            builder: (context) => EditEffectScreen(effectName: title),
          ),
        );
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