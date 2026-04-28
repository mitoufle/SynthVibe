# Third-Party Resources

## Wavetables

The "EP" and "Bell" wavetable entries (`Source/Engine/WavetableData.cpp`) are
adapted from single-cycle waveforms in the **Adventure Kid Waveforms (AKWF)**
collection by Kristoffer Ekstrand.

- Source: https://github.com/KristofferKarlAxelEkstrand/AKWF-FREE
- Original site: https://www.adventurekid.se/akrt/waveforms/adventure-kid-waveforms/
- License: CC0 / public domain (no attribution required, but acknowledged here).

The original `.wav` source files are committed under
`tools/wavetable/akwf-sources/` for reproducibility:

- `ep.wav`   — adapted from `AKWF/AKWF_epiano/AKWF_epiano_0067.wav`
- `bell.wav` — adapted from `AKWF/AKWF_fmsynth/AKWF_fmsynth_0070.wav`
  (AKWF has no `AKWF_bell` category; FM-synthesis is the canonical algorithm
  behind classic bell timbres, so `AKWF_fmsynth` was used as the source pool.)

Sources were resampled to 2048 single-cycle samples and band-limited via FFT
(see `tools/wavetable/generate.py`) to produce the 10 mip levels stored in
`Source/Engine/WavetableData.cpp`.
