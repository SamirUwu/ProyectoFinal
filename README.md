# Proyecto Final

Este proyecto contiene una interfaz en **PyQt6**, procesamiento de audio y comunicación entre diferentes módulos del sistema.

> **Plataforma:** Windows 10/11 únicamente.  
> El motor de audio C requiere PortAudio (Windows), Winsock2 y MinGW/GCC para compilar.  
> La entrada de audio usa la tarjeta **NI USB-6009** a través de la librería **nidaqmx** (NI-DAQmx driver para Windows).

## Requisitos

- Python 3.10 o superior (Windows)
- [NI-DAQmx driver](https://www.ni.com/en/support/downloads/drivers/download.ni-daq-mx.html) instalado
- [MinGW-w64](https://www.mingw-w64.org/) con `gcc` y `make` en el PATH
- [PortAudio](http://www.portaudio.com/) (`libportaudio.a` / `portaudio.dll`) en el PATH o en el directorio del proyecto

Dependencias de Python:

```
PyQt6
numpy
zeroconf
nidaqmx
pyqtgraph
pywin32
```

## Instalación

Entrar a la carpeta de la interfaz:

```
cd Interfaz
```

Crear el entorno virtual:

```
python -m venv venv
```

Activar el entorno (Windows):

```
venv\Scripts\activate
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

Esto genera `audio_engine.exe` usando MinGW GCC y enlaza con PortAudio y Winsock2.

## Ejecutar todo el sistema

Para correr todos los módulos del proyecto se usa el script de Windows:

```
run_all.bat
```

Opcionalmente se pueden pasar los parámetros del dispositivo NI:

```
run_all.bat Dev1 ai0 rse
```

El script:
1. Inicia el feeder NI USB-6009 (`ni6009_feeder.py`), que crea el named pipe `\\.\pipe\ni6009` y espera al motor de audio.
2. Inicia el motor de audio (`audio_engine.exe`), que se conecta al pipe y abre el servidor TCP en el puerto 54321.
3. Espera a que el socket TCP esté disponible y luego inicia la GUI Python (`main.py`).

## Modos de operación (SIM_MODE en `audio_rpi/src/main.c`)

| Valor | Fuente de audio |
|-------|----------------|
| `1`   | Señal de prueba (suma de senos, sin hardware necesario) |
| `3`   | NI USB-6009 a través de `ni6009_feeder.py` y named pipe de Windows |
| `0`   | ESP32 por puerto COM (serial) |

El valor por defecto es `SIM_MODE 1` para pruebas sin hardware. Cambiar a `3` para usar la NI USB-6009.

## Notas

- Si se actualiza el repositorio con `git pull`, se recomienda volver a ejecutar `make` en `audio_rpi`.
- El entorno virtual debe activarse antes de ejecutar la interfaz. Ver `requirements.txt`.
- Para `SIM_MODE 3`, iniciar `ni6009_feeder.py` **antes** de `audio_engine.exe` (el script `run_all.bat` lo hace automáticamente).
