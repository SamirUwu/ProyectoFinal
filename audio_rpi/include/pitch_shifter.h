#ifndef PITCH_SHIFTER_H
#define PITCH_SHIFTER_H

#include <math.h>

#define SAMPLE_RATE        44100
#define PI                 3.14159265358979323846f
#define PITCH_MAX_DELAY_MS 500
#define MAX_GRAINS         8
#define GRAIN_SIZE_MS      80  // más grande = menos artifacts

typedef struct {
    float writeIndex;
    float grainPhase[MAX_GRAINS];
    float grainReadIndex[MAX_GRAINS];
    float buffer[(SAMPLE_RATE * PITCH_MAX_DELAY_MS) / 1000];
} PitchVoice;

typedef struct {
    // Voz A
    float       semitones_a;
    float       mix_a;
    PitchVoice  voice_a;

    // Voz B
    float       semitones_b;
    float       mix_b;
    PitchVoice  voice_b;

    // Mix global (wet total)
    float       mix;

    int         grainSize;
} PitchShifter;

void  PitchShifter_init(PitchShifter *ps, float semitones, float mix);
float PitchShifter_process(PitchShifter *ps, float input);

#endif