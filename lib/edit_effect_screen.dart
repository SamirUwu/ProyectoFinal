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
  'Overdrive': [
    EffectParam('GAIN',   0,   1),   
    EffectParam('TONE',   0,   1),
    EffectParam('OUTPUT', 0,   1),    
  ],
  //'Distortion': [
  //  EffectParam('GAIN', 0, 1),
  //  EffectParam('TONE', 0, 1),
  //  EffectParam('OUTPUT', 0, 1),
  //],
  'Delay': [
    EffectParam('TIME',     1, 1000),
    EffectParam('FEEDBACK', 0, 1),
    EffectParam('MIX',      0, 0.3),    
  ],
  'Wah': [
    EffectParam('FREQ',  300, 4000),  
    EffectParam('Q',     0.1, 10),
    EffectParam('LEVEL', 0,   1),
  ],
  'Flanger': [
    EffectParam('RATE',     0.1, 3),  
    EffectParam('DEPTH',    0,   1),
    EffectParam('FEEDBACK', 0,   1),
    EffectParam('MIX',      0,   0.3),
  ],
  'Chorus': [
    EffectParam('RATE',     0.1, 3),  
    EffectParam('DEPTH',    0,   1),
    EffectParam('FEEDBACK', 0,   1),  
    EffectParam('MIX',      0,   0.3),
  ],
  'Phaser': [                         
    EffectParam('RATE',     0.1, 3),
    EffectParam('DEPTH',    0,   1),
    EffectParam('FEEDBACK', 0,   1),
    EffectParam('MIX',      0,   0.3),
  ],
  'PitchShifter': [                   
    EffectParam('SEMITONES',   -12, 12),
    EffectParam('SEMITONES_B', -12, 12),
    EffectParam('MIX_A',       0,   1),
    EffectParam('MIX_B',       0,   1),
    EffectParam('MIX',         0,   1),
  ],
  'Reverb': [                        
    EffectParam('FEEDBACK', 0,    1),
    EffectParam('LPFREQ',   2000, 10000),
    EffectParam('MIX',      0,    0.3),
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
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              widget.effectName.toUpperCase(),
              style: const TextStyle(
                fontSize: 26,
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(height: 6),
            const Text(
              'Ajusta los parámetros del efecto',
              style: TextStyle(color: Colors.grey),
            ),
            const SizedBox(height: 24),

            Expanded(
              child: GridView.count(
                crossAxisCount: 2,
                crossAxisSpacing: 16,
                mainAxisSpacing: 16,
                children: params.map((param) {
                  return _buildParamCard(
                    param.label,
                    values[param.label]!,
                    param.min,
                    param.max,
                    (v) => setState(() {
                      values[param.label] = v;
                    }),
                  );
                }).toList(),
              ),
            ),

            const SizedBox(height: 16),

            SizedBox(
              width: double.infinity,
              height: 52,
              child: ElevatedButton(
                onPressed: () {
                  Navigator.pop(context, values);
                },
                child: const Text(
                  'APLICAR',
                  style: TextStyle(fontSize: 18),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildParamCard(
    String label,
    double value,
    double min,
    double max,
    ValueChanged<double> onChanged,
  ) {
    return Container(
      decoration: BoxDecoration(
        color: const Color.fromARGB(255, 128, 92, 194),
        borderRadius: BorderRadius.circular(16),
      ),
      padding: const EdgeInsets.all(12),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Text(
            label,
            style: const TextStyle(
              fontSize: 16,
              fontWeight: FontWeight.bold,
            ),
          ),
          const SizedBox(height: 12),

          Text(
            value.toStringAsFixed(2),
            style: const TextStyle(
              fontSize: 20,
              color: Color.fromARGB(255, 255, 255, 255),
            ),
          ),

          const SizedBox(height: 12),

          Slider(
            value: value,
            min: min,
            max: max,
            divisions: 100,
            activeColor: const Color.fromARGB(255, 85, 77, 100),
            onChanged: onChanged,
          ),
        ],
      ),
    );
  }
}
