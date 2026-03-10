#!/usr/bin/env python3
"""
diagnose.py  —  Raw serial probe for Arduino ADC sampler
─────────────────────────────────────────────────────────
Run this FIRST to see exactly what bytes the Arduino is sending.
It will tell you:
  • Whether any bytes arrive at all
  • Whether the 4-byte sync word appears
  • A hex dump of the first 256 bytes received

Usage:
    python diagnose.py --port /dev/ttyUSB0
    python diagnose.py --port COM3
"""

import argparse
import time
import sys
import serial

SYNC = bytes([0xAA, 0x55, 0xFF, 0x00])

def hexdump(data: bytes, width: int = 16):
    for i in range(0, len(data), width):
        chunk = data[i:i+width]
        hex_part = ' '.join(f'{b:02X}' for b in chunk)
        asc_part = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
        print(f"  {i:04X}  {hex_part:<{width*3}}  {asc_part}")

def probe(port: str, baud: int, read_bytes: int = 256, timeout: float = 5.0):
    print(f"\n{'='*60}")
    print(f"  Serial probe: {port}  @  {baud} baud")
    print(f"{'='*60}\n")

    try:
        ser = serial.Serial(port, baud, timeout=timeout)
    except serial.SerialException as e:
        print(f"[ERROR] Cannot open port: {e}")
        sys.exit(1)

    # Give Arduino time to reset after DTR toggle
    print("[1] Waiting 2 s for Arduino to boot / reset …")
    time.sleep(2)
    ser.reset_input_buffer()

    print(f"[2] Reading up to {read_bytes} bytes (timeout {timeout} s) …\n")
    t0   = time.time()
    data = ser.read(read_bytes)
    elapsed = time.time() - t0

    print(f"    Received {len(data)} bytes in {elapsed:.2f} s")

    if len(data) == 0:
        print("\n[FAIL] NO BYTES RECEIVED — possible causes:")
        print("  • Wrong port name")
        print("  • Arduino sketch not uploaded / not running")
        print("  • Baud rate mismatch (Arduino must call Serial.begin(2000000))")
        print("  • USB cable is charge-only (no data lines)")
        print("  • Another process (e.g. Arduino IDE Serial Monitor) has the port open")
        ser.close()
        return

    print("\n[3] Hex dump of received bytes:")
    hexdump(data)

    # Search for sync word
    idx = data.find(SYNC)
    if idx == -1:
        print(f"\n[WARN] Sync word {SYNC.hex()} NOT found in {len(data)} bytes.")
        print("  Possible causes:")
        print("  • Arduino is sending plain text (Serial.println) instead of binary")
        print("    → Check that you uploaded adc_44khz_sampler.ino, not a test sketch")
        print("  • Sync constant in sketch differs from receiver")
        print("  • Buffer overrun: ISR filling faster than Serial draining")
        print("\n  ASCII interpretation of first 128 bytes:")
        print("  " + repr(data[:128]))
    else:
        print(f"\n[OK]  Sync word found at byte offset {idx}")
        after_sync = data[idx+4:]
        print(f"      {len(after_sync)} bytes follow the sync word in this dump")
        if len(after_sync) >= 2:
            # Show first few sample values
            import struct
            n = min(len(after_sync) // 2, 8)
            vals = struct.unpack_from(f'<{n}H', after_sync)
            print(f"      First {n} uint16 sample values: {list(vals)}")
            if all(v == 0 for v in vals):
                print("      [WARN] All zeros — A0 may be floating or sketch has a bug")
            elif all(v == 1023 for v in vals):
                print("      [WARN] All 1023 — A0 may be pulled to VCC")
            else:
                print("      [OK]  Values look reasonable for ADC data")

    ser.close()
    print("\n[Done]\n")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port",  required=True)
    ap.add_argument("--baud",  type=int, default=2_000_000)
    ap.add_argument("--bytes", type=int, default=512,
                    help="How many bytes to capture for the dump (default 512)")
    ap.add_argument("--timeout", type=float, default=6.0)
    args = ap.parse_args()
    probe(args.port, args.baud, args.bytes, args.timeout)

if __name__ == "__main__":
    main()
