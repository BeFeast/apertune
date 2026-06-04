#include "PitchMath.h"

#include <cmath>

namespace apertune
{
bool isValidFrequency(double frequencyHz) noexcept
{
    return std::isfinite(frequencyHz) && frequencyHz > 0.0;
}

bool isSupportedConcertA(double concertAHz) noexcept
{
    return std::isfinite(concertAHz)
        && concertAHz >= minConcertAHz
        && concertAHz <= maxConcertAHz;
}

bool isInLock(double cents) noexcept
{
    return std::isfinite(cents) && std::abs(cents) <= lockThresholdCents;
}

int octaveForMidiNote(int midiNote) noexcept
{
    // MIDI octave convention: C-1 is MIDI 0, middle C is C4 (MIDI 60), A4 is MIDI 69.
    const auto pitchClass = ((midiNote % 12) + 12) % 12;
    return (midiNote - pitchClass) / 12 - 1;
}

std::string_view noteNameForMidiNote(int midiNote) noexcept
{
    const auto index = ((midiNote % 12) + 12) % 12;
    return chromaticNoteNames[static_cast<std::size_t>(index)];
}

double midiNoteFromFrequency(double frequencyHz, double concertAHz)
{
    return 69.0 + 12.0 * std::log2(frequencyHz / concertAHz);
}

double frequencyFromMidiNote(double midiNote, double concertAHz)
{
    return concertAHz * std::pow(2.0, (midiNote - 69.0) / 12.0);
}

std::optional<PitchReading> analyseFrequency(double frequencyHz, double concertAHz)
{
    if (!isValidFrequency(frequencyHz) || !isSupportedConcertA(concertAHz))
        return std::nullopt;

    const auto midi = midiNoteFromFrequency(frequencyHz, concertAHz);
    const auto nearestMidi = static_cast<int>(std::lround(midi));
    const auto referenceFrequency = frequencyFromMidiNote(static_cast<double>(nearestMidi), concertAHz);
    const auto cents = (midi - static_cast<double>(nearestMidi)) * 100.0;

    PitchReading reading;
    reading.midiNote = nearestMidi;
    reading.octave = octaveForMidiNote(nearestMidi);
    reading.cents = cents;
    reading.referenceFrequencyHz = referenceFrequency;
    reading.noteName = noteNameForMidiNote(nearestMidi);
    reading.inLock = isInLock(cents);
    return reading;
}
} // namespace apertune
