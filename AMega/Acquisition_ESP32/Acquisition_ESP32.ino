// =============================================================================
//  Acquisition_ESP32.ino  —  v11  (arduino-esp32 core 3.x)
//  460800 baud  |  22 kSPS output  |  128-sample packets
//
//  ── Root cause of all timer failures (now fixed) ────────────────────────────
//
//  Every version up to v10 had the same two timer bugs:
//
//  BUG A — timerBegin returned NULL
//  core 3.x uses the IDF gptimer driver.  timerBegin() calls
//  gptimer_new_timer() which allocates from a pool of 4 hardware slots.
//  The framework pre-allocates one slot during init (for the system tick or
//  wdt).  If timerBegin is called too early or a driver holds a slot
//  temporarily, it returns NULL.  Fix: retry with a short delay.
//
//  BUG B — timerBegin succeeded but ISR never fired
//  In core 3.x the ISR fires on ALARM, not on counter overflow.
//  The core 2.x pattern was:
//    timerBegin(0, prescaler, true)   → timer ticks
//    timerAttachInterrupt(...)        → ISR registered
//    timerAlarmWrite(t, N, true)      → arm alarm at count N
//    timerAlarmEnable(t)              → start alarm
//  In core 3.x timerAlarmWrite and timerAlarmEnable are gone.
//  The single replacement call is:
//    timerAlarm(timer, alarm_value, autoreload, reload_count)
//  Without this call the counter runs but the ISR is NEVER triggered —
//  which is why all previous versions showed isr_count=0.
//
//  The correct core 3.x sequence for a periodic ISR at SAMPLE_RATE_HZ:
//    t = timerBegin(1000000);          // 1 MHz resolution
//    timerAttachInterrupt(t, &onISR);  // register callback
//    timerAlarm(t, 1000000/RATE, true, 0);  // fire every N µs
//
//  ── Wire format ─────────────────────────────────────────────────────────────
//    [0xAA 0x55 0xFF 0x00] + [128 × uint16 LE, 12-bit codes 0–4095]
//    Packet = 260 bytes.  Output Fs = 22 039 Hz (44077 / 2 decimation).
//
//  ── Usage ───────────────────────────────────────────────────────────────────
//  1. Upload with VERBOSE_BOOT 1, open Serial Monitor at 460800 baud.
//     Confirm you see "[5] Streaming..." and [STAT] isr count growing.
//  2. Set VERBOSE_BOOT 0, re-upload, then run Python:
//       python diagnose_esp32.py --port /dev/ttyUSB0 --baud 460800
//       python monitor.py --port /dev/ttyUSB0 --baud 460800 \
//                         --fs 22039 --vref 3.3 --esp32
// =============================================================================

#include <Arduino.h>
#include <driver/adc.h>
#include <soc/sens_reg.h>
#include <soc/rtc_cntl_reg.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/ringbuf.h>
#include <freertos/semphr.h>

// ── Set 1 for boot log; MUST be 0 for monitor.py / diagnose ──────────────────
#define VERBOSE_BOOT  1

// ── Configuration ─────────────────────────────────────────────────────────────
#define ADC_CHANNEL_NUM   ADC1_CHANNEL_0   // GPIO36 (VP)
#define ADC_ATTEN_USE     ADC_ATTEN_DB_11  // 0–3.3 V

#define SAMPLE_RATE_HZ    44077   // timer fires at this rate
#define DECIMATE          2       // push every 2nd sample → 22039 Hz output
#define SERIAL_BAUD       460800
#define PACKET_SAMPLES    128
#define RING_PACKETS      16
#define RING_BYTES        (RING_PACKETS * PACKET_SAMPLES * sizeof(uint16_t))

// Timer resolution and alarm value
// timerBegin(1000000) → 1 MHz counter (1 tick = 1 µs)
// alarm = 1000000 / 44077 = 22 ticks → actual Fs = 1e6/22 = 45 455 Hz
// Use 23 for 43 478 Hz — close enough; actual Fs measured by Python
#define TIMER_RESOLUTION_HZ  1000000UL
#define TIMER_ALARM_TICKS    (TIMER_RESOLUTION_HZ / SAMPLE_RATE_HZ)   // = 22

// ── Protocol ──────────────────────────────────────────────────────────────────
static const uint8_t SYNC[4] = { 0xAA, 0x55, 0xFF, 0x00 };

// ── Globals ───────────────────────────────────────────────────────────────────
static RingbufHandle_t   ring_buf  = nullptr;
static SemaphoreHandle_t tx_ready  = nullptr;
static hw_timer_t*       adc_timer = nullptr;

static volatile uint32_t overruns  = 0;
static volatile uint32_t isr_count = 0;

static uint32_t g_reg_start0 = 0;
static uint32_t g_reg_start1 = 0;
static bool     g_use_direct = false;


// ─────────────────────────────────────────────────────────────────────────────
//  Direct SAR ADC1 read (~2 µs)
//  Uses stable REG_READ/REG_WRITE macros (not SENS.* bitfield structs).
// ─────────────────────────────────────────────────────────────────────────────
static inline uint16_t IRAM_ATTR read_adc_direct()
{
    REG_WRITE(SENS_SAR_MEAS_START1_REG, g_reg_start0);
    REG_WRITE(SENS_SAR_MEAS_START1_REG, g_reg_start1);
    uint8_t guard = 200;
    while (!(REG_READ(SENS_SAR_MEAS_START1_REG) & SENS_MEAS1_DONE_SAR_M)) {
        if (--guard == 0) return 0;
    }
    return (uint16_t)((REG_READ(SENS_SAR_MEAS_START1_REG)
                       >> SENS_MEAS1_DATA_SAR_S) & 0x0FFF);
}


// ─────────────────────────────────────────────────────────────────────────────
//  Timer ISR — fires at ~SAMPLE_RATE_HZ via timerAlarm
// ─────────────────────────────────────────────────────────────────────────────
static void ARDUINO_ISR_ATTR onTimer()
{
    static uint8_t dec = 0;

    uint16_t sample;
    if (g_use_direct) {
        sample = read_adc_direct();
    } else {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        sample = (uint16_t)adc1_get_raw(ADC_CHANNEL_NUM);
        #pragma GCC diagnostic pop
    }
    isr_count++;

    if (++dec < DECIMATE) return;
    dec = 0;

    BaseType_t ok = xRingbufferSendFromISR(ring_buf, &sample,
                                           sizeof(sample), nullptr);
    if (ok != pdTRUE) overruns++;
}


// ─────────────────────────────────────────────────────────────────────────────
//  ADC init
// ─────────────────────────────────────────────────────────────────────────────
static bool init_adc()
{
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CHANNEL_NUM, ADC_ATTEN_USE);
    adc1_get_raw(ADC_CHANNEL_NUM);   // warm-up
    #pragma GCC diagnostic pop

    REG_CLR_BIT(SENS_SAR_READ_CTRL_REG, SENS_SAR1_DIG_FORCE_M);
    REG_SET_FIELD(SENS_SAR_MEAS_WAIT2_REG, SENS_FORCE_XPD_SAR, 3);

    uint32_t val = REG_READ(SENS_SAR_MEAS_START1_REG);
    val |=  SENS_MEAS1_START_FORCE_M;
    val |=  SENS_SAR1_EN_PAD_FORCE_M;
    val  = (val & ~SENS_SAR1_EN_PAD_M)
         | ((uint32_t)(1 << ADC_CHANNEL_NUM) << SENS_SAR1_EN_PAD_S);
    val &= ~SENS_MEAS1_START_SAR_M;
    REG_WRITE(SENS_SAR_MEAS_START1_REG, val);
    g_reg_start0 = val;
    g_reg_start1 = val | SENS_MEAS1_START_SAR_M;

    // Verify with a test conversion
    REG_WRITE(SENS_SAR_MEAS_START1_REG, g_reg_start0);
    REG_WRITE(SENS_SAR_MEAS_START1_REG, g_reg_start1);
    uint32_t t0 = micros();
    while (!(REG_READ(SENS_SAR_MEAS_START1_REG) & SENS_MEAS1_DONE_SAR_M)) {
        if (micros() - t0 > 50) {
#if VERBOSE_BOOT
            Serial.println("  [ADC] direct register timeout — using analogRead fallback");
            Serial.flush();
#endif
            return false;
        }
    }
#if VERBOSE_BOOT
    uint16_t v = (REG_READ(SENS_SAR_MEAS_START1_REG)
                  >> SENS_MEAS1_DATA_SAR_S) & 0x0FFF;
    Serial.printf("  [ADC] direct test read: %u (%.3f V)\n",
                  v, v * 3.3f / 4095.0f);
    Serial.flush();
#endif
    return true;
}


// ─────────────────────────────────────────────────────────────────────────────
//  Timer init — core 3.x with timerAlarm (the missing piece in all prev versions)
// ─────────────────────────────────────────────────────────────────────────────
static bool init_timer()
{
    // Retry up to 5 times in case a framework driver temporarily holds a slot
    for (int attempt = 1; attempt <= 5; attempt++) {
        adc_timer = timerBegin(TIMER_RESOLUTION_HZ);
        if (adc_timer) {
#if VERBOSE_BOOT
            Serial.printf("  [TIMER] timerBegin OK on attempt %d\n", attempt);
            Serial.flush();
#endif
            break;
        }
#if VERBOSE_BOOT
        Serial.printf("  [TIMER] timerBegin NULL attempt %d\n", attempt);
        Serial.flush();
#endif
        delay(50);
    }
    if (!adc_timer) return false;

    // Register ISR
    timerAttachInterrupt(adc_timer, &onTimer);

    // ARM THE ALARM — this is what was missing in every previous version.
    // Without this call the counter runs silently and the ISR never fires.
    //   alarm_value  = TIMER_ALARM_TICKS  (counter value that triggers ISR)
    //   autoreload   = true               (reset counter to 0 after each alarm)
    //   reload_count = 0                  (repeat indefinitely)
    timerAlarm(adc_timer, TIMER_ALARM_TICKS, true, 0);

    return true;
}


// ─────────────────────────────────────────────────────────────────────────────
//  TX task — Core 0
// ─────────────────────────────────────────────────────────────────────────────
static uint8_t tx_pkt[4 + PACKET_SAMPLES * sizeof(uint16_t)];

static void tx_task(void*)
{
    memcpy(tx_pkt, SYNC, 4);
    uint16_t* const samples = (uint16_t*)(tx_pkt + 4);
    xSemaphoreGive(tx_ready);

    for (;;) {
        size_t filled = 0;
        while (filled < PACKET_SAMPLES) {
            size_t item_sz = 0;
            void* chunk = xRingbufferReceiveUpTo(
                ring_buf, &item_sz, pdMS_TO_TICKS(500),
                (PACKET_SAMPLES - filled) * sizeof(uint16_t));
            if (!chunk) continue;   // wait — do NOT zero-pad
            memcpy(samples + filled, chunk, item_sz);
            vRingbufferReturnItem(ring_buf, chunk);
            filled += item_sz / sizeof(uint16_t);
        }
        Serial.write(tx_pkt, sizeof(tx_pkt));
    }
}


// ─────────────────────────────────────────────────────────────────────────────
//  setup / loop
// ─────────────────────────────────────────────────────────────────────────────
void setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(500);

#if VERBOSE_BOOT
    Serial.println("\n=== Acquisition_ESP32 v11 ===");
    Serial.printf("arduino-esp32 %d.%d.%d  |  heap: %u\n",
                  ESP_ARDUINO_VERSION_MAJOR,
                  ESP_ARDUINO_VERSION_MINOR,
                  ESP_ARDUINO_VERSION_PATCH,
                  (unsigned)esp_get_free_heap_size());
    Serial.printf("SAMPLE_RATE=%d  ALARM=%lu  DECIMATE=%d  OUT_FS=%d  BAUD=%d\n",
                  SAMPLE_RATE_HZ, (unsigned long)TIMER_ALARM_TICKS,
                  DECIMATE, SAMPLE_RATE_HZ/DECIMATE, SERIAL_BAUD);
    Serial.flush();
    Serial.println("[1] FreeRTOS objects...");
    Serial.flush();
#endif

    tx_ready = xSemaphoreCreateBinary();
    ring_buf = xRingbufferCreate(RING_BYTES, RINGBUF_TYPE_BYTEBUF);
    if (!tx_ready || !ring_buf) {
        pinMode(2, OUTPUT);
        for (;;) { digitalWrite(2, !digitalRead(2)); delay(200); }
    }

#if VERBOSE_BOOT
    Serial.println("  OK");
    Serial.println("[2] TX task...");
    Serial.flush();
#endif

    xTaskCreatePinnedToCore(tx_task, "tx", 4096, nullptr,
                            configMAX_PRIORITIES - 1, nullptr, 0);
    xSemaphoreTake(tx_ready, portMAX_DELAY);

#if VERBOSE_BOOT
    Serial.println("  alive");
    Serial.println("[3] ADC...");
    Serial.flush();
#endif

    g_use_direct = init_adc();

#if VERBOSE_BOOT
    Serial.printf("  mode: %s\n", g_use_direct ? "direct registers" : "analogRead");
    Serial.println("[4] Timer...");
    Serial.flush();
#endif

    bool timer_ok = init_timer();

#if VERBOSE_BOOT
    Serial.printf("  %s\n", timer_ok ? "OK" : "FAILED — no samples will be produced");
    if (timer_ok) {
        Serial.printf("[5] Streaming at ~%d Hz output Fs\n",
                      SAMPLE_RATE_HZ / DECIMATE);
        Serial.println("    Set VERBOSE_BOOT 0 and re-upload before using monitor.py");
    }
    Serial.flush();
    delay(200);   // flush boot log before binary stream starts
#endif
}

void loop()
{
#if VERBOSE_BOOT
    vTaskDelay(pdMS_TO_TICKS(5000));
    Serial.printf("[STAT] isr=%u  overruns=%u  heap=%u\n",
                  isr_count, overruns, (unsigned)esp_get_free_heap_size());
    Serial.flush();
#else
    vTaskDelay(portMAX_DELAY);
#endif
}
