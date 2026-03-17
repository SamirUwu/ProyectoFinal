#include <math.h>
#include "wah.h"

#define PI          3.14159265358979323846f

#define WAH_FREQ_MIN  200.0f    // igual que minscale=200 del Python
#define WAH_FREQ_MAX  2000.0f   // igual que maxscale=2000 del Python

typedef struct {
    float a0, a1, a2, b1, b2;
    float w1, w2;
} Biquad;

static Biquad filter;
static float  lfo_phase = 0.0f;

void Wah_init(Wah *wah, float freq, float q, float level)
{
    wah->freq  = freq;
    wah->q     = q;
    wah->level = level;
    filter.w1  = 0.0f;
    filter.w2  = 0.0f;
    lfo_phase  = 0.0f;
}

static void Wah_updateCoeffs(float center_freq, float q)
{
    float freq = center_freq;
    if (freq < 20.0f)    freq = 20.0f;
    if (freq > 18000.0f) freq = 18000.0f;

    float omega = 2.0f * PI * freq / SAMPLE_RATE;
    float cos_w = cosf(omega);
    float sin_w = sinf(omega);
    float alpha = sin_w / (2.0f * q);   // q=5 igual que el Python

    // Bandpass (pyo type=2, RBJ)
    float norm = 1.0f + alpha;
    filter.a0  =  alpha / norm;
    filter.a1  =  0.0f;
    filter.a2  = -alpha / norm;
    filter.b1  = (-2.0f * cos_w) / norm;
    filter.b2  = (1.0f - alpha)  / norm;
}

float Wah_process(Wah *wah, float input)
{
    // LFO lento, simula el pie moviendo el pedal
    float lfo_rate = wah->freq;
    if (lfo_rate < 0.1f) lfo_rate = 0.1f;
    if (lfo_rate > 4.0f) lfo_rate = 4.0f;

    // Curva logarítmica — el oído percibe frecuencias en escala log,
    // así el sweep suena uniforme igual que un pedal físico
    float lfo_val    = 0.5f * (1.0f + sinf(lfo_phase));   // 0..1 lineal
    float log_sweep  = WAH_FREQ_MIN * powf(WAH_FREQ_MAX / WAH_FREQ_MIN, lfo_val); // log

    lfo_phase += 2.0f * PI * lfo_rate / SAMPLE_RATE;
    if (lfo_phase >= 2.0f * PI) lfo_phase -= 2.0f * PI;

    // Q fijo en 5, igual que el Python — no usar wah->q para esto
    Wah_updateCoeffs(log_sweep, 5.0f);

    float w   = input - filter.b1 * filter.w1 - filter.b2 * filter.w2;
    float out = filter.a0 * w + filter.a1 * filter.w1 + filter.a2 * filter.w2;

    filter.w2 = filter.w1;
    filter.w1 = w;

    if (out >  2.0f) out =  2.0f;
    if (out < -2.0f) out = -2.0f;

    return out * wah->level;
}
