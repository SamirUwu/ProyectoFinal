// =============================================================================
//  Acquisition.ino  —  v2
//  Arduino Mega 2560 — ADC sampler @ ~44.1 kSamples/s
//
//  FIX vs v1:
//  ──────────
//  v1 used Timer1 Compare Match B as the ADC trigger (ADTS = 101).
//  The OCF1B flag must be *cleared by software* before the next trigger edge,
//  otherwise the ADC auto-trigger never re-fires after the first conversion.
//  On the ATmega2560 the ADC auto-trigger hardware only latches a *rising edge*
//  of the selected flag; if the flag stays set, no new conversion starts.
//
//  v2 uses Timer1 OVERFLOW as the ADC trigger (ADTS = 110).
//  Timer1 overflow flag (TOV1) is cleared automatically by hardware when the
//  ADC conversion completes — no manual flag clearing needed, rock-solid timing.
//
//  Timer1 configuration for 44.1 kHz overflow rate:
//  ─────────────────────────────────────────────────
//  Normal mode (WGM = 0000), prescaler = 1.
//  Timer counts up from TCNT1_START to 0xFFFF then overflows.
//  Period = (0x10000 - TCNT1_START) / F_CPU
//  TCNT1_START = 0x10000 - round(16e6 / 44100) = 65536 - 363 = 65173
//  Actual Fs   = 16e6 / (65536 - 65173) = 16e6 / 363 ≈ 44 077 Hz
//
//  The ISR reloads TCNT1 on every overflow to maintain the exact period.
//
//  ADC clock = 16 MHz / 32 = 500 kHz → 13 cycles → 38.5 µs/conversion
//  One conversion = 26 µs; one period = 22.6 µs → ADC finishes well within
//  the next period (the ADC start is triggered at overflow; it completes
//  ~13 ADC clocks = 26 µs later, before the next overflow at 22.6 µs …
//
//  WAIT — at 44 kHz the period is 1/44077 = 22.7 µs, but a conversion at
//  500 kHz ADC clock takes 13 cycles = 26 µs.  That means a prescaler of
//  /32 is TOO SLOW for 44 kHz triggering!
//
//  Correct prescaler choice:
//    Need conversion time < 22.7 µs
//    ADC clock must be > 13 / 22.7e-6 = 572 kHz
//    Use prescaler /16 → ADC clock = 1 MHz → 13 µs/conversion ✓
//    (ATmega2560 datasheet allows up to 1 MHz for 8-bit accuracy,
//     and in practice 10-bit results are usable up to ~1 MHz)
//
//  With prescaler /16:
//    ADC clock = 1 MHz, conversion = 13 µs < 22.7 µs period ✓
//
//  Wiring
//  ──────
//  Signal  →  A0  (keep source impedance < 10 kΩ)
//  GND     →  GND
//  Optional: 100 nF ceramic from A0 to GND near the pin
// =============================================================================

#include <avr/io.h>
#include <avr/interrupt.h>

// ── Timing constants ──────────────────────────────────────────────────────────
#define TARGET_FS       44077UL          // Hz
#define F_CPU_HZ        16000000UL
#define TIMER1_PERIOD   (F_CPU_HZ / TARGET_FS)          // = 363 counts
#define TCNT1_RELOAD    (0x10000UL - TIMER1_PERIOD)     // = 65173

// ── Buffer ────────────────────────────────────────────────────────────────────
#define BUFFER_SIZE     256              // samples per packet (keep power-of-2)

static const uint8_t SYNC[4] = { 0xAA, 0x55, 0xFF, 0x00 };

volatile uint16_t buf[2][BUFFER_SIZE];
volatile uint16_t writeIdx = 0;
volatile uint8_t  writeBuf = 0;
volatile bool     bufReady = false;
volatile uint8_t  readyBuf = 0;
volatile uint32_t overflowCount = 0;   // ISR-detected buffer overruns

// =============================================================================
//  setupADC()
//  Prescaler /16 → ADC clock 1 MHz → 13 µs per conversion.
//  Auto-trigger source: Timer1 Overflow  (ADTS[2:0] = 110)
// =============================================================================
static void setupADC()
{
    // AVcc reference, MUX = ADC0 (A0)
    ADMUX = (1 << REFS0);

    // Enable ADC, auto-trigger, interrupt; prescaler /16 (ADPS = 100)
    ADCSRA = (1 << ADEN)
           | (1 << ADATE)
           | (1 << ADIE)
           | (1 << ADPS2)   // ADPS2=1, ADPS1=0, ADPS0=0  → /16
           | (0 << ADPS1)
           | (0 << ADPS0);

    // Trigger source = Timer/Counter1 Overflow  (ADTS[2:0] = 110)
    ADCSRB = (1 << ADTS2) | (1 << ADTS1) | (0 << ADTS0);

    // Disable digital input buffer on ADC0 pin to reduce noise
    DIDR0 = (1 << ADC0D);
}

// =============================================================================
//  setupTimer1()
//  Normal mode, prescaler /1.
//  TCNT1 is preloaded so the timer overflows at exactly TARGET_FS Hz.
//  The overflow ISR reloads TCNT1 to maintain the period.
// =============================================================================
static void setupTimer1()
{
    TCCR1A = 0x00;                       // Normal mode
    TCCR1B = 0x00;
    TCNT1  = TCNT1_RELOAD;

    TIMSK1 = (1 << TOIE1);              // Enable overflow interrupt for reload

    TCCR1B = (1 << CS10);               // Start timer, prescaler = 1
}

// =============================================================================
//  Timer1 Overflow ISR — reload TCNT1 to maintain precise sample period
// =============================================================================
ISR(TIMER1_OVF_vect)
{
    // Reload immediately — the 4-cycle ISR entry latency adds a tiny fixed
    // offset (~0.25 µs) which is constant and does not affect frequency accuracy.
    TCNT1 = TCNT1_RELOAD;
    // TOV1 is cleared automatically by hardware on ISR entry.
    // The ADC auto-trigger fires on the rising edge of TOV1, which happened
    // just before this ISR was entered — conversion is already in progress.
}

// =============================================================================
//  ADC Conversion Complete ISR
// =============================================================================
ISR(ADC_vect)
{
    uint16_t sample = ADC;          // Read ADCL + ADCH atomically via SFR macro

    if (bufReady) {
        overflowCount++;            // Main loop is too slow — count dropped samples
        return;
    }

    buf[writeBuf][writeIdx] = sample;

    if (++writeIdx >= BUFFER_SIZE) {
        writeIdx = 0;
        readyBuf = writeBuf;
        writeBuf ^= 1;
        bufReady  = true;
    }
}

// =============================================================================
//  setup / loop
// =============================================================================
void setup()
{
    Serial.begin(1000000);
    delay(1000);                    // Let the host open the port

    setupADC();
    setupTimer1();
    sei();
}

void loop()
{
    if (!bufReady) return;

    uint8_t idx = readyBuf;         // snapshot before clearing flag

    Serial.write(SYNC, 4);
    Serial.write((const uint8_t*) buf[idx], BUFFER_SIZE * 2);

    bufReady = false;               // release buffer back to ISR
}
