#!/bin/bash

cleanup() {
    echo "Deteniendo..."
    kill $AUDIO_PID $GUI_PID 2>/dev/null
    exit 0
}
trap cleanup SIGINT SIGTERM

PROJECT_DIR="/home/raspi/ProyectoFinal/ProyectoFinal"

pkill -f audio_engine; pkill -f main.py; sleep 0.5

# Audio engine
cd "$PROJECT_DIR/audio_rpi"
./audio_engine &
AUDIO_PID=$!

# Esperar socket
until [ -S /tmp/audio_socket ]; do sleep 0.1; done
echo "Socket listo."

# Python GUI
cd "$PROJECT_DIR/Interfaz"
source venv/bin/activate
python main.py &
GUI_PID=$!

wait