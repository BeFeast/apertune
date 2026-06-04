#include "TunerUiModel.h"

#include <algorithm>
#include <cmath>

namespace apertune
{
namespace
{
constexpr double nearThresholdCents = 12.0;
constexpr double chevronThresholdCents = 0.5;
constexpr std::array<int, 7> guitarStringMidiNotes { 35, 40, 45, 50, 55, 59, 64 };

double clamp(double value, double low, double high) noexcept
{
    return std::max(low, std::min(high, value));
}

std::string_view previousNoteName(int midiNote) noexcept
{
    return noteNameForMidiNote(midiNote - 1);
}

std::string_view nextNoteName(int midiNote) noexcept
{
    return noteNameForMidiNote(midiNote + 1);
}

int nearestStringIndex(int midiNote) noexcept
{
    auto bestIndex = 0;
    auto bestDistance = std::abs(midiNote - guitarStringMidiNotes[0]);
    for (auto index = 1; index < static_cast<int>(guitarStringMidiNotes.size()); ++index)
    {
        const auto distance = std::abs(midiNote - guitarStringMidiNotes[static_cast<std::size_t>(index)]);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            bestIndex = index;
        }
    }
    return bestIndex;
}

PitchReading fixtureReading(int midiNote, double cents, double visibility = 1.0)
{
    PitchReading reading;
    reading.midiNote = midiNote;
    reading.octave = octaveForMidiNote(midiNote);
    reading.cents = cents;
    reading.referenceFrequencyHz = frequencyFromMidiNote(static_cast<double>(midiNote));
    reading.visibility = visibility;
    reading.noteName = noteNameForMidiNote(midiNote);
    reading.inLock = isInLock(cents);
    return reading;
}
} // namespace

TunerUiState classifyTuneState(const std::optional<PitchReading>& reading, bool muted) noexcept
{
    if (muted)
        return TunerUiState::muted;
    if (!reading || reading->visibility <= 0.0)
        return TunerUiState::noSignal;
    if (reading->inLock)
        return TunerUiState::inTuneLock;

    const auto absCents = std::abs(reading->cents);
    if (absCents <= nearThresholdCents)
        return TunerUiState::near;
    return reading->cents < 0.0 ? TunerUiState::flat : TunerUiState::sharp;
}

TuneDirection tuneDirectionForCents(double cents) noexcept
{
    if (!std::isfinite(cents) || std::abs(cents) <= chevronThresholdCents)
        return TuneDirection::none;
    return cents < 0.0 ? TuneDirection::tuneUp : TuneDirection::tuneDown;
}

TunerUiFrame makeTunerUiFrame(const std::optional<PitchReading>& reading, bool muted, double concertAHz)
{
    const auto state = classifyTuneState(reading, muted);

    TunerUiFrame frame;
    frame.state = state;
    frame.muted = muted;

    if (reading)
    {
        frame.hasSignal = reading->visibility > 0.0 && !muted;
        frame.inLock = reading->inLock && !muted;
        frame.midiNote = reading->midiNote;
        frame.octave = reading->octave;
        frame.cents = clamp(reading->cents, -50.0, 50.0);
        frame.frequencyHz = frequencyFromMidiNote(
            static_cast<double>(reading->midiNote) + reading->cents / 100.0,
            isSupportedConcertA(concertAHz) ? concertAHz : defaultConcertAHz);
        frame.visibility = muted ? 0.32 : clamp(reading->visibility, 0.0, 1.0);
        frame.noteName = reading->noteName;
        frame.previousNoteName = previousNoteName(reading->midiNote);
        frame.nextNoteName = nextNoteName(reading->midiNote);
        frame.activeStringIndex = nearestStringIndex(reading->midiNote);
    }
    else
    {
        frame.visibility = muted ? 0.26 : 0.18;
        frame.activeStringIndex = muted ? 0 : -1;
    }

    frame.direction = tuneDirectionForCents(frame.cents);
    frame.showChevron = frame.hasSignal && frame.direction != TuneDirection::none;
    frame.ballPosition = 0.5 + clamp(frame.cents / 50.0, -1.0, 1.0) * 0.5;
    frame.reticleDiameter = frame.inLock ? 78.0 : 102.0;
    frame.panelGlow = frame.inLock;

    if (frame.inLock && frame.activeStringIndex >= 0)
        frame.tunedStrings[static_cast<std::size_t>(frame.activeStringIndex)] = true;

    return frame;
}

TunerUiFrame deterministicTunerUiFrame(TunerUiState state)
{
    switch (state)
    {
        case TunerUiState::noSignal:
            return makeTunerUiFrame(std::nullopt, false);
        case TunerUiState::flat:
            return makeTunerUiFrame(fixtureReading(45, -28.0), false);
        case TunerUiState::near:
            return makeTunerUiFrame(fixtureReading(50, -7.2), false);
        case TunerUiState::inTuneLock:
            return makeTunerUiFrame(fixtureReading(40, 0.0), false);
        case TunerUiState::sharp:
            return makeTunerUiFrame(fixtureReading(55, 18.5), false);
        case TunerUiState::muted:
            return makeTunerUiFrame(fixtureReading(40, 0.0, 0.7), true);
    }

    return makeTunerUiFrame(std::nullopt, false);
}

std::string_view tunerUiStateName(TunerUiState state) noexcept
{
    switch (state)
    {
        case TunerUiState::noSignal:
            return "no_signal";
        case TunerUiState::flat:
            return "flat";
        case TunerUiState::near:
            return "near";
        case TunerUiState::inTuneLock:
            return "in_tune_lock";
        case TunerUiState::sharp:
            return "sharp";
        case TunerUiState::muted:
            return "muted";
    }

    return "unknown";
}
} // namespace apertune
