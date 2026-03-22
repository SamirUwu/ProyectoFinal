#include "pitch_shifter.h"
#include <string.h>
#include <math.h>

#define BUFFER_SIZE ((SAMPLE_RATE * PITCH_MAX_DELAY_MS) / 1000)

static inline float hanning(float x)
{
    if (x < 0.0f) x = 0.0f;
    if (x > 1.0f) x = 1.0f;
    return 0.5f * (1.0f - cosf(2.0f * PI * x));
}

static void voice_init(PitchVoice *v, int grainSize)
{
    memset(v->buffer, 0, sizeof(v->buffer));
    v->writeIndex = 0;
    for (int g = 0; g < MAX_GRAINS; g++) {
        v->grainPhase[g]     = (float)g / MAX_GRAINS;
        v->grainReadIndex[g] = (float)(g * grainSize / MAX_GRAINS);
    }
}

static float voice_process(PitchVoice *v, float input, float pitchFactor, int grainSize)
{
    // Escribir en buffer
    int wi = (int)v->writeIndex % BUFFER_SIZE;
    v->buffer[wi] = input;

    float output    = 0.0f;
    float windowSum = 0.0f;

    for (int g = 0; g < MAX_GRAINS; g++)
    {
        v->grainPhase[g] += 1.0f / grainSize;

        if (v->grainPhase[g] >= 1.0f) {
            v->grainPhase[g] -= 1.0f;
            // Reiniciar con offset aleatorio pequeño para evitar phase coherence
            int offset = (g * grainSize / MAX_GRAINS);
            v->grainReadIndex[g] = (float)((wi - offset + BUFFER_SIZE) % BUFFER_SIZE);
        }

        // Interpolación lineal
        float ri = v->grainReadIndex[g];
        while (ri < 0)           ri += BUFFER_SIZE;
        while (ri >= BUFFER_SIZE) ri -= BUFFER_SIZE;

        int   i1   = (int)ri % BUFFER_SIZE;
        int   i2   = (i1 + 1) % BUFFER_SIZE;
        float frac = ri - floorf(ri);
        float s    = v->buffer[i1] * (1.0f - frac) + v->buffer[i2] * frac;

        float win   = hanning(v->grainPhase[g]);
        output     += s * win;
        windowSum  += win;

        v->grainReadIndex[g] += pitchFactor;
        if (v->grainReadIndex[g] >= BUFFER_SIZE)
            v->grainReadIndex[g] -= BUFFER_SIZE;
        if (v->grainReadIndex[g] < 0)
            v->grainReadIndex[g] += BUFFER_SIZE;
    }

    if (windowSum > 0.01f)
        output /= windowSum;

    v->writeIndex++;
    if (v->writeIndex >= BUFFER_SIZE)
        v->writeIndex = 0;

    return output;
}

void PitchShifter_init(PitchShifter *ps, float semitones, float mix)
{
    ps->semitones_a = semitones;
    ps->semitones_b = 0.0f;
    ps->mix_a       = 1.0f;
    ps->mix_b       = 0.0f;
    ps->mix         = mix;
    ps->grainSize   = (GRAIN_SIZE_MS * SAMPLE_RATE) / 1000;

    voice_init(&ps->voice_a, ps->grainSize);
    voice_init(&ps->voice_b, ps->grainSize);
}

float PitchShifter_process(PitchShifter *ps, float input)
{
    float pf_a = powf(2.0f, ps->semitones_a / 12.0f);
    float pf_b = powf(2.0f, ps->semitones_b / 12.0f);

    float out_a = voice_process(&ps->voice_a, input, pf_a, ps->grainSize);
    float out_b = voice_process(&ps->voice_b, input, pf_b, ps->grainSize);

    float wet = out_a * ps->mix_a + out_b * ps->mix_b;

    return input * (1.0f - ps->mix) + wet * ps->mix;
}