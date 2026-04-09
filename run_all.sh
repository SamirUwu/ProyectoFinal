#!/bin/bash

cleanup() {
  echo ""
  echo "Deteniendo todos los procesos..."
  kill $AUDIO_PID $FEEDER_PID $GUI_PID 2>/dev/null
  wait $AUDIO_PID $FEEDER_PID $GUI_PID 2>/dev/null
  rm -f /tmp/ni6009_pipe
  echo "Listo."
  exit 0
}
trap cleanup SIGINT SIGTERM

PROJECT_DIR="/home/raspi/ProyectoFinal/ProyectoFinal"

# ── Argumentos opcionales ─────────────────────────────────────────────────────
NI_DEVICE="${1:-Dev1}" # ej: ./run_all.sh Dev2
NI_CHANNEL="${2:-ai0}" # ej: ./run_all.sh Dev1 ai1
NI_MODE="${3:-rse}"    # rse | diff

echo "============================================"
echo "  MultiFX Processor"
echo "  NI Device : $NI_DEVICE / $NI_CHANNEL ($NI_MODE)"
echo "============================================"

# ── Limpiar procesos anteriores ───────────────────────────────────────────────
pkill -f audio_engine
pkill -f ni6009_feeder.py
pkill -f main.py
sleep 0.5

# ── Crear el pipe antes de arrancar cualquier proceso ────────────────────────
rm -f /tmp/ni6009_pipe
mkfifo /tmp/ni6009_pipe
echo "[1/3] Pipe creado en /tmp/ni6009_pipe"

# ── Arrancar el feeder de NI USB-6009 (en segundo plano) ─────────────────────
cd "$PROJECT_DIR/Interfaz"
source venv/bin/activate
python ni6009_feeder.py \
  --device "$NI_DEVICE" \
  --channel "$NI_CHANNEL" \
  --mode "$NI_MODE" &
FEEDER_PID=$!
echo "[2/3] Feeder NI USB-6009 iniciado (PID $FEEDER_PID)"

# ── Arrancar el motor de audio C (abre el pipe en modo lectura) ───────────────
cd "$PROJECT_DIR/audio_rpi"
./audio_engine &
AUDIO_PID=$!
echo "[3/3] Motor de audio iniciado (PID $AUDIO_PID)"

# ── Esperar a que el socket Unix esté listo ───────────────────────────────────
echo "Esperando socket de audio..."
TIMEOUT=15
COUNT=0
until [ -S /tmp/audio_socket ]; do
  sleep 0.1
  COUNT=$((COUNT + 1))
  if [ $COUNT -ge $((TIMEOUT * 10)) ]; then
    echo "ERROR: el socket no apareció en ${TIMEOUT}s — revisa el motor de audio."
    cleanup
  fi
done
echo "Socket listo."

# ── Arrancar la GUI Python ────────────────────────────────────────────────────
cd "$PROJECT_DIR/Interfaz"
python main.py &
GUI_PID=$!
echo ""
echo "Todo corriendo. Ctrl+C para detener."
echo "  Audio engine : PID $AUDIO_PID"
echo "  NI Feeder    : PID $FEEDER_PID"
echo "  GUI          : PID $GUI_PID"
echo ""

wait

