/*
Copyright (c) 2020 Electrosmith, Corp
MIT License
Adapted for standalone Linux/RPi use — no Daisy hardware required.
*/
#pragma once
#ifndef DSY_CHORUS_DSP_H
#define DSY_CHORUS_DSP_H

#include "dsp.h"
#include "delayline.h"

namespace daisysp
{
class ChorusEngine
{
  public:
    ChorusEngine() {}
    ~ChorusEngine() {}

    void Init(float sample_rate);
    float Process(float in);
    void SetLfoDepth(float depth);
    void SetLfoFreq(float freq);
    void SetDelay(float delay);
    void SetDelayMs(float ms);
    void SetFeedback(float feedback);

  private:
    float sample_rate_;
    static constexpr int32_t kDelayLength = 2205; // 50ms @ 44100

    float lfo_phase_;
    float lfo_freq_;
    float lfo_amp_;
    float feedback_;
    float delay_;

    DelayLine<float, kDelayLength> del_;

    float ProcessLfo();
};

class Chorus
{
  public:
    Chorus() {}
    ~Chorus() {}

    void  Init(float sample_rate);
    float Process(float in);
    float GetLeft();
    float GetRight();

    void SetLfoDepth(float depth);
    void SetLfoFreq(float freq);
    void SetDelay(float delay);
    void SetDelayMs(float ms);
    void SetFeedback(float feedback);

  private:
    ChorusEngine engines_[2];
    float        gain_frac_;
    float        sigl_, sigr_;
};

} // namespace daisysp
#endif
