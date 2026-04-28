#pragma once
#include <cstddef>

// Read-only single-cycle wavetable bank with mipmapped (band-limited) tables.
//
// Construction sets up pointers to the constexpr arrays in WavetableData.cpp.
// Audio-thread API (`fetchSample`) is purely read-only and contains no
// allocation, locking, or I/O.
class WavetableBank
{
public:
    static constexpr int NumTables = 5;
    static constexpr int NumMips   = 10;
    static constexpr int Mip0Size  = 2048;   // mip[N] holds Mip0Size / (1 << N) samples

    WavetableBank();

    // Returns one band-limited sample.
    //
    // Preconditions (caller-enforced; bank does not validate):
    //   tableIdx ∈ [0, NumTables)
    //   phase    ∈ [0, 1)
    //   dt       > 0          // required so log2(dt * Mip0Size) is finite
    //
    // Calling with dt == 0 produces undefined behaviour (log2(0) = -∞).
    // Oscillator::getNextSample is the only intended caller and already
    // guarantees dt > 0 since frequency > 0 post-noteOn.
    float fetchSample(int tableIdx, double phase, double dt) const noexcept;

    // Returns a pointer to mip[0] (full-resolution single cycle) for UI
    // visualisation. Callers can read Mip0Size floats from the returned pointer.
    const float* getCycle(int tableIdx) const noexcept;

private:
    struct Entry
    {
        const float* mips[NumMips];   // mips[N] points at a band-limited array of (Mip0Size >> N) floats
    };

    Entry entries[NumTables];
};
