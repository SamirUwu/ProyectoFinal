#include "reverbsc.h"
#include <math.h>
#include "reverb.h"
#include <stdint.h>

#define DEFAULT_SRATE 48000.0
#define DELAYPOS_SHIFT 28
#define DELAYPOS_SCALE 0x10000000
#define DELAYPOS_MASK 0x0FFFFFFF

static const float kReverbParams[8][4] = {
    {2473.0/DEFAULT_SRATE, 0.0010, 3.1, 1966},
    {2767.0/DEFAULT_SRATE, 0.0011, 3.5, 29491},
    {3217.0/DEFAULT_SRATE, 0.0017, 1.11, 22937},
    {3557.0/DEFAULT_SRATE, 0.0006, 3.973, 9830},
    {3907.0/DEFAULT_SRATE, 0.0010, 2.341, 20643},
    {4127.0/DEFAULT_SRATE, 0.0011, 1.897, 22937},
    {2143.0/DEFAULT_SRATE, 0.0017, 0.891, 29491},
    {1933.0/DEFAULT_SRATE, 0.0006, 3.221, 14417}
};

static const float kOutputGain = 0.35;
static const float kJpScale = 0.25;

static int DelayLineMaxSamples(float sr, int n)
{
    float max_del = kReverbParams[n][0] + kReverbParams[n][1] * 1.125f;
    return (int)(max_del * sr + 16.5f);
}

int MyReverb::Init(float sr)
{
    sample_rate_ = sr;
    feedback_ = 0.97f;
    lpfreq_ = 10000.0f;
    prv_lpfreq_ = 0.0f;
    init_done_ = 1;

    int offset = 0;
    for(int i = 0; i < 8; i++)
    {
        delay_lines_[i].buf = &aux_[offset];
        delay_lines_[i].buffer_size = DelayLineMaxSamples(sr, i);
        delay_lines_[i].write_pos = 0;
        delay_lines_[i].read_pos = 0;
        delay_lines_[i].read_pos_frac = 0;
        delay_lines_[i].filter_state = 0.0f;

        for(int j = 0; j < delay_lines_[i].buffer_size; j++)
            delay_lines_[i].buf[j] = 0.0f;

        offset += delay_lines_[i].buffer_size;
    }
    return 0;
}

int MyReverb::Process(const float &in1, const float &in2, float *out1, float *out2)
{
    float outL = 0.0f, outR = 0.0f;

    for(int i = 0; i < 8; i++)
    {
        ReverbScDl *dl = &delay_lines_[i];

        float delayed = dl->buf[dl->read_pos];

        dl->filter_state = delayed * feedback_;
        dl->buf[dl->write_pos] = (i % 2 ? in2 : in1) + dl->filter_state;

        dl->write_pos = (dl->write_pos + 1) % dl->buffer_size;
        dl->read_pos  = (dl->read_pos + 1) % dl->buffer_size;

        if(i % 2)
            outR += delayed;
        else
            outL += delayed;
    }

    *out1 = outL * kOutputGain;
    *out2 = outR * kOutputGain;
    return 0;
}


static MyReverb g_reverb;

extern "C" void Reverb_init(Reverb *rv, float feedback, float lpfreq, float mix)
{
    rv->feedback = feedback;
    rv->lpfreq   = lpfreq;
    rv->mix      = mix;

    g_reverb.Init(44100.0f);
    g_reverb.SetFeedback(feedback);
    g_reverb.SetLpFreq(lpfreq);
}

extern "C" float Reverb_process(Reverb *rv, float input)
{
    g_reverb.SetFeedback(rv->feedback);
    g_reverb.SetLpFreq(rv->lpfreq);

    float outL, outR;
    g_reverb.Process(input, input, &outL, &outR);

    float wet = (outL + outR) * 0.5f;
    return input * (1.0f - rv->mix) + wet * rv->mix;
}
