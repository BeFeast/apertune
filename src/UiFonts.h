#pragma once

#include <juce_graphics/juce_graphics.h>

#include "BinaryData.h"

// Embedded design typefaces (Direction A): Space Grotesk (UI), Space Mono
// (numerics), Archivo Black (note glyph). Loaded once from BinaryData.
namespace apertune::ui
{
inline juce::Typeface::Ptr loadTypeface(const void* data, int size)
{
    return juce::Typeface::createSystemTypefaceFor(data, static_cast<size_t>(size));
}

inline juce::Typeface::Ptr groteskMedium()
{
    static auto t = loadTypeface(BinaryData::SpaceGrotesk500_ttf, BinaryData::SpaceGrotesk500_ttfSize);
    return t;
}
inline juce::Typeface::Ptr groteskSemi()
{
    static auto t = loadTypeface(BinaryData::SpaceGrotesk600_ttf, BinaryData::SpaceGrotesk600_ttfSize);
    return t;
}
inline juce::Typeface::Ptr groteskBold()
{
    static auto t = loadTypeface(BinaryData::SpaceGrotesk700_ttf, BinaryData::SpaceGrotesk700_ttfSize);
    return t;
}
inline juce::Typeface::Ptr monoRegular()
{
    static auto t = loadTypeface(BinaryData::SpaceMonoRegular_ttf, BinaryData::SpaceMonoRegular_ttfSize);
    return t;
}
inline juce::Typeface::Ptr monoBold()
{
    static auto t = loadTypeface(BinaryData::SpaceMonoBold_ttf, BinaryData::SpaceMonoBold_ttfSize);
    return t;
}
inline juce::Typeface::Ptr archivoBlack()
{
    static auto t = loadTypeface(BinaryData::ArchivoBlackRegular_ttf, BinaryData::ArchivoBlackRegular_ttfSize);
    return t;
}

inline juce::Font font(juce::Typeface::Ptr typeface, float height)
{
    return juce::Font(juce::FontOptions().withTypeface(typeface).withHeight(height));
}
} // namespace apertune::ui
