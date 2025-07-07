import 'package:flutter/material.dart';
import 'package:flutter_knob/flutter_knob.dart';

class EditEffectScreen extends StatefulWidget {
  final String effectName;

  const EditEffectScreen({super.key, required this.effectName});

  @override
  State<EditEffectScreen> createState() => _EditEffectScreenState();
}

class _EditEffectScreenState extends State<EditEffectScreen> {
  double time = 0;
  double feedback = 0;
  double mix = 0;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Edit: ${widget.effectName}'),
      ),
      body: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          const Text(
            'Editar parámetros: Delay',
            style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
          ),
          const SizedBox(height: 32),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _buildKnob('TIME', time, (v) => setState(() => time = v)),
              _buildKnob('FEEDBACK', feedback, (v) => setState(() => feedback = v)),
              _buildKnob('MIX', mix, (v) => setState(() => mix = v)),
            ],
          ),
          const SizedBox(height: 32),
          ElevatedButton(
            onPressed: () {
              print('Valores aplicados: time=$time, feedback=$feedback, mix=$mix');
              Navigator.pop(context); // A futuro podrías guardar los cambios
            },
            child: const Text('APPLY'),
          ),
        ],
      ),
    );
  }

  Widget _buildKnob(String label, double value, ValueChanged<double> onChanged) {
    return Column(
      children: [
        Knob(
          value: value,
          min: 0,
          max: 10,
          onChanged: onChanged,
          color: Colors.deepPurple,
          size: 70,
        ),
        const SizedBox(height: 8),
        Text(label),
      ],
    );
  }
}
