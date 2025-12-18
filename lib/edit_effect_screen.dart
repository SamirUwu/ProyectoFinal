import 'package:flutter/material.dart';

class EditEffectScreen extends StatefulWidget {
  final String effectName;
  final Map<String, double>? initialValues;
  const EditEffectScreen({super.key, required this.effectName, this.initialValues});

  @override
  State<EditEffectScreen> createState() => _EditEffectScreenState();
}

class EffectParam {
  final String label;
  final double min;
  final double max;

  EffectParam(this.label, this.min, this.max);
}

final Map<String, List<EffectParam>> effectParams = {
  'Delay': [
    EffectParam('TIME', 0, 2000),
    EffectParam('FEEDBACK', 0, 1),
    EffectParam('MIX', 0, 1),
  ],
  'Overdrive': [
    EffectParam('DRIVE', 0, 1),
    EffectParam('TONE', 0, 1),
    EffectParam('LEVEL', 0, 1),
  ],
  'Distortion': [
    EffectParam('GAIN', 0, 1),
    EffectParam('TONE', 0, 1),
    EffectParam('OUTPUT', 0, 1),
  ],
};

class _EditEffectScreenState extends State<EditEffectScreen> {
  late Map<String, double> values;

  @override
  void initState() {
    super.initState();

    final params = effectParams[widget.effectName]!;

    values = {
      for (var p in params)
        p.label: widget.initialValues?[p.label] ?? p.min
    };
  }

  @override
  Widget build(BuildContext context) {
    final params = effectParams[widget.effectName]!;

    return Scaffold(
      appBar: AppBar(
        title: Text('Edit: ${widget.effectName}'),
      ),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Text(
              'Editar parámetros: ${widget.effectName}',
              style: const TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(height: 32),
            Row(
              children: params
                  .map(
                    (param) => Expanded(
                      child: _buildKnob(
                        param.label,
                        values[param.label]!,
                        param.min,
                        param.max,
                        (v) => setState(() {
                          values[param.label] = v;
                        }),
                      ),
                    ),
                  )
                  .toList(),
            ),
            const SizedBox(height: 32),
            ElevatedButton(
              onPressed: () {
                print('Valores aplicados: $values');
                Navigator.pop(context, values);
              },
              child: const Text('APPLY'),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildKnob(
    String label,
    double value,
    double min,
    double max,
    ValueChanged<double> onChanged,
  ) {
    return Column(
      children: [
        Text(label),
        Slider(
          value: value,
          min: min,
          max: max,
          divisions: 100,
          activeColor: Colors.deepPurple,
          inactiveColor: Colors.deepPurple.shade100,
          label: value.toStringAsFixed(2),
          onChanged: onChanged,
        ),
      ],
    );
  }
}
