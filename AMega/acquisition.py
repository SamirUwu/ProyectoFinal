#!/usr/bin/env python3
"""
adc_44khz_receiver.py
─────────────────────
Reads the binary ADC stream from the Arduino Mega sampler sketch,
then plots the time-domain waveform and its FFT spectrum.

Requirements:
    pip install pyserial numpy matplotlib

Usage:
    python adc_44khz_receiver.py --port COM3          # Windows
    python adc_44khz_receiver.py --port /dev/ttyUSB0  # Linux / macOS

Options:
    --port      Serial port (required)
    --baud      Baud rate (default: 2000000)
    --fs        Declared sample rate in Hz (default: 44077)
    --bufsize   Samples per packet, must match Arduino BUFFER_SIZE (default: 512)
    --packets   How many packets to capture before plotting (default: 20)
    --vref      ADC reference voltage in volts (default: 5.0)
    --live      Stream live FFT instead of batch capture (flag)
"""

import argparse
import struct
import sys
import time
import numpy as np
import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# ── Constants ──────────────────────────────────────────────────────────────────
SYNC_WORD = bytes([0xAA, 0x55, 0xFF, 0x00])
ADC_BITS  = 10          # ATmega2560 ADC resolution

# ── Helpers ────────────────────────────────────────────────────────────────────

def find_sync(ser: serial.Serial, sync: bytes = SYNC_WORD) -> bool:
    """Read byte-by-byte until the 4-byte sync word is found. Returns True on success."""
    window = bytearray(len(sync))
    for _ in range(8192):          # give up after 8 kB of garbage
        b = ser.read(1)
        if not b:
            return False
        window = window[1:] + bytearray(b)
        if bytes(window) == sync:
            return True
    return False


def read_packet(ser: serial.Serial, buf_size: int) -> np.ndarray | None:
    """Sync, then read buf_size uint16 samples. Returns ndarray or None."""
    if not find_sync(ser):
        return None
    raw = ser.read(buf_size * 2)
    if len(raw) < buf_size * 2:
        return None
    return np.frombuffer(raw, dtype='<u2').astype(np.float32)


def adc_to_volts(samples: np.ndarray, vref: float = 5.0) -> np.ndarray:
    return samples * vref / (2**ADC_BITS - 1)


def compute_fft(samples: np.ndarray, fs: float):
    n    = len(samples)
    win  = np.hanning(n)
    spec = np.abs(np.fft.rfft(samples * win)) * 2 / n
    freq = np.fft.rfftfreq(n, d=1.0 / fs)
    # Convert to dBFS (full-scale = 5 V p-p → 2.5 V peak)
    spec_db = 20 * np.log10(np.maximum(spec, 1e-9))
    return freq, spec_db


# ── Batch capture & plot ───────────────────────────────────────────────────────

def batch_mode(args):
    print(f"[INFO] Opening {args.port} at {args.baud} baud …")
    ser = serial.Serial(args.port, args.baud, timeout=2)
    time.sleep(0.5)          # let Arduino settle after DTR reset
    ser.reset_input_buffer()

    all_samples = []
    print(f"[INFO] Collecting {args.packets} packets × {args.bufsize} samples …")
    t0 = time.time()
    for i in range(args.packets):
        pkt = read_packet(ser, args.bufsize)
        if pkt is None:
            print(f"[WARN] Packet {i} lost / sync failed", file=sys.stderr)
            continue
        all_samples.append(pkt)
        print(f"  packet {i+1}/{args.packets}", end='\r')

    elapsed   = time.time() - t0
    total_smp = sum(len(p) for p in all_samples)
    measured_fs = total_smp / elapsed
    print(f"\n[INFO] Captured {total_smp} samples in {elapsed:.2f} s")
    print(f"[INFO] Measured Fs ≈ {measured_fs:.0f} Hz  (declared {args.fs} Hz)")
    ser.close()

    samples_v  = adc_to_volts(np.concatenate(all_samples), args.vref)
    t_axis     = np.arange(len(samples_v)) / args.fs * 1e3   # ms
    freq, spec = compute_fft(samples_v, args.fs)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 7))
    fig.suptitle(f"Arduino Mega 2560 — ADC @ {args.fs/1e3:.2f} kHz  |  "
                 f"{len(samples_v)} samples  ({len(samples_v)/args.fs*1e3:.1f} ms)")

    # ─ Time domain ─
    ax1.plot(t_axis[:1024], samples_v[:1024], lw=0.6, color='royalblue')
    ax1.set_xlabel("Time (ms)")
    ax1.set_ylabel("Voltage (V)")
    ax1.set_title("Time domain (first 1024 samples)")
    ax1.set_ylim(-0.1, args.vref + 0.1)
    ax1.grid(True, alpha=0.35)

    # ─ Frequency domain ─
    ax2.plot(freq / 1e3, spec, lw=0.7, color='darkorange')
    ax2.set_xlabel("Frequency (kHz)")
    ax2.set_ylabel("Magnitude (dBFS)")
    ax2.set_title("FFT spectrum")
    ax2.set_xlim(0, args.fs / 2e3)
    ax2.set_ylim(-100, 5)
    ax2.grid(True, alpha=0.35)

    plt.tight_layout()
    plt.savefig("adc_capture.png", dpi=150)
    print("[INFO] Plot saved to adc_capture.png")
    plt.show()


# ── Live streaming FFT ─────────────────────────────────────────────────────────

def live_mode(args):
    print(f"[INFO] Live FFT mode — {args.port} @ {args.baud} baud")
    ser = serial.Serial(args.port, args.baud, timeout=1)
    time.sleep(0.5)
    ser.reset_input_buffer()

    ring     = np.zeros(args.bufsize * 4, dtype=np.float32)
    freq     = np.fft.rfftfreq(len(ring), d=1.0 / args.fs) / 1e3

    fig, ax = plt.subplots(figsize=(10, 4))
    (line,) = ax.plot(freq, np.zeros(len(freq)), lw=0.8, color='darkorange')
    ax.set_xlim(0, args.fs / 2e3)
    ax.set_ylim(-100, 5)
    ax.set_xlabel("Frequency (kHz)")
    ax.set_ylabel("Magnitude (dBFS)")
    ax.set_title("Live FFT — Arduino Mega ADC")
    ax.grid(True, alpha=0.35)

    def update(_frame):
        nonlocal ring
        pkt = read_packet(ser, args.bufsize)
        if pkt is None:
            return (line,)
        v      = adc_to_volts(pkt, args.vref)
        ring   = np.roll(ring, -len(v))
        ring[-len(v):] = v
        _, spec = compute_fft(ring, args.fs)
        line.set_ydata(spec)
        return (line,)

    ani = animation.FuncAnimation(fig, update, interval=50, blit=True, cache_frame_data=False)
    plt.tight_layout()
    plt.show()
    ser.close()


# ── CLI entry point ────────────────────────────────────────────────────────────

def main():
    ap = argparse.ArgumentParser(description="Arduino ADC 44 kHz stream receiver")
    ap.add_argument("--port",    required=True,             help="Serial port")
    ap.add_argument("--baud",    type=int,   default=2_000_000)
    ap.add_argument("--fs",      type=float, default=44_077, help="Declared sample rate (Hz)")
    ap.add_argument("--bufsize", type=int,   default=512,    help="Samples per packet")
    ap.add_argument("--packets", type=int,   default=20,     help="Packets to capture (batch mode)")
    ap.add_argument("--vref",    type=float, default=5.0,    help="ADC reference voltage (V)")
    ap.add_argument("--live",    action="store_true",        help="Enable live FFT display")
    args = ap.parse_args()

    if args.live:
        live_mode(args)
    else:
        batch_mode(args)


if __name__ == "__main__":
    main()
