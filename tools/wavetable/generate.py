#!/usr/bin/env python3
"""Generates Source/Engine/WavetableData.cpp from formulas + AKWF wavs.

Run from repo root:
    python tools/wavetable/generate.py

Optional: numpy for fast FFT (falls back to stdlib cmath DFT if missing).

Hand-coded tables (no external file required):
  - Organ: Hammond drawbar 888000000 (9 partials, integer ratios).
  - Vocal: formant comb F1=730/F2=1090/F3=2440 ('Aa' vowel).
  - Noise: deterministic white noise (fixed seed).

AKWF tables (wav file required at the listed path):
  - EP:   tools/wavetable/akwf-sources/ep.wav
  - Bell: tools/wavetable/akwf-sources/bell.wav

If the AKWF wavs are missing, the script falls back to a sine placeholder
for that entry and prints a warning. This is the default state until Task 16
selects real source files.
"""
# REPRODUCIBILITY NOTE: The committed Source/Engine/WavetableData.cpp was
# generated with numpy's FFT. Re-running this script without numpy uses the
# stdlib DFT fallback, which is DSP-equivalent but accumulates floating-point
# differently — producing the same waveforms but distinct 7th-decimal values.
# That creates a 2700-line spurious git diff. Install numpy before regenerating:
#   pip install numpy
from __future__ import annotations
import math
import random
import struct
import sys
import wave
from pathlib import Path

ROOT       = Path(__file__).resolve().parent.parent.parent
OUT_PATH   = ROOT / "Source" / "Engine" / "WavetableData.cpp"
AKWF_DIR   = ROOT / "tools" / "wavetable" / "akwf-sources"
NUM_TABLES = 5
NUM_MIPS   = 10
MIP0_SIZE  = 2048

# Try to use numpy for fast FFT; fall back to stdlib DFT if unavailable.
try:
    import numpy as np
    HAS_NUMPY = True
except ImportError:
    HAS_NUMPY = False

def normalize(samples, peak=0.95):
    m = max(abs(s) for s in samples) or 1.0
    return [s * peak / m for s in samples]

def make_organ():
    weights = [1.0] * 9
    cycle = []
    for i in range(MIP0_SIZE):
        t = i / MIP0_SIZE
        s = sum(w * math.sin(2 * math.pi * (k + 1) * t) for k, w in enumerate(weights))
        cycle.append(s)
    return normalize(cycle)

def make_vocal():
    f0 = 200.0
    formants = [(730.0, 80.0, 1.0), (1090.0, 90.0, 0.6), (2440.0, 120.0, 0.3)]
    n_harm = MIP0_SIZE // 2
    amplitudes = []
    for k in range(1, n_harm + 1):
        f = k * f0
        a = 0.0
        for (fc, bw, weight) in formants:
            a += weight * math.exp(-0.5 * ((f - fc) / bw) ** 2)
        amplitudes.append(a)
    cycle = []
    for i in range(MIP0_SIZE):
        t = i / MIP0_SIZE
        s = sum(a * math.sin(2 * math.pi * (k + 1) * t) for k, a in enumerate(amplitudes))
        cycle.append(s)
    return normalize(cycle)

def make_noise():
    rng = random.Random(0xA1)
    return normalize([rng.uniform(-1.0, 1.0) for _ in range(MIP0_SIZE)])

def load_wav_resampled(path: Path):
    if not path.exists():
        return None
    with wave.open(str(path), 'rb') as w:
        n_frames = w.getnframes()
        n_chans  = w.getnchannels()
        sw       = w.getsampwidth()
        raw      = w.readframes(n_frames)
    if sw == 2:
        fmt = '<' + 'h' * (n_frames * n_chans)
        ints = struct.unpack(fmt, raw)
        mono = [ints[i] / 32768.0 for i in range(0, n_frames * n_chans, n_chans)]
    elif sw == 3:
        mono = []
        for i in range(0, len(raw), 3 * n_chans):
            b = raw[i:i+3]
            v = int.from_bytes(b, 'little', signed=True)
            mono.append(v / (1 << 23))
    else:
        raise RuntimeError(f"unsupported wav sample width {sw} in {path}")
    out = []
    for i in range(MIP0_SIZE):
        pos = i * (len(mono) - 1) / MIP0_SIZE
        a = int(pos)
        f = pos - a
        b = min(a + 1, len(mono) - 1)
        out.append(mono[a] + f * (mono[b] - mono[a]))
    return normalize(out)

def make_ep():
    candidate = AKWF_DIR / "ep.wav"
    cycle = load_wav_resampled(candidate)
    if cycle is None:
        print(f"WARN: {candidate} missing - EP entry falling back to sine placeholder.",
              file=sys.stderr)
        return [math.sin(2 * math.pi * i / MIP0_SIZE) for i in range(MIP0_SIZE)]
    return cycle

def make_bell():
    candidate = AKWF_DIR / "bell.wav"
    cycle = load_wav_resampled(candidate)
    if cycle is None:
        print(f"WARN: {candidate} missing - Bell entry falling back to sine placeholder.",
              file=sys.stderr)
        return [math.sin(2 * math.pi * i / MIP0_SIZE) for i in range(MIP0_SIZE)]
    return cycle

def bandlimit_to_mip_numpy(cycle, mip_idx):
    """Fast bandlimiting using numpy rfft/irfft."""
    arr = np.array(cycle, dtype=np.float64)
    N = len(arr)
    if mip_idx == 0:
        return arr.tolist()
    spec = np.fft.rfft(arr)         # length N//2 + 1
    half = N // 2
    cutoff = half >> mip_idx        # highest bin to keep
    spec[cutoff + 1:] = 0.0        # zero bins above cutoff
    full = np.fft.irfft(spec, n=N) # back to length N
    step = 1 << mip_idx
    return full[::step].tolist()    # decimate

def bandlimit_to_mip_stdlib(cycle, mip_idx):
    """Stdlib-only bandlimiting using naive O(N^2) DFT."""
    import cmath
    if mip_idx == 0:
        return cycle[:]
    N = len(cycle)
    bins = [sum(cycle[n] * cmath.exp(-2j * math.pi * k * n / N) for n in range(N))
            for k in range(N)]
    half = N // 2
    cutoff = half >> mip_idx
    for k in range(cutoff + 1, N - cutoff):
        bins[k] = 0
    full = [
        (sum(bins[k] * cmath.exp(2j * math.pi * k * n / N) for k in range(N)) / N).real
        for n in range(N)
    ]
    step = 1 << mip_idx
    return [full[i * step] for i in range(N // step)]

def bandlimit_to_mip(cycle, mip_idx):
    if HAS_NUMPY:
        return bandlimit_to_mip_numpy(cycle, mip_idx)
    return bandlimit_to_mip_stdlib(cycle, mip_idx)

def emit_array(name, samples):
    floats_per_line = 8
    lines = [f"static constexpr float {name}[{len(samples)}] = {{"]
    for i in range(0, len(samples), floats_per_line):
        chunk = samples[i:i+floats_per_line]
        lines.append("    " + ", ".join(f"{x:.7f}f" for x in chunk) + ",")
    lines.append("};")
    return "\n".join(lines)

def main():
    approach = "numpy" if HAS_NUMPY else "stdlib DFT (slow)"
    print(f"Generating wavetables using {approach}...", file=sys.stderr)
    tables = {
        "organ": make_organ(),
        "ep":    make_ep(),
        "bell":  make_bell(),
        "vocal": make_vocal(),
        "noise": make_noise(),
    }
    out = []
    out.append("// AUTO-GENERATED by tools/wavetable/generate.py - do not edit by hand.")
    out.append("// Re-run that script after changing source data.")
    out.append("//")
    out.append("// Source files (where applicable):")
    out.append(f"//   ep:   tools/wavetable/akwf-sources/ep.wav  (from AKWF_epiano/AKWF_epiano_0067.wav)")
    out.append(f"//   bell: tools/wavetable/akwf-sources/bell.wav (from AKWF_fmsynth/AKWF_fmsynth_0070.wav)")
    out.append("")
    out.append('#include "WavetableBank.h"')
    out.append("")
    out.append("namespace {")
    for name, cycle in tables.items():
        print(f"  bandlimiting {name}...", file=sys.stderr)
        for mip in range(NUM_MIPS):
            band = bandlimit_to_mip(cycle, mip)
            band = normalize(band)  # re-normalize each mip: bandlimiting is not non-expansive
            out.append(emit_array(f"{name}_mip{mip}", band))
            out.append("")
    out.append("} // namespace")
    out.append("")
    out.append("WavetableBank::WavetableBank() {")
    for idx, name in enumerate(["organ", "ep", "bell", "vocal", "noise"]):
        for mip in range(NUM_MIPS):
            out.append(f"    entries[{idx}].mips[{mip}] = {name}_mip{mip};")
    out.append("}")
    OUT_PATH.write_text("\n".join(out) + "\n")
    print(f"wrote {OUT_PATH}", file=sys.stderr)

if __name__ == "__main__":
    main()
