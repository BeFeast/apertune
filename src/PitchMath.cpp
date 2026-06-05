#include "PitchMath.h"

#include <algorithm>
#include <cmath>

namespace apertune
{
namespace
{
double clamp(double value, double low, double high)
{
    return std::max(low, std::min(high, value));
}
} // namespace

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
    reading.visibility = 1.0;
    reading.noteName = noteNameForMidiNote(nearestMidi);
    reading.inLock = isInLock(cents);
    return reading;
}

RealTimePitchDetector::RealTimePitchDetector(PitchDetectorConfig detectorConfig)
    : config(detectorConfig)
{
    prepare(sampleRate);
}

void RealTimePitchDetector::prepare(double newSampleRate)
{
    sampleRate = std::isfinite(newSampleRate) && newSampleRate > 0.0 ? newSampleRate : 44100.0;

    const auto minFrequency = std::max(1.0, config.minFrequencyHz);
    const auto maxTau = static_cast<std::size_t>(std::ceil(sampleRate / minFrequency));
    hopSize = std::max<std::size_t>(1,
        static_cast<std::size_t>(std::round(sampleRate * config.updateIntervalSeconds)));
    windowSize = std::max<std::size_t>(2048, maxTau + hopSize);
    yinBuffer.assign(maxTau + 1, 0.0);

    reset();
}

void RealTimePitchDetector::reset()
{
    samplesSinceAnalysis = 0;
    silentSamples = 0;
    sampleWindow.clear();
    smoothedMidi.reset();
    currentReading.reset();
    heldReading.reset();
}

void RealTimePitchDetector::pushSample(double sample)
{
    sampleWindow.push_back(std::isfinite(sample) ? sample : 0.0);
    while (sampleWindow.size() > windowSize)
        sampleWindow.pop_front();
}

void RealTimePitchDetector::analysePending(double concertAHz)
{
    if (!isSupportedConcertA(concertAHz))
    {
        currentReading.reset();
        heldReading.reset();
        smoothedMidi.reset();
        return;
    }

    const auto signalPresent = calculateRecentRms() >= config.silenceRmsThreshold;
    if (sampleWindow.size() < windowSize / 2 || !signalPresent)
    {
        updateRelease(signalPresent);
        return;
    }

    if (const auto frequency = estimateFrequency())
    {
        currentReading = smoothReading(*frequency, concertAHz);
        heldReading = currentReading;
        silentSamples = 0;
        return;
    }

    updateRelease(signalPresent);
}

std::optional<double> RealTimePitchDetector::estimateFrequency()
{
    if (sampleWindow.size() < windowSize / 2 || sampleRate <= 0.0)
        return std::nullopt;

    const auto minTau = std::max<std::size_t>(2,
        static_cast<std::size_t>(std::floor(sampleRate / config.maxFrequencyHz)));
    const auto maxTau = std::min<std::size_t>(yinBuffer.size() - 1,
        static_cast<std::size_t>(std::ceil(sampleRate / config.minFrequencyHz)));
    if (minTau >= maxTau || sampleWindow.size() <= maxTau)
        return std::nullopt;

    const auto compareCount = sampleWindow.size() - maxTau;
    std::fill(yinBuffer.begin(), yinBuffer.end(), 0.0);

    for (std::size_t tau = 1; tau <= maxTau; ++tau)
    {
        double sum = 0.0;
        for (std::size_t i = 0; i < compareCount; ++i)
        {
            const auto delta = sampleWindow[i] - sampleWindow[i + tau];
            sum += delta * delta;
        }
        yinBuffer[tau] = sum;
    }

    yinBuffer[0] = 1.0;
    double runningSum = 0.0;
    for (std::size_t tau = 1; tau <= maxTau; ++tau)
    {
        runningSum += yinBuffer[tau];
        yinBuffer[tau] = runningSum > 0.0
            ? yinBuffer[tau] * static_cast<double>(tau) / runningSum
            : 1.0;
    }

    std::size_t tauEstimate = 0;
    for (std::size_t tau = minTau; tau <= maxTau; ++tau)
    {
        if (yinBuffer[tau] < config.yinThreshold)
        {
            while (tau + 1 <= maxTau && yinBuffer[tau + 1] < yinBuffer[tau])
                ++tau;
            tauEstimate = tau;
            break;
        }
    }

    if (tauEstimate == 0)
    {
        tauEstimate = minTau;
        for (std::size_t tau = minTau + 1; tau <= maxTau; ++tau)
        {
            if (yinBuffer[tau] < yinBuffer[tauEstimate])
                tauEstimate = tau;
        }

        if (yinBuffer[tauEstimate] > config.yinThreshold * 1.75)
            return std::nullopt;
    }

    auto refinedTau = static_cast<double>(tauEstimate);
    if (tauEstimate > 1 && tauEstimate + 1 <= maxTau)
    {
        const auto left = yinBuffer[tauEstimate - 1];
        const auto center = yinBuffer[tauEstimate];
        const auto right = yinBuffer[tauEstimate + 1];
        const auto denominator = left - 2.0 * center + right;
        if (std::abs(denominator) > 1e-12)
            refinedTau += 0.5 * (left - right) / denominator;
    }

    if (refinedTau <= 0.0)
        return std::nullopt;

    const auto frequency = sampleRate / refinedTau;
    if (frequency < config.minFrequencyHz || frequency > config.maxFrequencyHz)
        return std::nullopt;

    return frequency;
}

double RealTimePitchDetector::calculateRecentRms() const
{
    if (sampleWindow.empty())
        return 0.0;

    const auto count = std::min(sampleWindow.size(), hopSize);
    const auto first = sampleWindow.size() - count;
    double sum = 0.0;
    for (std::size_t i = first; i < sampleWindow.size(); ++i)
    {
        const auto sample = sampleWindow[i];
        sum += sample * sample;
    }
    return std::sqrt(sum / static_cast<double>(count));
}

std::optional<PitchReading> RealTimePitchDetector::smoothReading(double frequencyHz, double concertAHz)
{
    if (!isValidFrequency(frequencyHz))
        return std::nullopt;

    const auto rawMidi = midiNoteFromFrequency(frequencyHz, concertAHz);
    if (!smoothedMidi)
    {
        smoothedMidi = rawMidi;
    }
    else
    {
        const auto centsDelta = std::abs(rawMidi - *smoothedMidi) * 100.0;
        const auto timeConstant = centsDelta >= config.turnThresholdCents
            ? config.turnTimeConstantSeconds
            : config.jitterTimeConstantSeconds;
        const auto alpha = clamp(config.updateIntervalSeconds / std::max(config.updateIntervalSeconds, timeConstant),
            0.0,
            1.0);
        *smoothedMidi += (rawMidi - *smoothedMidi) * alpha;
    }

    return analyseFrequency(frequencyFromMidiNote(*smoothedMidi, concertAHz), concertAHz);
}

void RealTimePitchDetector::updateRelease(bool signalPresent)
{
    if (signalPresent)
        silentSamples = 0;
    else
        silentSamples += hopSize;

    const auto heldSamples = static_cast<std::size_t>(std::round(config.releaseHoldSeconds * sampleRate));
    const auto fadeSamples = static_cast<std::size_t>(std::round(config.releaseFadeSeconds * sampleRate));
    if (heldReading && silentSamples <= heldSamples)
    {
        currentReading = heldReading;
        currentReading->visibility = 1.0;
        return;
    }

    if (heldReading && fadeSamples > 0 && silentSamples <= heldSamples + fadeSamples)
    {
        currentReading = heldReading;
        const auto fadeProgress = static_cast<double>(silentSamples - heldSamples) / static_cast<double>(fadeSamples);
        currentReading->visibility = clamp(1.0 - fadeProgress, 0.0, 1.0);
        return;
    }

    currentReading.reset();
    heldReading.reset();
    smoothedMidi.reset();
}
} // namespace apertune
