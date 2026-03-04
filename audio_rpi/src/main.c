#include <stdio.h>
#include <math.h>
#include "../include/delay.h"
#include "../include/socket_server.h"

#define PI 3.14159265359f

int main()
{
    Delay delay;
    Delay_init(&delay, 50.0f, 0.9f, 0.8f);

    socket_init();

    for (int i = 0; i < SAMPLE_RATE; i++)
    {
        float input = sinf(2.0f * PI * 440.0f * i / SAMPLE_RATE);
        float output = Delay_process(&delay, input);

        socket_send_float(output);
    }

    socket_close();
    
    return 0;
}