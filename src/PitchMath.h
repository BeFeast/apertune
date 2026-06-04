#pragma once

#include <array>
#include <optional>
#include <string_view>

namespace apertune
{
inline constexpr double defaultConcertAHz = 440.0;
inline constexpr double minConcertAHz = 432.0;
inline constexpr double maxConcertAHz = 448.0;
inline constexpr double lockThresholdCents = 3.0;

inline constexpr std::array<std::string_view, 12> chromaticNoteNames {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

struct PitchReading
{
    int midiNote = 69;
    int octave = 4;
    double cents = 0.0;
    double referenceFrequencyHz = defaultConcertAHz;
    std::string_view noteName = "A";
    bool inLock = true;
};

bool isValidFrequency(double frequencyHz) noexcept;
bool isSupportedConcertA(double concertAHz) noexcept;
bool isInLock(double cents) noexcept;

int octaveForMidiNote(int midiNote) noexcept;
std::string_view noteNameForMidiNote(int midiNote) noexcept;

double midiNoteFromFrequency(double frequencyHz, double concertAHz = defaultConcertAHz);
double frequencyFromMidiNote(double midiNote, double concertAHz = defaultConcertAHz);

std::optional<PitchReading> analyseFrequency(double frequencyHz, double concertAHz = defaultConcertAHz);
} // namespace apertune
