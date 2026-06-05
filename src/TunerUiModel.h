#pragma once

#include "PitchMath.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

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

enum class DisplayUnit
{
    cents = 0,
    hertz
};

enum class InstrumentScope
{
    bass = 0,
    guitar,
    custom
};

enum class TuningPreset
{
    bass4Standard = 0,
    bass5LowB,
    bass6Standard,
    guitar6Standard,
    guitar7LowB,
    guitar8Standard,
    guitar9Standard,
    custom,
    dropD,
    dropA,
    dropE
};

enum class AccidentalSpelling
{
    sharps = 0,
    flats
};

struct TunerSettings
{
    DisplayUnit displayUnit = DisplayUnit::cents;
    InstrumentScope instrumentScope = InstrumentScope::guitar;
    TuningPreset tuningPreset = TuningPreset::guitar6Standard;
    AccidentalSpelling accidentalSpelling = AccidentalSpelling::sharps;
};

struct TuningDefinition
{
    TuningPreset preset = TuningPreset::guitar6Standard;
    InstrumentScope scope = InstrumentScope::guitar;
    std::string_view name = "Guitar 6 Standard";
    std::vector<int> stringMidiNotes {};
    std::vector<std::string> stringLabels {};
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
    std::vector<std::string> stringLabels {};
    std::vector<bool> tunedStrings {};
};

std::string_view noteNameForMidiNote(int midiNote, AccidentalSpelling spelling) noexcept;
std::string_view displayUnitName(DisplayUnit unit) noexcept;
std::string_view instrumentScopeName(InstrumentScope scope) noexcept;
std::string_view tuningPresetName(TuningPreset preset) noexcept;
std::string_view accidentalSpellingName(AccidentalSpelling spelling) noexcept;
std::vector<TuningPreset> presetsForScope(InstrumentScope scope);
TuningPreset defaultPresetForScope(InstrumentScope scope) noexcept;
bool presetBelongsToScope(TuningPreset preset, InstrumentScope scope) noexcept;
TuningDefinition tuningDefinitionForPreset(TuningPreset preset);
TuningPreset coercePresetForScope(TuningPreset preset, InstrumentScope scope) noexcept;

TunerUiState classifyTuneState(const std::optional<PitchReading>& reading, bool muted) noexcept;
TuneDirection tuneDirectionForCents(double cents) noexcept;
TunerUiFrame makeTunerUiFrame(
    const std::optional<PitchReading>& reading,
    bool muted,
    double concertAHz = defaultConcertAHz,
    TunerSettings settings = {});
TunerUiFrame deterministicTunerUiFrame(TunerUiState state);
std::string_view tunerUiStateName(TunerUiState state) noexcept;
} // namespace apertune
