#include "WavetableBank.h"
#include <algorithm>
#include <cmath>

float WavetableBank::fetchSample(int tableIdx, double phase, double dt) const noexcept
{
    // Precondition: dt > 0. See header.
    const int mipIdx = std::clamp(
        static_cast<int>(std::ceil(std::log2(dt * static_cast<double>(Mip0Size)))),
        0, NumMips - 1);

    const int mipLen = Mip0Size >> mipIdx;
    const float* mip = entries[tableIdx].mips[mipIdx];

    const double pos    = phase * mipLen;
    const int    i      = static_cast<int>(pos);
    const float  frac   = static_cast<float>(pos - i);
    const int    iNext  = (i + 1) >= mipLen ? 0 : (i + 1);

    return mip[i] + frac * (mip[iNext] - mip[i]);
}

const float* WavetableBank::getCycle(int tableIdx) const noexcept
{
    return entries[tableIdx].mips[0];
}
