# Proyecto Final

Este proyecto contiene una interfaz en **PyQt6**, procesamiento de audio y comunicación entre diferentes módulos del sistema.

## Requisitos
Python 3
pip
make


Dependencias de Python:

PyQt6
numpy
zeroconf

## Instalación

Entrar a la carpeta de la interfaz:

```
cd Interfaz
```

Crear el entorno virtual:

```
python3 -m venv venv
```

Activar el entorno:

Linux 

```
source venv/bin/activate
```

Instalar dependencias:

```
pip install -r requirements.txt
```

## Compilar audio (IMPORTANTE)

Cada vez que se haga un **git pull**, es necesario recompilar el módulo de audio.

Entrar a la carpeta:

```
cd audio_rpi
```

y ejecutar:

```
make
```

## Ejecutar todo el sistema

Para correr todos los módulos del proyecto se usa el script:

```
run_all.sh
```

La primera vez hay que darle permisos de ejecución:

```
chmod +x run_all.sh
```

Luego se ejecuta con:
./run_all.sh


## Notas

Si se actualiza el repositorio con `git pull`, se recomienda volver a ejecutar `make` en `audio_rpi`.
El entorno virtual debe activarse antes de ejecutar la interfaz. Leer requirements.txt

---

https://www.mediafire.com/file/2sd98dhd92951xq/RJ+Pasin+-+Free+Guitar+Samples.zip


FIXING RPI4 AUDIO PROBLEM:



Extra one-time RPi4 setup
Run these once on the Pi — they persist across reboots:
Give your binary real-time caps so you don't need sudo every time:
bashsudo setcap cap_sys_nice,cap_ipc_lock+ep ~/your_project/audio_rpi/audio_engine
After this you can remove sudo from the script.

Set CPU governor to performance (prevents frequency scaling stalls):
bashecho performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
Add that line to the top of your run_all.sh to apply it every run.

Quick test to confirm it's WSL, not your code
On the RPi4, run this before anything else:
bashsudo aplay -D default --duration=3 /usr/share/sounds/alsa/Front_Center.wav
