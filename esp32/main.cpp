#include <Arduino.h>
#include "driver/i2s.h"
#include "driver/adc.h"

#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 44100
#define BUFFER_LEN 256

void i2s_adc_init() {
    // Configurar ADC1 canal 0 (GPIO36)
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);

    // Configuración I2S para ADC interno
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_ADC_BUILT_IN),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false
    };

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

    // Vincular ADC al I2S y habilitar
    i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_0);
    i2s_adc_enable(I2S_PORT);
}

void setup() {
    Serial.begin(115200);
    i2s_adc_init();
    Serial.println("I2S ADC inicializado");
}

void loop() {
    int16_t buffer[BUFFER_LEN];
    size_t bytes_read = 0;

    // Leer muestras del ADC vía I2S
    i2s_read(I2S_PORT, buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);

    // Mostrar algunas muestras por Serial para debug
    for (int i = 0; i < 10; i++) {  // solo las primeras 10 para no saturar el serial
        Serial.println(buffer[i]);
    }

    delay(100);  // pequeño retardo para ver los valores
}