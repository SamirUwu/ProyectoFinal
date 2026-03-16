#include "pitch_shifter.h"
#include <string.h>
#include <math.h>

static inline float hanning(float x)
{
    if (x < 0.0f) x = 0.0f;
    if (x > 1.0f) x = 1.0f;
    return 0.5f * (1.0f - cosf(2.0f * PI * x));
}

void PitchShifter_init(PitchShifter *ps, float semitones, float mix)
{
    ps->semitones   = semitones;
    ps->pitchFactor = powf(2.0f, semitones / 12.0f);
    ps->mix         = mix;
    ps->writeIndex  = 0;
    ps->grainSize   = (GRAIN_SIZE_MS * SAMPLE_RATE) / 1000;

    int bufferSize = (SAMPLE_RATE * PITCH_MAX_DELAY_MS) / 1000;
    for (int i = 0; i < bufferSize; i++)
        ps->buffer[i] = 0.0f;

    // Cada grano empieza en una fase distinta uniformemente distribuida
    // para que siempre haya cobertura continua sin huecos
    for (int g = 0; g < MAX_GRAINS; g++) {
        ps->grainPhase[g]     = (float)g / MAX_GRAINS;  // fase [0,1] escalonada
        ps->grainReadIndex[g] = (float)(g * ps->grainSize / MAX_GRAINS);
    }
}

float PitchShifter_process(PitchShifter *ps, float input)
{
    int bufferSize = (SAMPLE_RATE * PITCH_MAX_DELAY_MS) / 1000;

    ps->pitchFactor = powf(2.0f, ps->semitones / 12.0f);

    // Escribir input en el buffer circular
    ps->buffer[ps->writeIndex] = input;

    float output    = 0.0f;
    float windowSum = 0.0f;

    for (int g = 0; g < MAX_GRAINS; g++)
    {
        // grainPhase avanza a 1/grainSize por sample
        ps->grainPhase[g] += 1.0f / ps->grainSize;

        // Cuando el grano termina, reiniciarlo desde el writeIndex actual
        if (ps->grainPhase[g] >= 1.0f) {
            ps->grainPhase[g]     -= 1.0f;
            // El nuevo grano empieza a leer desde writeIndex
            // con un pequeño offset para evitar que todos los granos
            // lean exactamente lo mismo
            ps->grainReadIndex[g]  = (float)ps->writeIndex;
        }

        // Leer del buffer con interpolacion lineal
        float ri   = ps->grainReadIndex[g];
        while (ri < 0)          ri += bufferSize;
        while (ri >= bufferSize) ri -= bufferSize;

        int   i1   = (int)ri % bufferSize;
        int   i2   = (i1 + 1) % bufferSize;
        float frac = ri - floorf(ri);
        float s    = ps->buffer[i1] * (1.0f - frac) + ps->buffer[i2] * frac;

        // Ventana Hanning sobre la fase del grano
        float win  = hanning(ps->grainPhase[g]);
        output    += s * win;
        windowSum += win;

        // Avanzar el read index a velocidad pitchFactor
        // pitchFactor > 1 → lee más rápido → pitch sube
        // pitchFactor < 1 → lee más lento  → pitch baja
        ps->grainReadIndex[g] += ps->pitchFactor;
        if (ps->grainReadIndex[g] >= bufferSize)
            ps->grainReadIndex[g] -= bufferSize;
        if (ps->grainReadIndex[g] < 0)
            ps->grainReadIndex[g] += bufferSize;
    }

    // Normalizar por suma real de ventanas
    if (windowSum > 0.01f)
        output /= windowSum;

    ps->writeIndex++;
    if (ps->writeIndex >= bufferSize)
        ps->writeIndex = 0;

    return input * (1.0f - ps->mix) + output * ps->mix;
}
