/*
 * chorus.cpp
 * DaisySP Chorus portado para Linux/RPi — sin dependencias de hardware.
 * Wrapper extern "C" para que main.c lo use sin cambios.
 */

#include "chorus_dsp.h"
#include "chorus.h"
#include <cmath>

using namespace daisysp;

// ── ChorusEngine ─────────────────────────────────────────────────────────────

void ChorusEngine::Init(float sample_rate)
{
    sample_rate_ = sample_rate;
    del_.Init();
    lfo_amp_  = 0.f;
    feedback_ = 0.2f;
    SetDelay(0.75f);
    lfo_phase_ = 0.f;
    SetLfoFreq(0.3f);
    SetLfoDepth(0.9f);
}

float ChorusEngine::Process(float in)
{
    float lfo_sig = ProcessLfo();
    del_.SetDelay(lfo_sig + delay_);
    float out = del_.Read();
    del_.Write(in + out * feedback_);
    return (in + out) * .5f;
}

void ChorusEngine::SetLfoDepth(float depth)
{
    depth    = fclamp(depth, 0.f, .93f);
    lfo_amp_ = depth * delay_;
}

void ChorusEngine::SetLfoFreq(float freq)
{
    freq = 4.f * freq / sample_rate_;
    freq *= lfo_freq_ < 0.f ? -1.f : 1.f;
    lfo_freq_ = fclamp(freq, -.25f, .25f);
}

void ChorusEngine::SetDelay(float delay)
{
    delay = (.1f + delay * 7.9f); // 0.1 to 8 ms
    SetDelayMs(delay);
}

void ChorusEngine::SetDelayMs(float ms)
{
    ms     = fmax(0.1f, ms);
    delay_ = ms * .001f * sample_rate_;
    lfo_amp_ = fmin(lfo_amp_, delay_);
}

void ChorusEngine::SetFeedback(float feedback)
{
    feedback_ = fclamp(feedback, 0.f, 1.f);
}

float ChorusEngine::ProcessLfo()
{
    lfo_phase_ += lfo_freq_;
    if(lfo_phase_ > 1.f)
    {
        lfo_phase_ = 1.f - (lfo_phase_ - 1.f);
        lfo_freq_ *= -1.f;
    }
    else if(lfo_phase_ < -1.f)
    {
        lfo_phase_ = -1.f - (lfo_phase_ + 1.f);
        lfo_freq_ *= -1.f;
    }
    return lfo_phase_ * lfo_amp_;
}

// ── Chorus ───────────────────────────────────────────────────────────────────

void Chorus::Init(float sample_rate)
{
    engines_[0].Init(sample_rate);
    engines_[1].Init(sample_rate);
    // Offset de fase entre los dos engines para ancho estéreo
    engines_[1].SetLfoFreq(0.3f);
    engines_[1].SetDelay(0.5f);  // delay diferente para el canal derecho
    gain_frac_ = .5f;
    sigl_ = sigr_ = 0.f;
}

float Chorus::Process(float in)
{
    sigl_ = engines_[0].Process(in);
    sigr_ = engines_[1].Process(in);
    sigl_ *= gain_frac_;
    sigr_ *= gain_frac_;
    return (sigl_ + sigr_);  // mono mix
}

float Chorus::GetLeft()  { return sigl_; }
float Chorus::GetRight() { return sigr_; }

void Chorus::SetLfoDepth(float depth)
{
    engines_[0].SetLfoDepth(depth);
    engines_[1].SetLfoDepth(depth);
}

void Chorus::SetLfoFreq(float freq)
{
    engines_[0].SetLfoFreq(freq);
    engines_[1].SetLfoFreq(freq * 1.02f); // leve detune para más richness
}

void Chorus::SetDelay(float delay)
{
    engines_[0].SetDelay(delay);
    engines_[1].SetDelay(delay);
}

void Chorus::SetDelayMs(float ms)
{
    engines_[0].SetDelayMs(ms);
    engines_[1].SetDelayMs(ms);
}

void Chorus::SetFeedback(float feedback)
{
    engines_[0].SetFeedback(feedback);
    engines_[1].SetFeedback(feedback);
}

// ── Wrapper extern "C" para main.c ───────────────────────────────────────────

static Chorus g_chorus;

extern "C" void Chorus_init(::Chorus *ch, float rate, float depth,
                             float feedback, float mix)
{
    ch->rate     = rate;
    ch->depth    = depth;
    ch->feedback = feedback;
    ch->mix      = mix;

    g_chorus.Init(44100.f);

    // LFO freq cuadrático: más musical en el rango bajo
    float freq = 0.5f + rate * rate * 9.5f;  // 0.5 to 10 Hz
    g_chorus.SetLfoFreq(freq);
    g_chorus.SetLfoDepth(depth);
    g_chorus.SetDelay(0.5f);
    g_chorus.SetFeedback(feedback * 0.7f);  // limitar feedback máx
}

extern "C" float Chorus_process(::Chorus *ch, float input)
{
    float freq = 0.5f + ch->rate * ch->rate * 9.5f;
    g_chorus.SetLfoFreq(freq);
    g_chorus.SetLfoDepth(ch->depth);
    g_chorus.SetFeedback(ch->feedback * 0.7f);

    float wet = g_chorus.Process(input);
    return input * (1.f - ch->mix) + wet * ch->mix;
}