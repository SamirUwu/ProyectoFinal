#!/bin/bash

echo "Limpiando procesos viejos..."
pkill -f audio_engine
pkill -f main.py
sleep 0.5

# ── C audio engine ────────────────────────────────────────────────────────────
cd audio_rpi
echo "Iniciando servidor C..."

# taskset 0x1 = pin to core 0 exclusively for audio
# sudo needed for SCHED_FIFO + mlockall
sudo taskset 0x1 ./audio_engine &
AUDIO_PID=$!

# Wait until the socket actually exists instead of blind sleep
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
cd ../Interfaz
echo "Activando virtual environment..."
source ../env/bin/activate

# Pin GUI to cores 1-3, leaving core 0 free for audio
echo "Iniciando interfaz..."
taskset 0x6 python main.py &

wait