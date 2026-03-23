#include "daisysp.h"
#include "pitch_shifter.h"

static daisysp::PitchShifter g_ps_a;
static daisysp::PitchShifter g_ps_b;

extern "C" void PitchShifter_init(PitchShifter *ps, float semitones, float mix)
{
    ps->semitones_a = semitones;
    ps->semitones_b = 0.0f;
    ps->mix_a       = 1.0f;
    ps->mix_b       = 0.0f;
    ps->mix         = mix;
    ps->grainSize   = 0;

    g_ps_a.Init(44100.f);
    g_ps_b.Init(44100.f);
    g_ps_a.SetTransposition(semitones);
    g_ps_b.SetTransposition(0.0f);
    g_ps_a.SetFun(0.0f);
    g_ps_b.SetFun(0.0f);
}

extern "C" float PitchShifter_process(PitchShifter *ps, float input)
{
    g_ps_a.SetTransposition(ps->semitones_a);
    g_ps_b.SetTransposition(ps->semitones_b);

    float wet_a = g_ps_a.Process(input) * ps->mix_a;
    float wet_b = g_ps_b.Process(input) * ps->mix_b;

    float total_mix = ps->mix_a + ps->mix_b;
    float wet = total_mix > 0.01f ? (wet_a + wet_b) / total_mix : 0.f;

    return input * (1.f - ps->mix) + wet * ps->mix;
}