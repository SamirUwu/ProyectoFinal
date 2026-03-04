#include "../include/delay.h"

void Delay_init(Delay *d, float delay_ms, float feedback, float mix)
{
    d->writeIndex = 0;
    d->delaySamples = (int)((delay_ms / 1000.0f) * SAMPLE_RATE);
    d->feedback = feedback;
    d->mix = mix;

    for (int i = 0; i < SAMPLE_RATE * MAX_DELAY_MS / 1000; i++)
        d->buffer[i] = 0.0f;
}

float Delay_process(Delay *d, float input)
{
    int readIndex = d->writeIndex - d->delaySamples;

    if (readIndex < 0)
        readIndex += SAMPLE_RATE * MAX_DELAY_MS / 1000;

    float delayedSample = d->buffer[readIndex];

    d->buffer[d->writeIndex] = input + delayedSample * d->feedback;

    d->writeIndex++;
    if (d->writeIndex >= SAMPLE_RATE * MAX_DELAY_MS / 1000)
        d->writeIndex = 0;

    return input * (1.0f - d->mix) + delayedSample * d->mix;
}