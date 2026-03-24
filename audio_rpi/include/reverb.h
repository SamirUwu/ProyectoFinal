#ifndef REVERB_H
#define REVERB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float feedback; 
    float lpfreq;    
    float mix;      
} Reverb;

// Inicializa parámetros
void  Reverb_init(Reverb *rv, float feedback, float lpfreq, float mix);

// Procesa una muestra
float Reverb_process(Reverb *rv, float input);

#ifdef __cplusplus
}
#endif

#endif