#include "PitchMath.h"

#include <cmath>

namespace apertune
{
bool isValidFrequency(double frequencyHz) noexcept
{
    return std::isfinite(frequencyHz) && frequencyHz > 0.0;
}

double midiNoteFromFrequency(double frequencyHz, double concertAHz)
{
    return 69.0 + 12.0 * std::log2(frequencyHz / concertAHz);
}

double frequencyFromMidiNote(double midiNote, double concertAHz)
{
    return concertAHz * std::pow(2.0, (midiNote - 69.0) / 12.0);
}

std::string_view noteNameForMidiNote(int midiNote) noexcept
{
    const auto index = ((midiNote % 12) + 12) % 12;
    return chromaticNoteNames[static_cast<std::size_t>(index)];
}

std::optional<PitchReading> analyseFrequency(double frequencyHz, double concertAHz)
{
    if (!isValidFrequency(frequencyHz) || !isValidFrequency(concertAHz))
        return std::nullopt;

    const auto midi = midiNoteFromFrequency(frequencyHz, concertAHz);
    const auto nearestMidi = static_cast<int>(std::lround(midi));
    const auto referenceFrequency = frequencyFromMidiNote(static_cast<double>(nearestMidi), concertAHz);

    PitchReading reading;
    reading.midiNote = nearestMidi;
    reading.cents = (midi - static_cast<double>(nearestMidi)) * 100.0;
    reading.referenceFrequencyHz = referenceFrequency;
    reading.noteName = noteNameForMidiNote(nearestMidi);
    return reading;
}
} // namespace apertune
