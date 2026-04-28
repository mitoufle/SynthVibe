// AUTO-GENERATED PLACEHOLDER — replaced at Task 15 by tools/wavetable/generate.py.
//
// All 5 entries currently hold a 1-Hz sine across all 10 mip levels.
// This is *not* a usable bank for hearing instrument timbres — it's only
// here to prove the WavetableBank wiring compiles and passes mechanical tests.
#include "WavetableBank.h"
#include <array>
#include <cmath>

namespace
{
    constexpr int NumTables = WavetableBank::NumTables;
    constexpr int NumMips   = WavetableBank::NumMips;
    constexpr int Mip0Size  = WavetableBank::Mip0Size;

    // Total samples across all mips per table = sum of (Mip0Size >> n) for n in [0, NumMips).
    constexpr int totalPerTable()
    {
        int total = 0;
        for (int n = 0; n < NumMips; ++n) total += Mip0Size >> n;
        return total;
    }

    using TableStorage = std::array<float, totalPerTable()>;

    TableStorage makeSineTable()
    {
        TableStorage t {};
        int offset = 0;
        for (int n = 0; n < NumMips; ++n)
        {
            const int len = Mip0Size >> n;
            for (int i = 0; i < len; ++i)
                t[offset + i] = static_cast<float>(std::sin(2.0 * 3.14159265358979323846 * i / len));
            offset += len;
        }
        return t;
    }

    const TableStorage& storage(int /*idx*/)
    {
        static const TableStorage s = makeSineTable();
        return s;   // PLACEHOLDER: same data for all 5 entries.
    }
}

WavetableBank::WavetableBank()
{
    for (int t = 0; t < NumTables; ++t)
    {
        const float* base = storage(t).data();
        int offset = 0;
        for (int n = 0; n < NumMips; ++n)
        {
            entries[t].mips[n] = base + offset;
            offset += Mip0Size >> n;
        }
    }
}
