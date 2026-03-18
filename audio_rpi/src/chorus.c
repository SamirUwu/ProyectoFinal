#include <math.h>
#include <string.h>
#include "chorus.h"

#define PI 3.14159265358979323846f
#define MAX_SAMPLES ((int)(CHORUS_MAX_DELAY_MS * SAMPLE_RATE / 1000))
#define NUM_VOICES 3

static float buffer[NUM_VOICES][MAX_SAMPLES];
static int   writeIndex = 0;
static float lfoPhase[NUM_VOICES];

// Fases escalonadas para que las voces no sean idénticas
static const float voicePhaseOffset[NUM_VOICES] = {
    0.0f,
    1.0f / 3.0f,
    2.0f / 3.0f
};

// mid_delay distinto por voz para sonido más rico
static const float voiceMidDelay[NUM_VOICES] = {
    0.014f,   // 14ms
    0.013f,   // 13ms
    0.015f    // 15ms
};

void Chorus_init(Chorus *ch, float rate, float depth, float feedback, float mix)
{
    ch->rate     = rate;
    ch->depth    = depth;
    ch->feedback = feedback;
    ch->mix      = mix;
    memset(buffer, 0, sizeof(buffer));
    writeIndex = 0;
    for (int v = 0; v < NUM_VOICES; v++)
        lfoPhase[v] = voicePhaseOffset[v];
}

float Chorus_process(Chorus *ch, float input)
{
    // RATE: 0.1-3 Hz directo, sin escala cuadrática
    float rate = ch->rate;
    if (rate < 0.1f) rate = 0.1f;
    if (rate > 10.0f) rate = 10.0f;

    // FEEDBACK: máximo 0.25 como dice la referencia
    float feedback = ch->feedback * 0.25f;

    // DEPTH: controla desviación ±10ms alrededor del mid_delay
    float depthSamples = ch->depth * 0.010f * SAMPLE_RATE;  // ±10ms

    float wet = 0.0f;

    for (int v = 0; v < NUM_VOICES; v++) {
        // delay = mid_delay + depth * 10ms * sin(lfo)
        float delaySamples = voiceMidDelay[v] * SAMPLE_RATE
                           + sinf(2.0f * PI * lfoPhase[v]) * depthSamples;

        float readPos = (float)writeIndex - delaySamples;
        while (readPos < 0)            readPos += MAX_SAMPLES;
        while (readPos >= MAX_SAMPLES) readPos -= MAX_SAMPLES;

        int   i1      = (int)readPos % MAX_SAMPLES;
        int   i2      = (i1 + 1) % MAX_SAMPLES;
        float fr      = readPos - floorf(readPos);
        float delayed = buffer[v][i1] * (1.0f - fr) + buffer[v][i2] * fr;

        wet += delayed;

        lfoPhase[v] += rate / SAMPLE_RATE;
        if (lfoPhase[v] >= 1.0f) lfoPhase[v] -= 1.0f;
    }

    wet /= NUM_VOICES;

    // Escribir con feedback sobre wet mezclado (no por voz)
    for (int v = 0; v < NUM_VOICES; v++)
        buffer[v][writeIndex] = input + wet * feedback;

    writeIndex = (writeIndex + 1) % MAX_SAMPLES;

    return input * (1.0f - ch->mix) + wet * ch->mix;
}