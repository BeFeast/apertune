#include "TunerUiModel.h"

#include <algorithm>
#include <cmath>

namespace apertune
{
namespace
{
constexpr double nearThresholdCents = 12.0;
constexpr double chevronThresholdCents = 0.5;
constexpr std::array<std::string_view, 12> flatNoteNames {
    "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"
};

double clamp(double value, double low, double high) noexcept
{
    return std::max(low, std::min(high, value));
}

std::string_view previousNoteName(int midiNote, AccidentalSpelling spelling) noexcept
{
    return noteNameForMidiNote(midiNote - 1, spelling);
}

std::string_view nextNoteName(int midiNote, AccidentalSpelling spelling) noexcept
{
    return noteNameForMidiNote(midiNote + 1, spelling);
}

std::vector<std::string> labelsForMidiNotes(const std::vector<int>& midiNotes, AccidentalSpelling spelling)
{
    std::vector<std::string> labels;
    labels.reserve(midiNotes.size());
    for (const auto midiNote : midiNotes)
        labels.emplace_back(noteNameForMidiNote(midiNote, spelling));
    return labels;
}

int nearestStringIndex(int midiNote, const std::vector<int>& stringMidiNotes) noexcept
{
    if (stringMidiNotes.empty())
        return -1;

    auto bestIndex = 0;
    auto bestDistance = std::abs(midiNote - stringMidiNotes.front());
    for (auto index = 1; index < static_cast<int>(stringMidiNotes.size()); ++index)
    {
        const auto distance = std::abs(midiNote - stringMidiNotes[static_cast<std::size_t>(index)]);
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

std::string_view noteNameForMidiNote(int midiNote, AccidentalSpelling spelling) noexcept
{
    if (spelling == AccidentalSpelling::flats)
    {
        const auto index = ((midiNote % 12) + 12) % 12;
        return flatNoteNames[static_cast<std::size_t>(index)];
    }

    return noteNameForMidiNote(midiNote);
}

std::string_view displayUnitName(DisplayUnit unit) noexcept
{
    switch (unit)
    {
        case DisplayUnit::cents:
            return "Cents";
        case DisplayUnit::hertz:
            return "Hz";
    }

    return "Cents";
}

std::string_view instrumentScopeName(InstrumentScope scope) noexcept
{
    switch (scope)
    {
        case InstrumentScope::bass:
            return "Bass";
        case InstrumentScope::guitar:
            return "Guitar";
        case InstrumentScope::custom:
            return "Custom";
    }

    return "Guitar";
}

std::string_view tuningPresetName(TuningPreset preset) noexcept
{
    switch (preset)
    {
        case TuningPreset::bass4Standard:
            return "Bass 4 Standard";
        case TuningPreset::bass5LowB:
            return "Bass 5 Low B";
        case TuningPreset::bass6Standard:
            return "Bass 6 Standard";
        case TuningPreset::guitar6Standard:
            return "Guitar 6 Standard";
        case TuningPreset::guitar7LowB:
            return "Guitar 7 Low B";
        case TuningPreset::guitar8Standard:
            return "Guitar 8 Standard";
        case TuningPreset::guitar9Standard:
            return "Guitar 9 Standard";
        case TuningPreset::custom:
            return "Custom";
        case TuningPreset::dropD:
            return "Drop D";
        case TuningPreset::dropA:
            return "Drop A";
        case TuningPreset::dropE:
            return "Drop E";
    }

    return "Guitar 6 Standard";
}

std::string_view accidentalSpellingName(AccidentalSpelling spelling) noexcept
{
    switch (spelling)
    {
        case AccidentalSpelling::sharps:
            return "Sharps";
        case AccidentalSpelling::flats:
            return "Flats";
    }

    return "Sharps";
}

std::vector<TuningPreset> presetsForScope(InstrumentScope scope)
{
    switch (scope)
    {
        case InstrumentScope::bass:
            return { TuningPreset::bass4Standard, TuningPreset::bass5LowB, TuningPreset::bass6Standard };
        case InstrumentScope::guitar:
            return { TuningPreset::guitar6Standard,
                TuningPreset::dropD,
                TuningPreset::guitar7LowB,
                TuningPreset::dropA,
                TuningPreset::guitar8Standard,
                TuningPreset::dropE,
                TuningPreset::guitar9Standard };
        case InstrumentScope::custom:
            return { TuningPreset::custom };
    }

    return { TuningPreset::guitar6Standard };
}

TuningPreset defaultPresetForScope(InstrumentScope scope) noexcept
{
    switch (scope)
    {
        case InstrumentScope::bass:
            return TuningPreset::bass4Standard;
        case InstrumentScope::guitar:
            return TuningPreset::guitar6Standard;
        case InstrumentScope::custom:
            return TuningPreset::custom;
    }

    return TuningPreset::guitar6Standard;
}

bool presetBelongsToScope(TuningPreset preset, InstrumentScope scope) noexcept
{
    switch (preset)
    {
        case TuningPreset::bass4Standard:
        case TuningPreset::bass5LowB:
        case TuningPreset::bass6Standard:
            return scope == InstrumentScope::bass;
        case TuningPreset::guitar6Standard:
        case TuningPreset::guitar7LowB:
        case TuningPreset::guitar8Standard:
        case TuningPreset::guitar9Standard:
        case TuningPreset::dropD:
        case TuningPreset::dropA:
        case TuningPreset::dropE:
            return scope == InstrumentScope::guitar;
        case TuningPreset::custom:
            return scope == InstrumentScope::custom;
    }

    return false;
}

TuningDefinition tuningDefinitionForPreset(TuningPreset preset)
{
    switch (preset)
    {
        case TuningPreset::bass4Standard:
            return { preset, InstrumentScope::bass, tuningPresetName(preset), { 28, 33, 38, 43 }, { "E", "A", "D", "G" } };
        case TuningPreset::bass5LowB:
            return { preset, InstrumentScope::bass, tuningPresetName(preset), { 23, 28, 33, 38, 43 }, { "B", "E", "A", "D", "G" } };
        case TuningPreset::bass6Standard:
            return { preset, InstrumentScope::bass, tuningPresetName(preset), { 23, 28, 33, 38, 43, 48 }, { "B", "E", "A", "D", "G", "C" } };
        case TuningPreset::guitar6Standard:
            return { preset, InstrumentScope::guitar, tuningPresetName(preset), { 40, 45, 50, 55, 59, 64 }, { "E", "A", "D", "G", "B", "E" } };
        case TuningPreset::guitar7LowB:
            return { preset, InstrumentScope::guitar, tuningPresetName(preset), { 35, 40, 45, 50, 55, 59, 64 }, { "B", "E", "A", "D", "G", "B", "E" } };
        case TuningPreset::guitar8Standard:
            return { preset, InstrumentScope::guitar, tuningPresetName(preset), { 30, 35, 40, 45, 50, 55, 59, 64 }, { "F#", "B", "E", "A", "D", "G", "B", "E" } };
        case TuningPreset::guitar9Standard:
            return { preset, InstrumentScope::guitar, tuningPresetName(preset), { 25, 30, 35, 40, 45, 50, 55, 59, 64 }, { "C#", "F#", "B", "E", "A", "D", "G", "B", "E" } };
        case TuningPreset::dropD:
            return { preset, InstrumentScope::guitar, tuningPresetName(preset), { 38, 45, 50, 55, 59, 64 }, { "D", "A", "D", "G", "B", "E" } };
        case TuningPreset::dropA:
            return { preset, InstrumentScope::guitar, tuningPresetName(preset), { 33, 40, 45, 50, 55, 59, 64 }, { "A", "E", "A", "D", "G", "B", "E" } };
        case TuningPreset::dropE:
            return { preset, InstrumentScope::guitar, tuningPresetName(preset), { 28, 35, 40, 45, 50, 55, 59, 64 }, { "E", "B", "E", "A", "D", "G", "B", "E" } };
        case TuningPreset::custom:
            return { preset, InstrumentScope::custom, tuningPresetName(preset), { 40, 45, 50, 55, 59, 64 }, { "E", "A", "D", "G", "B", "E" } };
    }

    return tuningDefinitionForPreset(TuningPreset::guitar6Standard);
}

TuningPreset coercePresetForScope(TuningPreset preset, InstrumentScope scope) noexcept
{
    return presetBelongsToScope(preset, scope) ? preset : defaultPresetForScope(scope);
}

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

TunerUiFrame makeTunerUiFrame(
    const std::optional<PitchReading>& reading,
    bool muted,
    double concertAHz,
    TunerSettings settings)
{
    const auto state = classifyTuneState(reading, muted);
    const auto preset = coercePresetForScope(settings.tuningPreset, settings.instrumentScope);
    auto tuning = tuningDefinitionForPreset(preset);
    tuning.stringLabels = labelsForMidiNotes(tuning.stringMidiNotes, settings.accidentalSpelling);

    TunerUiFrame frame;
    frame.state = state;
    frame.muted = muted;
    frame.stringLabels = tuning.stringLabels;
    frame.tunedStrings.assign(frame.stringLabels.size(), false);

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
        frame.noteName = noteNameForMidiNote(reading->midiNote, settings.accidentalSpelling);
        frame.previousNoteName = previousNoteName(reading->midiNote, settings.accidentalSpelling);
        frame.nextNoteName = nextNoteName(reading->midiNote, settings.accidentalSpelling);
        frame.activeStringIndex = nearestStringIndex(reading->midiNote, tuning.stringMidiNotes);
    }
    else
    {
        frame.visibility = muted ? 0.26 : 0.18;
        frame.activeStringIndex = muted && !frame.stringLabels.empty() ? 0 : -1;
    }

    frame.direction = tuneDirectionForCents(frame.cents);
    frame.showChevron = frame.hasSignal && frame.direction != TuneDirection::none;
    frame.ballPosition = 0.5 + clamp(frame.cents / 50.0, -1.0, 1.0) * 0.5;
    frame.reticleDiameter = frame.inLock ? 78.0 : 102.0;
    frame.panelGlow = frame.inLock;

    if (frame.inLock
        && frame.activeStringIndex >= 0
        && frame.activeStringIndex < static_cast<int>(frame.tunedStrings.size()))
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
