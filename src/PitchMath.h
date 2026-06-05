#pragma once

#include <array>
#include <cstddef>
#include <deque>
#include <optional>
#include <string_view>
#include <vector>

namespace apertune
{
inline constexpr double defaultConcertAHz = 440.0;
inline constexpr double minConcertAHz = 432.0;
inline constexpr double maxConcertAHz = 448.0;
inline constexpr double concertAStepHz = 0.1;
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
    double visibility = 1.0;
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

struct PitchDetectorConfig
{
    double minFrequencyHz = 28.0;
    double maxFrequencyHz = 1400.0;
    double silenceRmsThreshold = 0.0015;
    double yinThreshold = 0.15;
    double updateIntervalSeconds = 0.025;
    double jitterTimeConstantSeconds = 0.10;
    double turnTimeConstantSeconds = 0.025;
    double turnThresholdCents = 12.0;
    double releaseHoldSeconds = 0.16;
    double releaseFadeSeconds = 0.18;
};

class RealTimePitchDetector
{
public:
    explicit RealTimePitchDetector(PitchDetectorConfig config = {});

    void prepare(double newSampleRate);
    void reset();

    template <typename SampleType>
    std::optional<PitchReading> processSamples(
        const SampleType* samples, std::size_t count, double concertAHz = defaultConcertAHz)
    {
        for (std::size_t i = 0; i < count; ++i)
        {
            pushSample(static_cast<double>(samples[i]));
            ++samplesSinceAnalysis;
            if (samplesSinceAnalysis >= hopSize)
            {
                samplesSinceAnalysis = 0;
                analysePending(concertAHz);
            }
        }

        return currentReading;
    }

    std::optional<PitchReading> getCurrentReading() const { return currentReading; }

private:
    void pushSample(double sample);
    void analysePending(double concertAHz);
    std::optional<double> estimateFrequency();
    double calculateRecentRms() const;
    std::optional<PitchReading> smoothReading(double frequencyHz, double concertAHz);
    void updateRelease(bool signalPresent);

    PitchDetectorConfig config;
    double sampleRate = 44100.0;
    std::size_t windowSize = 4096;
    std::size_t hopSize = 1024;
    std::size_t samplesSinceAnalysis = 0;
    std::size_t silentSamples = 0;
    std::deque<double> sampleWindow;
    std::vector<double> yinBuffer;
    std::optional<double> smoothedMidi;
    std::optional<PitchReading> currentReading;
    std::optional<PitchReading> heldReading;
};
} // namespace apertune
