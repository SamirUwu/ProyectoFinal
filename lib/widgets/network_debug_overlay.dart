import 'dart:async';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import '../network_service.dart';

/// Envuelve cualquier widget y muestra un panel de debug en la parte inferior.
/// Solo se activa en kDebugMode.
///
/// Uso:
///   home: kDebugMode
///       ? NetworkDebugOverlay(child: WelcomeScreen())
///       : WelcomeScreen(),
class NetworkDebugOverlay extends StatefulWidget {
  final Widget child;
  const NetworkDebugOverlay({super.key, required this.child});

  @override
  State<NetworkDebugOverlay> createState() => _NetworkDebugOverlayState();
}

class _NetworkDebugOverlayState extends State<NetworkDebugOverlay> {
  final List<_LogEntry> _logs = [];
  bool _expanded = true;
  int _fails = 0;
  int _sent = 0;
  Timer? _pingTimer;

  @override
  void initState() {
    super.initState();
    NetworkDebugLogger.init(_onLog);
    _pingTimer = Timer.periodic(const Duration(seconds: 2), (_) {
      if (mounted) setState(() {});
    });
  }

  void _onLog(String level, String msg) {
    if (!mounted) return;
    setState(() {
      _logs.insert(0, _LogEntry(level, msg, DateTime.now()));
      if (_logs.length > 100) _logs.removeLast();
      if (level == 'ERR') _fails++;
      if (level == 'SEND') _sent++;
    });
  }

  @override
  void dispose() {
    _pingTimer?.cancel();
    NetworkDebugLogger.dispose();
    super.dispose();
  }

  String get _status => NetworkService.isConnected ? 'CONNECTED' : 'DISCONNECTED';
  Color get _statusColor => NetworkService.isConnected ? const Color(0xFF50FA7B) : const Color(0xFFFF5555);

  @override
  Widget build(BuildContext context) {
    if (!kDebugMode) return widget.child;

    return Stack(
      children: [
        widget.child,
        Positioned(
          bottom: 0, left: 0, right: 0,
          child: Material(
            color: Colors.transparent,
            child: Container(
              decoration: const BoxDecoration(
                color: Color(0xF2000000),
                border: Border(top: BorderSide(color: Color(0xFFFF6B35), width: 1.5)),
              ),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  // ── Header ──
                  GestureDetector(
                    onTap: () => setState(() => _expanded = !_expanded),
                    child: Container(
                      color: const Color(0xFFFF6B35),
                      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 5),
                      child: Row(
                        children: [
                          const Text('NET DEBUG',
                              style: TextStyle(
                                  color: Colors.black, fontSize: 11,
                                  fontWeight: FontWeight.bold,
                                  letterSpacing: 1.2,
                                  fontFamily: 'monospace')),
                          const SizedBox(width: 8),
                          _StatusBadge(_status, _statusColor),
                          const Spacer(),
                          GestureDetector(
                            onTap: () => setState(() { _logs.clear(); _fails = 0; _sent = 0; }),
                            child: const Text('CLEAR',
                                style: TextStyle(color: Colors.black54, fontSize: 10,
                                    fontFamily: 'monospace')),
                          ),
                          const SizedBox(width: 12),
                          Text(_expanded ? '▼' : '▲',
                              style: const TextStyle(color: Colors.black, fontSize: 12)),
                        ],
                      ),
                    ),
                  ),
                  // ── Body ──
                  if (_expanded) ...[
                    Padding(
                      padding: const EdgeInsets.fromLTRB(8, 8, 8, 4),
                      child: Row(
                        children: [
                          _Stat('HOST',
                              NetworkService.host.isEmpty ? '(vacío!)' : NetworkService.host,
                              NetworkService.host.isEmpty ? const Color(0xFFFF5555) : const Color(0xFF8BE9FD)),
                          _Stat('PORT', '5000', Colors.white70),
                          _Stat('FAILS', '$_fails', const Color(0xFFFFB86C)),
                          _Stat('SENT', '$_sent', const Color(0xFF50FA7B)),
                        ],
                      ),
                    ),
                    SizedBox(
                      height: 160,
                      child: _logs.isEmpty
                          ? const Center(
                              child: Text('Sin logs aún...',
                                  style: TextStyle(color: Colors.white38,
                                      fontSize: 11, fontFamily: 'monospace')))
                          : ListView.builder(
                              padding: const EdgeInsets.symmetric(horizontal: 8),
                              itemCount: _logs.length,
                              itemBuilder: (_, i) => _LogLine(_logs[i]),
                            ),
                    ),
                    const SizedBox(height: 6),
                  ],
                ],
              ),
            ),
          ),
        ),
      ],
    );
  }
}

class _StatusBadge extends StatelessWidget {
  final String label;
  final Color color;
  const _StatusBadge(this.label, this.color);

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 2),
      decoration: BoxDecoration(
        color: Colors.black26,
        borderRadius: BorderRadius.circular(3),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(width: 6, height: 6,
              decoration: BoxDecoration(color: color, shape: BoxShape.circle)),
          const SizedBox(width: 4),
          Text(label,
              style: TextStyle(color: color, fontSize: 10,
                  fontWeight: FontWeight.bold, fontFamily: 'monospace')),
        ],
      ),
    );
  }
}

class _Stat extends StatelessWidget {
  final String label, value;
  final Color valueColor;
  const _Stat(this.label, this.value, this.valueColor);

  @override
  Widget build(BuildContext context) {
    return Expanded(
      child: Container(
        margin: const EdgeInsets.symmetric(horizontal: 3),
        padding: const EdgeInsets.symmetric(vertical: 6),
        decoration: BoxDecoration(
          color: const Color(0xFF111111),
          borderRadius: BorderRadius.circular(4),
        ),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(value,
                overflow: TextOverflow.ellipsis,
                style: TextStyle(color: valueColor, fontSize: 10,
                    fontWeight: FontWeight.bold, fontFamily: 'monospace')),
            const SizedBox(height: 2),
            Text(label,
                style: const TextStyle(color: Color(0xFF555555), fontSize: 8,
                    letterSpacing: 0.8, fontFamily: 'monospace')),
          ],
        ),
      ),
    );
  }
}

class _LogLine extends StatelessWidget {
  final _LogEntry entry;
  const _LogLine(this.entry);

  Color get _color => switch (entry.level) {
    'ERR'  => const Color(0xFFFF5555),
    'OK'   => const Color(0xFF50FA7B),
    'WARN' => const Color(0xFFFFB86C),
    'SEND' => const Color(0xFF8BE9FD),
    _      => const Color(0xFFAAAAAA),
  };

  @override
  Widget build(BuildContext context) {
    final t = entry.time;
    final ts =
        '${t.hour.toString().padLeft(2, '0')}:${t.minute.toString().padLeft(2, '0')}:${t.second.toString().padLeft(2, '0')}';
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 1.5),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text('$ts ', style: const TextStyle(color: Color(0xFF555555),
              fontSize: 9, fontFamily: 'monospace')),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 4),
            margin: const EdgeInsets.only(right: 5, top: 1),
            decoration: BoxDecoration(
              color: _color.withOpacity(0.15),
              borderRadius: BorderRadius.circular(2),
            ),
            child: Text(entry.level,
                style: TextStyle(color: _color, fontSize: 8,
                    fontWeight: FontWeight.bold, fontFamily: 'monospace')),
          ),
          Expanded(
            child: Text(entry.msg,
                softWrap: true,
                style: TextStyle(color: _color, fontSize: 10,
                    fontFamily: 'monospace')),
          ),
        ],
      ),
    );
  }
}

class _LogEntry {
  final String level, msg;
  final DateTime time;
  _LogEntry(this.level, this.msg, this.time);
}