#!/usr/bin/env python3
"""
diagnose_esp32.py  —  Raw stream inspector for ESP32 I2S-ADC DMA sketch
════════════════════════════════════════════════════════════════════════
Run this BEFORE monitor.py to see exactly what the ESP32 is sending.

Usage:
    python diagnose_esp32.py --port /dev/ttyUSB0
    python diagnose_esp32.py --port COM4 --bytes 2048
"""

import argparse
import struct
import sys
import time
import serial

SYNC = bytes([0xAA, 0x55, 0xFF, 0x00])


def hexdump(data: bytes, width: int = 16):
    for i in range(0, min(len(data), 512), width):
        chunk = data[i:i + width]
        hx  = ' '.join(f'{b:02X}' for b in chunk)
        asc = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
        print(f"  {i:04X}  {hx:<{width*3}}  {asc}")


def decode_i2s_words(data: bytes, n: int = 16):
    """
    Show the first n uint16 words interpreted as raw I2S-ADC DMA output.
    ESP32 I2S ADC DMA word layout:
      bits [15:12] = ADC channel number  (should be 0 for ADC1_CH0 / GPIO36)
      bits [11: 0] = 12-bit ADC result
    """
    print(f"\n  First {n} uint16 words decoded as I2S-ADC DMA format:")
    print(f"  {'idx':>4}  {'raw hex':>8}  {'chan':>6}  {'adc12':>6}  {'~mV':>6}")
    print(f"  {'─'*4}  {'─'*8}  {'─'*6}  {'─'*6}  {'─'*6}")
    for i in range(min(n, len(data) // 2)):
        word = struct.unpack_from('<H', data, i * 2)[0]
        chan  = (word >> 12) & 0xF
        adc12 = word & 0x0FFF
        mv    = round(adc12 * 3300 / 4095)
        print(f"  {i:>4}  {word:>08X}  {chan:>6}  {adc12:>6}  {mv:>6}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port",    required=True)
    ap.add_argument("--baud",    type=int, default=2_000_000)
    ap.add_argument("--bytes",   type=int, default=2048,
                    help="How many bytes to capture (default 2048)")
    ap.add_argument("--timeout", type=float, default=6.0)
    args = ap.parse_args()

    print(f"\n{'='*64}")
    print(f"  ESP32 ADC-DMA stream diagnostic")
    print(f"  Port: {args.port}   Baud: {args.baud}")
    print(f"{'='*64}\n")

    try:
        ser = serial.Serial(args.port, args.baud, timeout=args.timeout)
    except serial.SerialException as e:
        print(f"[ERROR] {e}")
        sys.exit(1)

    print(f"[1] Waiting 2 s for ESP32 to boot …")
    time.sleep(2)
    ser.reset_input_buffer()

    print(f"[2] Reading {args.bytes} bytes …")
    t0   = time.time()
    data = ser.read(args.bytes)
    elapsed = time.time() - t0
    ser.close()

    print(f"    Received {len(data)} bytes in {elapsed:.2f} s")

    if len(data) == 0:
        print("\n[FAIL] No bytes received.")
        print("  • Check port name and baud rate")
        print("  • Make sure sketch is uploaded and running")
        print("  • Try pressing the EN/RESET button on the ESP32")
        sys.exit(1)

    # ── Throughput check ──────────────────────────────────────────────────────
    measured_bps = len(data) / elapsed
    expected_bps = 44100 * 2 + 4 / (512 * 2 / 44100)  # rough: 2 bytes/sample + sync overhead
    print(f"    Throughput: {measured_bps:.0f} bytes/s  "
          f"(expected ≈ {44100*2:.0f} bytes/s for 44 kHz 16-bit mono)")

    # ── Hex dump ──────────────────────────────────────────────────────────────
    print("\n[3] Hex dump (first 256 bytes):")
    hexdump(data)

    # ── Sync word search ──────────────────────────────────────────────────────
    sync_idx = data.find(SYNC)
    print(f"\n[4] Sync word {SYNC.hex()} search:")
    if sync_idx >= 0:
        print(f"    ✓ Found at byte offset {sync_idx}")
        after = data[sync_idx + 4:]
        if len(after) >= 4:
            first_sample = struct.unpack_from('<H', after, 0)[0]
            print(f"    First uint16 after sync: {first_sample}  "
                  f"(as mV if ESP32 calibrated: {first_sample} mV, "
                  f"as raw ADC if not: {first_sample}/4095 × 3.3V = "
                  f"{first_sample*3.3/4095:.3f} V)")
    else:
        print(f"    ✗ Sync word NOT found in {len(data)} bytes")
        print("\n[5] Checking for common issues …")

        # Check 1: Is it printing text (boot log or Serial.print)?
        try:
            text = data.decode('utf-8', errors='replace')
            printable = sum(1 for c in text if c.isprintable())
            if printable / len(text) > 0.7:
                print(f"\n  ⚠  Data looks like TEXT ({printable}/{len(text)} printable chars):")
                print(f"  {repr(text[:200])}")
                print(f"\n  → The ESP32 is sending text, not binary.")
                print(f"    Likely cause: wrong sketch uploaded, or sketch is stuck")
                print(f"    in setup() / hitting an ESP_ERROR_CHECK assertion.")
                print(f"    Open Arduino Serial Monitor at {args.baud} baud to see boot log.")
                sys.exit(1)
        except Exception:
            pass

        # Check 2: Decode raw bytes as I2S words to see if ADC data is there
        # but without the sync wrapper (sketch bug)
        print(f"\n  Decoding raw bytes as I2S-ADC uint16 words (no sync expected):")
        decode_i2s_words(data, n=24)

        # Check 3: Count distinct values — all-zero = ADC not running
        words = [struct.unpack_from('<H', data, i*2)[0]
                 for i in range(len(data) // 2)]
        unique = len(set(words))
        max_v  = max(words)
        min_v  = min(words)
        print(f"\n  Word stats: min={min_v}  max={max_v}  unique={unique}")

        if max_v == 0:
            print("  ✗ All zeros — I2S ADC DMA not producing data")
            print("    → Check i2s_adc_enable() was called")
            print("    → Check ADC1 channel matches GPIO (ADC1_CH0 = GPIO36)")
        elif unique < 10:
            print("  ⚠  Very few unique values — ADC may be reading a DC signal")
            print("    → Connect a varying signal or short GPIO36 to 3.3V/GND to test")
        elif min_v > 0x0FFF and max_v > 0x0FFF:
            print("  ⚠  All words have bits [15:12] set — raw I2S data present but")
            print("    sync word was never written. Check loop()/tx_task are running.")
        else:
            print("  ✓ Values look like valid ADC data — sync word is the only issue")
            print("    → The sketch is producing ADC samples but not wrapping them")
            print("       in the sync+packet format. Check tx_task is running on Core 0.")

    print("\n[Done]\n")


if __name__ == "__main__":
    main()
