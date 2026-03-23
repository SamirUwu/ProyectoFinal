#ifndef CHORUS_H
#define CHORUS_H

#define CHORUS_MAX_DELAY_MS 50
#define SAMPLE_RATE 44100

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float rate;
    float depth;
    float feedback;
    float mix;
} Chorus;

void  Chorus_init(Chorus *ch, float rate, float depth, float feedback, float mix);
float Chorus_process(Chorus *ch, float input);

#ifdef __cplusplus
}
#endif

#endif