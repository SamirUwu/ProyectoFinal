#include "reverb.h"
#include <stdlib.h>

#define MAX_DELAY 44100  // 1 segundo máx

static float delay_buffer[MAX_DELAY];
static int write_idx = 0;

extern "C" void Reverb_init(Reverb *rv, float feedback, float lpfreq, float mix)
{
    rv->feedback = feedback;
    rv->lpfreq   = lpfreq;
    rv->mix      = mix;

    for(int i = 0; i < MAX_DELAY; i++)
        delay_buffer[i] = 0.0f;

    write_idx = 0;
}

extern "C" float Reverb_process(Reverb *rv, float input)
{
    // delays distintos (en samples)
    int d1 = 1116;
    int d2 = 1188;
    int d3 = 1277;
    int d4 = 1356;

    float y = 0.0f;

    int idx1 = (write_idx - d1 + MAX_DELAY) % MAX_DELAY;
    int idx2 = (write_idx - d2 + MAX_DELAY) % MAX_DELAY;
    int idx3 = (write_idx - d3 + MAX_DELAY) % MAX_DELAY;
    int idx4 = (write_idx - d4 + MAX_DELAY) % MAX_DELAY;

    y = (delay_buffer[idx1] +
         delay_buffer[idx2] +
         delay_buffer[idx3] +
         delay_buffer[idx4]) * 0.25f;

    // simple low-pass (muy importante para que no suene feo)
    static float lp = 0.0f;
    float alpha = rv->lpfreq / (rv->lpfreq + 44100.0f);
    lp = alpha * y + (1.0f - alpha) * lp;

    // feedback
    delay_buffer[write_idx] = input + lp * rv->feedback;

    write_idx = (write_idx + 1) % MAX_DELAY;

    return input * (1.0f - rv->mix) + lp * rv->mix;
}