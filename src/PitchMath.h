#pragma once

#include <array>
#include <optional>
#include <string_view>

namespace apertune
{
struct PitchReading
{
    int midiNote = 69;
    double cents = 0.0;
    double referenceFrequencyHz = 440.0;
    std::string_view noteName = "A";
};

bool isValidFrequency(double frequencyHz) noexcept;
double midiNoteFromFrequency(double frequencyHz, double concertAHz = 440.0);
double frequencyFromMidiNote(double midiNote, double concertAHz = 440.0);
std::optional<PitchReading> analyseFrequency(double frequencyHz, double concertAHz = 440.0);
std::string_view noteNameForMidiNote(int midiNote) noexcept;

inline constexpr std::array<std::string_view, 12> chromaticNoteNames {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
} // namespace apertune
