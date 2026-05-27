import csv
import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import firwin, lfilter, filtfilt

# ============================================================
#  CONFIGURATION — edit per-file settings here
# ============================================================

CSV_FILES = ['sigA.csv', 'sigB.csv', 'sigC.csv', 'sigD.csv']

MAF_SETTINGS = {
    'sigA.csv': {'n': 25},
    'sigB.csv': {'n': 25},
    'sigC.csv': {'n': 25},
    'sigD.csv': {'n': 25},
}

IIR_SETTINGS = {
    'sigA.csv': {'alpha': 0.01},
    'sigB.csv': {'alpha': 0.01},
    'sigC.csv': {'alpha': 0.01},
    'sigD.csv': {'alpha': 0.01},
}

FIR_SETTINGS = {
    'sigA.csv': {'numtaps': 3001, 'cutoff': 8, 'type': 'lowpass',  'bandwidth': None},
    'sigB.csv': {'numtaps': 3001, 'cutoff': 8,  'type': 'lowpass',  'bandwidth': None},
    'sigC.csv': {'numtaps': 3001, 'cutoff': 8, 'type': 'bandpass', 'bandwidth': None},
    'sigD.csv': {'numtaps': 51, 'cutoff': 8,  'type': 'lowpass',  'bandwidth': None},
}

# ============================================================
#  HELPER: read a two-column CSV  →  (t[], data[])
# ============================================================
def read_csv(filename):
    t, data = [], []
    with open(filename) as f:
        reader = csv.reader(f)
        for row in reader:
            t.append(float(row[0]))
            data.append(float(row[1]))
    return np.array(t), np.array(data)

# ============================================================
#  HELPER: compute one-sided FFT for plotting
# ============================================================
def compute_fft(signal, t):
    n  = len(signal)
    dt = t[1] - t[0]           # sample interval
    Fs = 1.0 / dt              # sample rate
    Y  = np.fft.fft(signal) / n
    Y  = Y[:n // 2]
    frq = np.fft.fftfreq(n, d=dt)[:n // 2]
    return frq, np.abs(Y)

# ============================================================
#  HELPER: plot signal + FFT side by side (before & after)
# ============================================================
def plot_results(t, raw, filtered, title):
    frq_raw, fft_raw       = compute_fft(raw,      t)
    frq_filt, fft_filt     = compute_fft(filtered,  t)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6))
    fig.suptitle(title, fontsize=11, fontweight='bold')

    # --- time-domain ---
    ax1.plot(t, raw,      color='black', label='Unfiltered', linewidth=0.8)
    ax1.plot(t, filtered, color='red',   label='Filtered',   linewidth=1.2)
    ax1.set_xlabel('Time [s]')
    ax1.set_ylabel('Signal')
    ax1.legend()

    # --- frequency-domain ---
    ax2.loglog(frq_raw,  fft_raw,  color='black', label='Unfiltered', linewidth=0.8)
    ax2.loglog(frq_filt, fft_filt, color='red',   label='Filtered',   linewidth=1.2)
    ax2.set_xlabel('Freq [Hz]')
    ax2.set_ylabel('|Y(freq)|')
    ax2.legend()

    plt.tight_layout()
    plt.show()

# ============================================================
#  FILTER IMPLEMENTATIONS
# ============================================================

def apply_maf(data, n):
    """Moving Average Filter — average over n points."""
    filtered = np.zeros(len(data))
    for i in range(len(data)):
        start = max(0, i - n + 1)
        filtered[i] = np.mean(data[start:i + 1])
    return filtered

def apply_iir(data, alpha):
    """IIR low-pass: y[i] = alpha*x[i] + (1-alpha)*y[i-1]"""
    filtered = np.zeros(len(data))
    filtered[0] = data[0]
    for i in range(1, len(data)):
        filtered[i] = alpha * data[i] + (1 - alpha) * filtered[i - 1]
    return filtered

def apply_fir(data, t, numtaps, cutoff, bw, ftype):
    """FIR filter using scipy.signal.firwin."""
    dt = t[1] - t[0]
    Fs = 1.0 / dt
    nyq = Fs / 2.0

    if ftype == 'bandpass' and bw is not None:
        low  = (cutoff - bw / 2) / nyq
        high = (cutoff + bw / 2) / nyq
        low  = np.clip(low,  1e-6, 1 - 1e-6)
        high = np.clip(high, 1e-6, 1 - 1e-6)
        taps = firwin(numtaps, [low, high], pass_zero=False)
    else:
        norm_cutoff = cutoff / nyq
        norm_cutoff = np.clip(norm_cutoff, 1e-6, 1 - 1e-6)
        taps = firwin(numtaps, norm_cutoff)

    return filtfilt(taps, 1.0, data), taps

# ============================================================
#  MAIN LOOP
# ============================================================
for csv_file in CSV_FILES:
    print(f"\nProcessing {csv_file} ...")
    t, data = read_csv(csv_file)

    # ---- MAF ----
    m = MAF_SETTINGS[csv_file]
    maf_out = apply_maf(data, m['n'])
    plot_results(t, data, maf_out,
        f"{csv_file}  |  MAF  —  N = {m['n']} points")

    # ---- IIR ----
    i = IIR_SETTINGS[csv_file]
    iir_out = apply_iir(data, i['alpha'])
    plot_results(t, data, iir_out,
        f"{csv_file}  |  IIR  —  alpha = {i['alpha']}")

    # ---- FIR ----
    s = FIR_SETTINGS[csv_file]
    fir_out, taps = apply_fir(data, t, s['numtaps'], s['cutoff'], s['bandwidth'], s['type'])
    bw_str = f", BW={s['bandwidth']} Hz" if s['type'] == 'bandpass' else ""
    plot_results(t, data, fir_out,
        f"{csv_file}  |  FIR  —  {s['type'].capitalize()}, "
        f"{s['numtaps']} taps, fc={s['cutoff']} Hz{bw_str}")

print("\nDone — all files processed.")