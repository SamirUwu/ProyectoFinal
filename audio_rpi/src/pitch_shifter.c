#include "pitch_shifter.h"
#include <string.h>
#include <math.h>

static inline float hanning(float x)
{
    // x debe estar en [0, 1]
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

    // Repartir granos uniformemente en el grainSize completo
    for (int g = 0; g < MAX_GRAINS; g++)
        ps->grainOffsets[g] = g * (ps->grainSize / MAX_GRAINS);
}

float PitchShifter_process(PitchShifter *ps, float input)
{
    int bufferSize = (SAMPLE_RATE * PITCH_MAX_DELAY_MS) / 1000;

    ps->pitchFactor = powf(2.0f, ps->semitones / 12.0f);
    ps->buffer[ps->writeIndex] = input;

    float output = 0.0f;

    for (int g = 0; g < MAX_GRAINS; g++)
    {
        // Desplazamiento en samples escalado por pitchFactor
        float offset    = (float)ps->grainOffsets[g] * ps->pitchFactor;
        float readIndex = (float)ps->writeIndex - offset;

        // Wrap positivo
        while (readIndex < 0)          readIndex += bufferSize;
        while (readIndex >= bufferSize) readIndex -= bufferSize;

        // Interpolacion lineal
        int   i1   = (int)readIndex;
        int   i2   = (i1 + 1) % bufferSize;
        float frac = readIndex - floorf(readIndex);
        float s    = ps->buffer[i1] * (1.0f - frac) + ps->buffer[i2] * frac;

        // Ventana Hanning por posicion dentro del grano
        int posInt     = ((ps->writeIndex - ps->grainOffsets[g])
                          % ps->grainSize + ps->grainSize) % ps->grainSize;
        float grainPos = (float)posInt / (float)ps->grainSize;
        s *= hanning(grainPos);

        output += s;
    }

    // Normalizar por numero de granos
    output /= MAX_GRAINS;

    ps->writeIndex++;
    if (ps->writeIndex >= bufferSize)
        ps->writeIndex = 0;

    return input * (1.0f - ps->mix) + output * ps->mix;
}
