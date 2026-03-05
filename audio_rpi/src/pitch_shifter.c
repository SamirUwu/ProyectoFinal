#include "pitch_shifter.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Ventana Hanning
static inline float hanning(float x)
{
    return 0.5f * (1.0f - cosf(2.0f * PI * x));
}

void PitchShifter_init(PitchShifter *ps, float semitones, float mix)
{
    ps->semitones = semitones;
    ps->pitchFactor = powf(2.0f, semitones / 12.0f);
    ps->mix = mix;
    ps->writeIndex = 0;
    ps->grainSize = (GRAIN_SIZE_MS * SAMPLE_RATE) / 1000;

    int bufferSize = (SAMPLE_RATE * PITCH_MAX_DELAY_MS) / 1000;
    for(int i=0; i<bufferSize; i++)
        ps->buffer[i] = 0.0f;

    for(int g=0; g<MAX_GRAINS; g++)
        ps->grainOffsets[g] = g * (ps->grainSize / MAX_GRAINS);
}

float PitchShifter_process(PitchShifter *ps, float input)
{
    int bufferSize = (SAMPLE_RATE * PITCH_MAX_DELAY_MS) / 1000;
    ps->buffer[ps->writeIndex] = input;

    float output = 0.0f;

    for(int g=0; g<MAX_GRAINS; g++)
    {
        float readIndex = ps->writeIndex - ps->grainOffsets[g];
        if(readIndex < 0)
            readIndex += bufferSize;

        // Aplicar pitch factor
        readIndex /= ps->pitchFactor;

        int index1 = (int)readIndex % bufferSize;
        int index2 = (index1 + 1) % bufferSize;
        float frac = readIndex - floorf(readIndex);

        // Interpolación lineal
        float grainSample = ps->buffer[index1] * (1.0f - frac) + ps->buffer[index2] * frac;

        // Aplicar ventana Hanning según posición del grano
        float grainPos = ((ps->writeIndex - ps->grainOffsets[g]) % ps->grainSize) / (float)ps->grainSize;
        grainSample *= hanning(grainPos);

        // Crossfade entre granos
        float weight = (g + 1) / (float)MAX_GRAINS;
        output += grainSample * weight;
    }

    output /= MAX_GRAINS;

    ps->writeIndex++;
    if(ps->writeIndex >= bufferSize)
        ps->writeIndex = 0;

    return input * (1.0f - ps->mix) + output * ps->mix;
}
