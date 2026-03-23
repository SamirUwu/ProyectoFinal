#include <math.h>
#include <string.h>
#include "chorus.h"

#define PI        3.14159265358979323846f
#define MAX_DELAY ((int)(CHORUS_MAX_DELAY_MS * SAMPLE_RATE / 1000))

// ── Delay line con interpolación lineal ──────────────────────────────────────
typedef struct {
    float buf[MAX_DELAY];
    int   head;
    float delay;
} DelayLine;

static void dl_init(DelayLine *d) {
    memset(d->buf, 0, sizeof(d->buf));
    d->head  = 0;
    d->delay = 0.f;
}

static void dl_set_delay(DelayLine *d, float delay_samples) {
    if (delay_samples < 0.f)         delay_samples = 0.f;
    if (delay_samples > MAX_DELAY-2) delay_samples = (float)(MAX_DELAY-2);
    d->delay = delay_samples;
}

static float dl_read(DelayLine *d) {
    float pos = (float)d->head - d->delay;
    while (pos < 0)          pos += MAX_DELAY;
    while (pos >= MAX_DELAY) pos -= MAX_DELAY;
    int   i1   = (int)pos % MAX_DELAY;
    int   i2   = (i1 + 1) % MAX_DELAY;
    float frac = pos - floorf(pos);
    return d->buf[i1] * (1.f - frac) + d->buf[i2] * frac;
}

static void dl_write(DelayLine *d, float val) {
    d->buf[d->head] = val;
    d->head = (d->head + 1) % MAX_DELAY;
}

// ── Engine (un canal, igual que ChorusEngine de DaisySP) ─────────────────────
typedef struct {
    DelayLine del;
    float     lfo_phase;
    float     lfo_freq;
    float     lfo_amp;
    float     delay_base;
    float     feedback;
} ChorusEngine;

static void engine_init(ChorusEngine *e, float phase_offset) {
    dl_init(&e->del);
    e->feedback   = 0.2f;
    e->lfo_phase  = phase_offset;
    e->delay_base = MAX_DELAY * 0.25f;  // ~25% del max delay como base
    e->lfo_amp    = 0.9f * e->delay_base;
    e->lfo_freq   = 4.f * 0.3f / (float)SAMPLE_RATE;
}

static float engine_process(ChorusEngine *e, float in) {
    // LFO triangular (igual que DaisySP)
    e->lfo_phase += e->lfo_freq;
    if (e->lfo_phase > 0.25f || e->lfo_phase < -0.25f)
        e->lfo_freq = -e->lfo_freq;

    float lfo_sig = e->lfo_phase * e->lfo_amp * 4.f;

    dl_set_delay(&e->del, lfo_sig + e->delay_base);

    float out = dl_read(&e->del);
    dl_write(&e->del, in + out * e->feedback);

    return (in + out) * 0.5f;  // equal mix — igual que DaisySP
}

// ── Dos engines con fase offset para ancho estéreo ───────────────────────────
static ChorusEngine eng_l, eng_r;

void Chorus_init(Chorus *ch, float rate, float depth, float feedback, float mix)
{
    ch->rate     = rate;
    ch->depth    = depth;
    ch->feedback = feedback;
    ch->mix      = mix;

    engine_init(&eng_l, 0.f);
    engine_init(&eng_r, 0.5f);  // 180° offset → stereo width
}

float Chorus_process(Chorus *ch, float input)
{
    // LFO freq: igual que ChorusModule (cuadrático para rango más musical)
    float lfo_freq_min = 1.0f, lfo_freq_max = 20.0f;
    float freq = lfo_freq_min + ch->rate * ch->rate * (lfo_freq_max - lfo_freq_min);
    float lfo_inc = 4.f * freq / (float)SAMPLE_RATE;

    eng_l.lfo_freq = eng_l.lfo_freq < 0.f ? -lfo_inc : lfo_inc;
    eng_r.lfo_freq = eng_r.lfo_freq < 0.f ? -lfo_inc : lfo_inc;

    // depth → lfo_amp relativo al delay base
    float depth = ch->depth > 0.93f ? 0.93f : ch->depth;
    eng_l.lfo_amp = depth * eng_l.delay_base;
    eng_r.lfo_amp = depth * eng_r.delay_base;

    eng_l.feedback = ch->feedback * 0.95f;
    eng_r.feedback = ch->feedback * 0.95f;

    float out_l = engine_process(&eng_l, input);
    float out_r = engine_process(&eng_r, input);

    float wet = (out_l + out_r) * 0.5f;
    return input * (1.f - ch->mix) + wet * ch->mix;
}