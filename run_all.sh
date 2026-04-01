#!/bin/bash

cleanup() {
    echo ""
    echo "Deteniendo procesos..."
    kill $AUDIO_PID $GUI_PID 2>/dev/null
    sudo pkill -9 -f audio_engine 2>/dev/null
    pkill -9 -f main.py 2>/dev/null
    exit 0
}
trap cleanup SIGINT SIGTERM

echo "Performance governor..."
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor > /dev/null

echo "Limpiando procesos viejos..."
pkill -f audio_engine
pkill -f main.py
sleep 0.5

# Ruta base absoluta — siempre correcta sin importar desde dónde corras el script
PROJECT_DIR="/home/raspi/ProyectoFinal/ProyectoFinal"
VENV="$PROJECT_DIR/Interfaz/venv/bin/activate"

# ── Audio engine ──────────────────────────────────────────────────────────────
cd "$PROJECT_DIR/audio_rpi"
echo "Iniciando servidor C..."
taskset 0x1 ./audio_engine &
AUDIO_PID=$!

echo "Esperando socket..."
for i in $(seq 1 20); do
    [ -S /tmp/audio_socket ] && break
    sleep 0.1
done

if [ ! -S /tmp/audio_socket ]; then
    echo "ERROR: socket no apareció, revisar audio_engine"
    kill $AUDIO_PID 2>/dev/null
    exit 1
fi
echo "Socket listo."

# ── Python GUI ────────────────────────────────────────────────────────────────
cd "$PROJECT_DIR/Interfaz"

if [ ! -f "$VENV" ]; then
    echo "ERROR: venv no encontrado en $VENV"
    kill $AUDIO_PID 2>/dev/null
    exit 1
fi

echo "Activando venv: $VENV"
source "$VENV"

echo "Iniciando interfaz..."
taskset 0x6 python main.py &
GUI_PID=$!

wait