#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include "../include/socket_server.h"
#include <string.h>

#include "../include/delay.h"
#include "../include/overdrive.h"
#include "../include/wah.h"
#include "../include/chorus.h"
#include "../include/flanger.h"
#include "../include/pitch_shifter.h"

#define SAMPLE_RATE 44100
#define PI 3.14159265358979323846f

char json_buffer[4096];

// --- IDs de efectos ---
#define FX_OVERDRIVE     0
#define FX_WAH           1
#define FX_DELAY         2
#define FX_CHORUS        3
#define FX_FLANGER       4
#define FX_PITCHSHIFTER  5
#define FX_COUNT         6

// Se resetea en cada JSON: solo quedan activos los efectos que llegaron en ese mensaje
int enabled[FX_COUNT] = {0};

// --- Tabla de mapeo generico efecto -> parametro -> puntero ---
typedef struct {
    const char *effect_key;  // coincide con "type" en el JSON
    const char *param_key;   // nombre del parametro dentro de "params"
    int         fx_id;       // indice en enabled[] para activar el efecto
    float      *target;      // puntero directo al campo del struct
    float       scale;       // valor_final = raw * scale + offset
    float       offset;
} ParamMap;

int main()
{
    Delay delay;
    Delay_init(&delay, 20.0f, 0.5f, 0.4f);

    Overdrive od;
    Overdrive_init(&od, 0.0f, 0.0f, 0.0f);

    Wah wah;
    Wah_init(&wah, 2.0f, 3.0f, 0.9f);

    Chorus ch;
    Chorus_init(&ch, 0.8f, 0.7f, 0.5f);

    Flanger flanger;
    Flanger_init(&flanger, 0.25f, 0.7f, 0.3f, 0.5f);

    PitchShifter pitch;
    PitchShifter_init(&pitch, 7.0f, 0.5f);

    // Para agregar un efecto nuevo: 1) define su FX_ID arriba, 2) agrega sus filas aqui
    ParamMap map[] = {
        // Overdrive
        { "Overdrive",    "GAIN",      FX_OVERDRIVE,    &od.gain,         10.0f, 1.0f },
        { "Overdrive",    "TONE",      FX_OVERDRIVE,    &od.tone,          1.0f, 0.0f },
        { "Overdrive",    "OUTPUT",    FX_OVERDRIVE,    &od.output,        1.0f, 0.0f },
        // Wah
        { "Wah",          "FREQ",      FX_WAH,          &wah.freq,         1.0f, 0.0f },
        { "Wah",          "Q",         FX_WAH,          &wah.q,            1.0f, 0.0f },
        { "Wah",          "LEVEL",     FX_WAH,          &wah.level,        1.0f, 0.0f },
        // Delay
        { "Delay",        "TIME",      FX_DELAY,        &delay.delay_ms,   1.0f, 0.0f },
        { "Delay",        "FEEDBACK",  FX_DELAY,        &delay.feedback,   1.0f, 0.0f },
        { "Delay",        "MIX",       FX_DELAY,        &delay.mix,        1.0f, 0.0f },
        // Chorus
        { "Chorus",       "RATE",      FX_CHORUS,       &ch.rate,          1.0f, 0.0f },
        { "Chorus",       "DEPTH",     FX_CHORUS,       &ch.depth,         1.0f, 0.0f },
        { "Chorus",       "MIX",       FX_CHORUS,       &ch.mix,           1.0f, 0.0f },
        // Flanger
        { "Flanger",      "RATE",      FX_FLANGER,      &flanger.rate,     1.0f, 0.0f },
        { "Flanger",      "DEPTH",     FX_FLANGER,      &flanger.depth,    1.0f, 0.0f },
        { "Flanger",      "FEEDBACK",  FX_FLANGER,      &flanger.feedback, 1.0f, 0.0f },
        { "Flanger",      "MIX",       FX_FLANGER,      &flanger.mix,      1.0f, 0.0f },
        // PitchShifter
        { "PitchShifter", "SEMITONES", FX_PITCHSHIFTER, &pitch.semitones,  1.0f, 0.0f },
        { "PitchShifter", "MIX",       FX_PITCHSHIFTER, &pitch.mix,        1.0f, 0.0f },
    };

    int map_size = sizeof(map) / sizeof(map[0]);

    socket_init();

    int i = 0;
    while (1) {
        int n = socket_receive(json_buffer, sizeof(json_buffer) - 1);

        if (n > 0) {
            printf("JSON recibido:\n%s\n", json_buffer);

            // Resetear todos los efectos: solo los que lleguen en este JSON quedaran activos
            memset(enabled, 0, sizeof(enabled));

            char *cursor = json_buffer;

            // Iterar sobre cada objeto en effects[]
            while ((cursor = strstr(cursor, "\"type\"")) != NULL) {

                // Extraer valor de "type": "XxxXxx"
                char *type_colon = strchr(cursor, ':');
                if (!type_colon) break;
                char *type_start = strchr(type_colon + 1, '"');
                if (!type_start) break;
                type_start++;
                char *type_end = strchr(type_start, '"');
                if (!type_end) break;

                char effect_type[64] = {0};
                int type_len = type_end - type_start;
                if (type_len >= (int)sizeof(effect_type)) type_len = sizeof(effect_type) - 1;
                strncpy(effect_type, type_start, type_len);

                cursor = type_end + 1;

                // Leer campo "enabled": true/false
                int fx_enabled = 1;  // default activo si no se especifica
                char *enabled_pos = strstr(cursor, "\"enabled\"");
                if (enabled_pos) {
                    char *en_colon = strchr(enabled_pos, ':');
                    if (en_colon) {
                        char *en_val = en_colon + 1;
                        while (*en_val == ' ') en_val++;
                        fx_enabled = (strncmp(en_val, "true", 4) == 0) ? 1 : 0;
                    }
                }

                // Buscar bloque "params" { ... }
                char *params_pos   = strstr(cursor, "\"params\"");
                if (!params_pos) continue;
                char *params_open  = strchr(params_pos, '{');
                if (!params_open) continue;
                char *params_close = strchr(params_open, '}');
                if (!params_close) continue;

                char params_block[512] = {0};
                int block_len = params_close - params_open + 1;
                if (block_len >= (int)sizeof(params_block)) block_len = sizeof(params_block) - 1;
                strncpy(params_block, params_open, block_len);

                printf("  [%s] enabled=%d\n", effect_type, fx_enabled);

                // Aplicar parametros y marcar efecto como activo/inactivo
                for (int m = 0; m < map_size; m++) {
                    if (strcmp(map[m].effect_key, effect_type) != 0) continue;

                    // Marcar enabled segun el campo del JSON
                    enabled[map[m].fx_id] = fx_enabled;

                    // Parsear parametros solo si el efecto esta activo
                    if (!fx_enabled) continue;

                    char *param_pos = strstr(params_block, map[m].param_key);
                    if (!param_pos) continue;
                    char *colon = strchr(param_pos, ':');
                    if (!colon) continue;

                    float raw_value = 0.0f;
                    if (sscanf(colon + 1, "%f", &raw_value) == 1) {
                        *map[m].target = raw_value * map[m].scale + map[m].offset;
                        printf("    %s = %f\n", map[m].param_key, *map[m].target);
                    }
                }
            }

            memset(json_buffer, 0, sizeof(json_buffer));
        }

        // --- Cadena de efectos dinamica ---
        // La senal pasa por cada efecto solo si esta enabled, en orden fijo
        float input = sinf(2.0f * PI * 440.0f * i / SAMPLE_RATE);
        i++;
        if (i >= SAMPLE_RATE) i = 0;

        float sig = input;

        if (enabled[FX_OVERDRIVE])    sig = Overdrive_process(&od, sig);
        if (enabled[FX_WAH])          sig = Wah_process(&wah, sig);
        if (enabled[FX_CHORUS])       sig = Chorus_process(&ch, sig);
        if (enabled[FX_FLANGER])      sig = Flanger_process(&flanger, sig);
        if (enabled[FX_PITCHSHIFTER]) sig = PitchShifter_process(&pitch, sig);
        if (enabled[FX_DELAY])        sig = Delay_process(&delay, sig);

        if (i % 3000 == 0) {
            printf("audio: input=%f out=%f  [OD:%d WAH:%d CH:%d FL:%d PT:%d DL:%d]\n",
                   input, sig,
                   enabled[FX_OVERDRIVE], enabled[FX_WAH], enabled[FX_CHORUS],
                   enabled[FX_FLANGER], enabled[FX_PITCHSHIFTER], enabled[FX_DELAY]);
        }

        socket_send_two_floats(input, sig);
        usleep(22);
    }

    socket_close();
    return 0;
}