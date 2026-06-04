#pragma once

#include "PitchMath.h"

#include <array>
#include <optional>
#include <string_view>

namespace apertune
{
enum class TunerUiState
{
    noSignal,
    flat,
    near,
    inTuneLock,
    sharp,
    muted
};

enum class TuneDirection
{
    none,
    tuneUp,
    tuneDown
};

struct TunerUiFrame
{
    TunerUiState state = TunerUiState::noSignal;
    bool muted = false;
    bool hasSignal = false;
    bool inLock = false;
    TuneDirection direction = TuneDirection::none;
    int midiNote = 69;
    int octave = 4;
    double cents = 0.0;
    double frequencyHz = defaultConcertAHz;
    double visibility = 0.0;
    double ballPosition = 0.5;
    double reticleDiameter = 102.0;
    bool panelGlow = false;
    bool showChevron = false;
    std::string_view noteName = "A";
    std::string_view previousNoteName = "G#";
    std::string_view nextNoteName = "A#";
    int activeStringIndex = -1;
    std::array<bool, 7> tunedStrings {};
};

TunerUiState classifyTuneState(const std::optional<PitchReading>& reading, bool muted) noexcept;
TuneDirection tuneDirectionForCents(double cents) noexcept;
TunerUiFrame makeTunerUiFrame(
    const std::optional<PitchReading>& reading,
    bool muted,
    double concertAHz = defaultConcertAHz);
TunerUiFrame deterministicTunerUiFrame(TunerUiState state);
std::string_view tunerUiStateName(TunerUiState state) noexcept;
} // namespace apertune
