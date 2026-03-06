#include "overdrive.h"
#include <math.h>

// Inicializa parámetros
void Overdrive_init(Overdrive *od, float gain, float tone, float output) {
    od->gain = gain;
    od->tone = tone;
    od->output = output;
}

float Overdrive_process(Overdrive *od, float input) {

    float sig = input * od->gain;

    // soft clipping
    sig = tanhf(sig);

    // mezcla con señal limpia (tone)
    sig = sig * od->tone + input * (1.0f - od->tone);

    // salida
    sig *= od->output;

    return sig;
}