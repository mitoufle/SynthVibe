#pragma once

namespace SynthVibe
{
    enum class BP { Compact, Default, Wide };

    inline BP breakpointForWidth(int w)
    {
        if (w < 1150) return BP::Compact;
        if (w < 1550) return BP::Default;
        return BP::Wide;
    }
}
