#!/usr/bin/env python3
"""
HX711 Data Collector & Plotter
Communicates with the Pico over serial, collects load cell data,
and plots raw vs IIR-filtered voltage against time.
"""

import serial
import serial.tools.list_ports
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import numpy as np
import sys

SAMPLE_RATE_HZ = 80


def list_ports():
    ports = serial.tools.list_ports.comports()
    return [p.device for p in ports]


def select_port():
    ports = list_ports()
    if not ports:
        print("No serial ports found. Is the Pico connected?")
        sys.exit(1)

    if len(ports) == 1:
        print(f"Auto-selected port: {ports[0]}")
        return ports[0]

    print("\nAvailable serial ports:")
    for i, p in enumerate(ports):
        print(f"  [{i}] {p}")
    while True:
        try:
            choice = int(input("Select port number: "))
            if 0 <= choice < len(ports):
                return ports[choice]
        except ValueError:
            pass
        print("Invalid choice, try again.")


def get_duration():
    while True:
        try:
            duration = float(input("\nHow many seconds of data would you like to collect? "))
            if duration > 0:
                return duration
        except ValueError:
            pass
        print("Please enter a positive number.")


def collect_data(port, num_samples, baud=115200):
    print(f"\nConnecting to {port} at {baud} baud...")
    indices, times, iir_vals, raw_vals = [], [], [], []

    with serial.Serial(port, baud, timeout=10) as ser:
        ser.reset_input_buffer()
        print(f"Sending sample count: {num_samples}")
        ser.write(f"{num_samples}\n".encode())

        print(f"Collecting {num_samples} samples", end="", flush=True)
        for _ in range(num_samples):
            line = ser.readline().decode("utf-8", errors="replace").strip()
            if not line:
                continue
            parts = line.split()
            if len(parts) != 4:
                continue
            try:
                idx  = int(parts[0])
                t    = int(parts[1])   # ms since boot
                iir  = int(parts[2])
                raw  = int(parts[3])
                indices.append(idx)
                times.append(t)
                iir_vals.append(iir)
                raw_vals.append(raw)
                if len(indices) % (SAMPLE_RATE_HZ // 4) == 0:
                    print(".", end="", flush=True)
            except ValueError:
                continue

    print(f"\nReceived {len(indices)} samples.")
    return indices, times, iir_vals, raw_vals


def compute_fft(signal, Fs):
    """Return (frequencies, magnitudes) for the one-sided FFT of signal."""
    y = np.array(signal)
    n = len(y)
    k = np.arange(n)
    T = n / Fs
    frq = k / T                          # two-sided frequency range
    frq = frq[range(int(n / 2))]        # one-sided
    Y = np.fft.fft(y) / n               # FFT + normalise
    Y = Y[range(int(n / 2))]
    return frq, np.abs(Y)


def style_ax(ax, title, xlabel, ylabel, color):
    """Apply dark theme styling to an axes."""
    ax.set_title(title, color="#aaaaaa", fontfamily="monospace", fontsize=11, pad=6)
    ax.set_xlabel(xlabel, color="#cccccc", fontfamily="monospace")
    ax.set_ylabel(ylabel, color="#cccccc", fontfamily="monospace")
    ax.tick_params(colors="#888888")
    ax.set_facecolor("#1a1a1a")
    for spine in ax.spines.values():
        spine.set_edgecolor("#333333")
    ax.grid(True, color="#2a2a2a", linewidth=0.5)


def plot_data(times, iir_vals, raw_vals, duration):
    # Normalise time to seconds from zero
    t0    = times[0]
    t_sec = np.array([(t - t0) / 1000.0 for t in times])

    actual_duration = t_sec[-1] - t_sec[0] if len(t_sec) > 1 else duration
    Fs = len(times) / actual_duration      # effective sample rate

    raw_arr = np.array(raw_vals)
    iir_arr = np.array(iir_vals)

    frq_raw, Y_raw = compute_fft(raw_arr, Fs)
    frq_iir, Y_iir = compute_fft(iir_arr, Fs)

    info = (f"Samples: {len(times)}   "
            f"Duration: {actual_duration:.2f}s   "
            f"Effective rate: {Fs:.1f} Hz")

    # ── Figure 1: Raw data + FFT ──────────────────────────────────────────────
    fig1 = plt.figure(figsize=(12, 7), facecolor="#0f0f0f")
    fig1.suptitle("HX711 — Raw ADC", fontsize=16, fontweight="bold",
                  color="#e8e8e8", fontfamily="monospace", y=0.97)
    gs1 = gridspec.GridSpec(2, 1, figure=fig1, hspace=0.5,
                            left=0.09, right=0.96, top=0.91, bottom=0.09)

    ax1a = fig1.add_subplot(gs1[0])
    ax1a.plot(t_sec, raw_arr, color="#00d4ff", linewidth=0.7,
              alpha=0.85, label="Raw ADC")
    style_ax(ax1a, "Raw ADC — Time Domain", "Time (s)", "ADC Counts", "#00d4ff")
    ax1a.legend(facecolor="#222222", edgecolor="#444444",
                labelcolor="#cccccc", fontsize=9)

    ax1b = fig1.add_subplot(gs1[1])
    ax1b.loglog(frq_raw, Y_raw, color="#00d4ff", linewidth=0.8, alpha=0.9,
                label="FFT (Raw)")
    style_ax(ax1b, "Raw ADC — Frequency Domain", "Freq (Hz)", "|Y(freq)|", "#00d4ff")
    ax1b.legend(facecolor="#222222", edgecolor="#444444",
                labelcolor="#cccccc", fontsize=9)

    fig1.text(0.5, 0.01, info, ha="center", fontsize=8,
              color="#666666", fontfamily="monospace")

    # ── Figure 2: IIR data + FFT ──────────────────────────────────────────────
    fig2 = plt.figure(figsize=(12, 7), facecolor="#0f0f0f")
    fig2.suptitle("HX711 — IIR Filtered ADC", fontsize=16, fontweight="bold",
                  color="#e8e8e8", fontfamily="monospace", y=0.97)
    gs2 = gridspec.GridSpec(2, 1, figure=fig2, hspace=0.5,
                            left=0.09, right=0.96, top=0.91, bottom=0.09)

    ax2a = fig2.add_subplot(gs2[0])
    ax2a.plot(t_sec, iir_arr, color="#ff6b35", linewidth=1.2,
              alpha=0.9, label="IIR Filtered (α=0.1)")
    style_ax(ax2a, "IIR Filtered — Time Domain", "Time (s)", "ADC Counts", "#ff6b35")
    ax2a.legend(facecolor="#222222", edgecolor="#444444",
                labelcolor="#cccccc", fontsize=9)

    ax2b = fig2.add_subplot(gs2[1])
    ax2b.loglog(frq_iir, Y_iir, color="#ff6b35", linewidth=0.8, alpha=0.9,
                label="FFT (IIR)")
    style_ax(ax2b, "IIR Filtered — Frequency Domain", "Freq (Hz)", "|Y(freq)|", "#ff6b35")
    ax2b.legend(facecolor="#222222", edgecolor="#444444",
                labelcolor="#cccccc", fontsize=9)

    fig2.text(0.5, 0.01, info, ha="center", fontsize=8,
              color="#666666", fontfamily="monospace")

    plt.show()


def main():
    print("╔══════════════════════════════════╗")
    print("║   HX711 Data Collector & Plotter ║")
    print("╚══════════════════════════════════╝")
    print(f"  Sample rate: {SAMPLE_RATE_HZ} Hz")

    port        = select_port()
    duration    = get_duration()
    num_samples = int(duration * SAMPLE_RATE_HZ)

    print(f"\n  Duration   : {duration} s")
    print(f"  Samples    : {num_samples}  ({duration} × {SAMPLE_RATE_HZ} Hz)")

    indices, times, iir_vals, raw_vals = collect_data(port, num_samples)

    if len(times) < 2:
        print("Not enough data received to plot.")
        sys.exit(1)

    plot_data(times, iir_vals, raw_vals, duration)


if __name__ == "__main__":
    main()