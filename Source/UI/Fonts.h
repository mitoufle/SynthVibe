#pragma once
#include <juce_graphics/juce_graphics.h>
#include "DesignTokens.h"

// When the user drops JetBrains Mono / Inter / Instrument Serif .ttf files
// into Resources/Fonts/ and re-runs CMake with juce_add_binary_data, these
// accessors will resolve to the bundled files. Until then they fall back
// to JUCE's platform defaults so nothing breaks.
namespace SynthVibe::Fonts
{
    inline juce::Font mono (float height = Tokens::Font::body)
    {
        // TODO(font-embedding): replace with juce::Typeface::createSystemTypefaceFor(
        //     BinaryData::JetBrainsMono_Regular_ttf, BinaryData::JetBrainsMono_Regular_ttfSize)
        // once Resources/Fonts/JetBrainsMono-Regular.ttf is added to juce_add_binary_data.
        return juce::Font(juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain);
    }

    inline juce::Font sans (float height = Tokens::Font::body, int style = juce::Font::plain)
    {
        // TODO(font-embedding): replace with Inter BinaryData.
        return juce::Font(juce::Font::getDefaultSansSerifFontName(), height, style);
    }

    inline juce::Font serif (float height = Tokens::Font::presetTitle, int style = juce::Font::italic)
    {
        // TODO(font-embedding): replace with Instrument Serif BinaryData.
        return juce::Font(juce::Font::getDefaultSerifFontName(), height, style);
    }
}
