// serial_loopback_test.ino
// ─────────────────────────────────────────────────────────────
// STEP 1 diagnostic sketch — upload this INSTEAD of the ADC sampler.
// It sends the exact same binary packet format (sync word + uint16 samples)
// but fills each buffer with a known ramp (0,1,2,…,511) so Python can
// confirm the serial link works before debugging the ADC trigger.
//
// Expected output when received correctly:
//   ✓ Sync word found
//   ✓ Sample values: 0, 1, 2, 3, … 511  (ramp)
// ─────────────────────────────────────────────────────────────

#define BUFFER_SIZE 512

static const uint8_t SYNC[4] = { 0xAA, 0x55, 0xFF, 0x00 };

void setup() {
    Serial.begin(2000000);
    delay(500);
}

void loop() {
    // Send sync word
    Serial.write(SYNC, 4);

    // Send ramp as little-endian uint16
    for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
        uint8_t lo = i & 0xFF;
        uint8_t hi = (i >> 8) & 0xFF;
        Serial.write(lo);
        Serial.write(hi);
    }

    // ~11.6 ms per packet matches real ADC rate; adjust if needed
    delay(12);
}
