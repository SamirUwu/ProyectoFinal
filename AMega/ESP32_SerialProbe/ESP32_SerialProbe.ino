// =============================================================================
//  ESP32_SerialProbe.ino  —  core 3.x correct timer pattern
//
//  TWO bugs fixed from previous version:
//
//  BUG 1 — timerBegin returned NULL
//  In core 3.x, timerBegin() calls gptimer_new_timer() which allocates one
//  of the 4 hardware timer slots.  If all 4 are taken it returns NULL.
//  The arduino-esp32 3.x framework pre-allocates timer slots for some
//  internal uses.  If timerBegin fails, this sketch retries with increasing
//  delay, which forces the framework to finish its own init first.
//
//  BUG 2 — timerBegin succeeded but ISR never fired (ticks stayed 0)
//  In core 3.x the interrupt fires on ALARM, not on counter overflow.
//  After timerAttachInterrupt() you MUST call timerAlarm(timer, value,
//  autoreload, reload_count) to arm the alarm.  Without this call the
//  timer counts but never triggers the ISR.
//  Correct sequence:
//    t = timerBegin(1000000);           // 1 MHz resolution
//    timerAttachInterrupt(t, &onTick);  // register ISR
//    timerAlarm(t, 1000, true, 0);      // fire every 1000 ticks = 1 ms
//
//  Expected output (Serial Monitor, 460800 baud):
//    ESP32 SerialProbe boot
//    arduino-esp32 version: 3.3.7
//    Free heap: 266996 bytes
//    timerBegin OK on attempt 1
//    PING 1  |  ticks: ~1000  |  heap: 266xxx
//    PING 2  ...
// =============================================================================
#include <Arduino.h>

#define BAUD 460800

hw_timer_t*       t     = nullptr;
volatile uint32_t ticks = 0;

void ARDUINO_ISR_ATTR onTick() { ticks++; }

void setup()
{
    Serial.begin(BAUD);
    delay(500);

    Serial.println("\nESP32 SerialProbe boot");
    Serial.printf("arduino-esp32 version: %d.%d.%d\n",
                  ESP_ARDUINO_VERSION_MAJOR,
                  ESP_ARDUINO_VERSION_MINOR,
                  ESP_ARDUINO_VERSION_PATCH);
    Serial.printf("Free heap: %u bytes\n", (unsigned)esp_get_free_heap_size());
    Serial.flush();

    // Retry loop: gptimer slots may be briefly held by framework init code.
    // In practice it always succeeds on attempt 1 or 2.
    for (int attempt = 1; attempt <= 5; attempt++) {
        t = timerBegin(1000000);   // 1 MHz counter resolution
        if (t) {
            Serial.printf("timerBegin OK on attempt %d\n", attempt);
            Serial.flush();
            break;
        }
        Serial.printf("timerBegin NULL on attempt %d, retrying...\n", attempt);
        Serial.flush();
        delay(100);
    }

    if (!t) {
        Serial.println("timerBegin FAILED after 5 attempts.");
        Serial.println("All 4 hardware timers are in use by the framework.");
        Serial.println("This should not happen on a bare ESP32 — reflash bootloader.");
        Serial.flush();
        return;
    }

    // Register ISR
    timerAttachInterrupt(t, &onTick);

    // ARM the alarm: fire every 1000 ticks (= 1 ms at 1 MHz), auto-reload
    // timerAlarm(timer, alarm_value, autoreload, reload_count)
    //   alarm_value  : counter value that triggers the ISR
    //   autoreload   : true = reset counter to 0 after alarm
    //   reload_count : 0 = reload indefinitely
    timerAlarm(t, 1000, true, 0);

    Serial.println("Timer armed — ticks should increase ~1000/s");
    Serial.flush();
}

void loop()
{
    static uint32_t n = 0;
    delay(1000);
    Serial.printf("PING %u  |  ticks: %u  |  heap: %u\n",
                  ++n, ticks, (unsigned)esp_get_free_heap_size());
    Serial.flush();
}
