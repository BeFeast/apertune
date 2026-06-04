#include "PitchMath.h"
#include "TunerUiModel.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{
int failureCount = 0;

bool near(double actual, double expected, double tolerance)
{
    return std::abs(actual - expected) <= tolerance;
}

void check(bool condition, const std::string& label)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << label << '\n';
        ++failureCount;
    }
}

void checkNear(double actual, double expected, double tolerance, const std::string& label)
{
    if (!near(actual, expected, tolerance))
    {
        std::cerr << "FAIL: " << label
                  << " (expected " << expected
                  << " +/- " << tolerance
                  << ", got " << actual << ")\n";
        ++failureCount;
    }
}

double pitchAtCents(double midi, double cents, double concertA = apertune::defaultConcertAHz)
{
    return concertA * std::pow(2.0, (midi - 69.0 + cents / 100.0) / 12.0);
}

std::vector<double> renderSine(double frequencyHz, double seconds, double sampleRate, double amplitude = 0.2)
{
    const auto count = static_cast<std::size_t>(std::round(seconds * sampleRate));
    std::vector<double> samples(count, 0.0);
    double phase = 0.0;
    const auto phaseStep = 2.0 * std::acos(-1.0) * frequencyHz / sampleRate;
    for (auto& sample : samples)
    {
        sample = amplitude * std::sin(phase);
        phase += phaseStep;
    }
    return samples;
}

std::vector<double> renderContinuousSine(
    const std::vector<std::pair<double, double>>& frequencySeconds,
    double sampleRate,
    double amplitude = 0.2)
{
    std::vector<double> samples;
    double phase = 0.0;
    for (const auto& segment : frequencySeconds)
    {
        const auto count = static_cast<std::size_t>(std::round(segment.second * sampleRate));
        const auto phaseStep = 2.0 * std::acos(-1.0) * segment.first / sampleRate;
        for (std::size_t i = 0; i < count; ++i)
        {
            samples.push_back(amplitude * std::sin(phase));
            phase += phaseStep;
        }
    }
    return samples;
}

std::vector<double> renderHarmonicTone(double frequencyHz, double seconds, double sampleRate)
{
    const auto count = static_cast<std::size_t>(std::round(seconds * sampleRate));
    std::vector<double> samples(count, 0.0);
    for (std::size_t i = 0; i < count; ++i)
    {
        const auto t = static_cast<double>(i) / sampleRate;
        samples[i] = 0.22 * std::sin(2.0 * std::acos(-1.0) * frequencyHz * t)
            + 0.17 * std::sin(2.0 * std::acos(-1.0) * frequencyHz * 2.0 * t)
            + 0.12 * std::sin(2.0 * std::acos(-1.0) * frequencyHz * 3.0 * t)
            + 0.07 * std::sin(2.0 * std::acos(-1.0) * frequencyHz * 4.0 * t);
    }
    return samples;
}

std::optional<apertune::PitchReading> feed(apertune::RealTimePitchDetector& detector,
    const std::vector<double>& samples,
    double concertA = apertune::defaultConcertAHz)
{
    return detector.processSamples(samples.data(), samples.size(), concertA);
}

void testCoreConversion()
{
    checkNear(apertune::midiNoteFromFrequency(440.0), 69.0, 1e-9,
        "midiNoteFromFrequency: A4 = 440 Hz");
    checkNear(apertune::frequencyFromMidiNote(69.0), 440.0, 1e-9,
        "frequencyFromMidiNote: MIDI 69 = 440 Hz");

    checkNear(apertune::midiNoteFromFrequency(432.0, 432.0), 69.0, 1e-9,
        "midiNoteFromFrequency respects concert A 432");
    checkNear(apertune::midiNoteFromFrequency(448.0, 448.0), 69.0, 1e-9,
        "midiNoteFromFrequency respects concert A 448");
    checkNear(apertune::frequencyFromMidiNote(40.0), 82.40689, 1e-3,
        "frequencyFromMidiNote: MIDI 40 = E2");
    checkNear(apertune::frequencyFromMidiNote(88.0), 1318.51023, 1e-3,
        "frequencyFromMidiNote: MIDI 88 = E6");
}

void testNoteAndOctaveTables()
{
    check(apertune::noteNameForMidiNote(60) == "C", "noteNameForMidiNote: 60 = C");
    check(apertune::noteNameForMidiNote(69) == "A", "noteNameForMidiNote: 69 = A");
    check(apertune::noteNameForMidiNote(70) == "A#", "noteNameForMidiNote: 70 = A#");
    check(apertune::noteNameForMidiNote(23) == "B", "noteNameForMidiNote: 23 = B");
    check(apertune::noteNameForMidiNote(88) == "E", "noteNameForMidiNote: 88 = E");

    check(apertune::octaveForMidiNote(0) == -1, "octaveForMidiNote: 0 = C-1");
    check(apertune::octaveForMidiNote(12) == 0, "octaveForMidiNote: 12 = C0");
    check(apertune::octaveForMidiNote(23) == 0, "octaveForMidiNote: 23 = B0");
    check(apertune::octaveForMidiNote(40) == 2, "octaveForMidiNote: 40 = E2");
    check(apertune::octaveForMidiNote(60) == 4, "octaveForMidiNote: 60 = C4");
    check(apertune::octaveForMidiNote(69) == 4, "octaveForMidiNote: 69 = A4");
    check(apertune::octaveForMidiNote(88) == 6, "octaveForMidiNote: 88 = E6");
}

void testGuitarBassRangeExact()
{
    struct Case
    {
        double frequency;
        int midi;
        int octave;
        std::string_view note;
    };

    const Case cases[] {
        { apertune::frequencyFromMidiNote(23.0), 23, 0, "B" },
        { apertune::frequencyFromMidiNote(28.0), 28, 1, "E" },
        { apertune::frequencyFromMidiNote(40.0), 40, 2, "E" },
        { apertune::frequencyFromMidiNote(45.0), 45, 2, "A" },
        { apertune::frequencyFromMidiNote(50.0), 50, 3, "D" },
        { apertune::frequencyFromMidiNote(64.0), 64, 4, "E" },
        { apertune::frequencyFromMidiNote(69.0), 69, 4, "A" },
        { apertune::frequencyFromMidiNote(88.0), 88, 6, "E" },
    };

    for (const auto& c : cases)
    {
        const auto reading = apertune::analyseFrequency(c.frequency);
        if (!reading)
        {
            check(false, std::string("analyseFrequency: expected reading at MIDI ") + std::to_string(c.midi));
            continue;
        }
        check(reading->midiNote == c.midi,
            std::string("analyseFrequency: midi for MIDI ") + std::to_string(c.midi));
        check(reading->octave == c.octave,
            std::string("analyseFrequency: octave for MIDI ") + std::to_string(c.midi));
        check(reading->noteName == c.note,
            std::string("analyseFrequency: note name for MIDI ") + std::to_string(c.midi));
        checkNear(reading->cents, 0.0, 1e-6,
            std::string("analyseFrequency: cents at MIDI ") + std::to_string(c.midi));
        check(reading->inLock,
            std::string("analyseFrequency: inLock for exact MIDI ") + std::to_string(c.midi));
    }
}

void testSharpAndFlatOffsets()
{
    const auto sharp = apertune::analyseFrequency(446.0);
    check(sharp.has_value(), "analyseFrequency: 446 Hz returns reading");
    if (sharp)
    {
        check(sharp->midiNote == 69, "446 Hz still maps to A4");
        check(sharp->octave == 4, "446 Hz octave is 4");
        check(sharp->cents > 0.0, "446 Hz reads sharp");
        checkNear(sharp->cents, 23.45, 0.05, "446 Hz cents ~= 23.45");
        check(!sharp->inLock, "446 Hz is outside lock band");
    }

    const auto flat = apertune::analyseFrequency(434.0);
    check(flat.has_value(), "analyseFrequency: 434 Hz returns reading");
    if (flat)
    {
        check(flat->midiNote == 69, "434 Hz still maps to A4");
        check(flat->cents < 0.0, "434 Hz reads flat");
        checkNear(flat->cents, -23.78, 0.05, "434 Hz cents ~= -23.78");
        check(!flat->inLock, "434 Hz is outside lock band");
    }

    const auto sharpHalfStep = apertune::analyseFrequency(
        apertune::frequencyFromMidiNote(70.0));
    check(sharpHalfStep.has_value(), "analyseFrequency: A#4 returns reading");
    if (sharpHalfStep)
    {
        check(sharpHalfStep->midiNote == 70, "A#4 frequency maps to MIDI 70");
        check(sharpHalfStep->noteName == "A#", "A#4 note name");
        checkNear(sharpHalfStep->cents, 0.0, 1e-6, "A#4 cents == 0");
    }
}

void testReferencePitchExtremes()
{
    check(apertune::isSupportedConcertA(432.0), "432 Hz is supported");
    check(apertune::isSupportedConcertA(440.0), "440 Hz is supported");
    check(apertune::isSupportedConcertA(448.0), "448 Hz is supported");
    check(!apertune::isSupportedConcertA(431.9), "431.9 Hz is unsupported");
    check(!apertune::isSupportedConcertA(448.1), "448.1 Hz is unsupported");
    check(!apertune::isSupportedConcertA(0.0), "0 Hz is unsupported");
    check(!apertune::isSupportedConcertA(std::numeric_limits<double>::quiet_NaN()),
        "NaN concert A is unsupported");
    check(!apertune::isSupportedConcertA(std::numeric_limits<double>::infinity()),
        "infinite concert A is unsupported");

    const auto a432 = apertune::analyseFrequency(432.0, 432.0);
    check(a432.has_value(), "432 Hz at A=432 returns reading");
    if (a432)
    {
        check(a432->midiNote == 69, "432 Hz at A=432 -> A4");
        check(a432->noteName == "A", "432 Hz at A=432 note name");
        checkNear(a432->cents, 0.0, 1e-6, "432 Hz at A=432 cents == 0");
        check(a432->inLock, "432 Hz at A=432 inLock");
    }

    const auto a448 = apertune::analyseFrequency(448.0, 448.0);
    check(a448.has_value(), "448 Hz at A=448 returns reading");
    if (a448)
    {
        check(a448->midiNote == 69, "448 Hz at A=448 -> A4");
        checkNear(a448->cents, 0.0, 1e-6, "448 Hz at A=448 cents == 0");
        check(a448->inLock, "448 Hz at A=448 inLock");
    }

    const auto e2At432 = apertune::analyseFrequency(
        apertune::frequencyFromMidiNote(40.0, 432.0), 432.0);
    check(e2At432.has_value() && e2At432->midiNote == 40,
        "E2 at A=432 maps to MIDI 40");

    const auto e6At448 = apertune::analyseFrequency(
        apertune::frequencyFromMidiNote(88.0, 448.0), 448.0);
    check(e6At448.has_value() && e6At448->midiNote == 88 && e6At448->octave == 6,
        "E6 at A=448 maps to MIDI 88 / octave 6");

    check(!apertune::analyseFrequency(440.0, 431.9).has_value(),
        "analyseFrequency rejects concert A below 432");
    check(!apertune::analyseFrequency(440.0, 448.1).has_value(),
        "analyseFrequency rejects concert A above 448");
}

void testBoundaryFiftyCents()
{
    const auto justUnderSharp = apertune::analyseFrequency(pitchAtCents(69.0, 49.5));
    check(justUnderSharp.has_value(), "+49.5c reading exists");
    if (justUnderSharp)
    {
        check(justUnderSharp->midiNote == 69, "+49.5c stays on A4");
        checkNear(justUnderSharp->cents, 49.5, 0.01, "+49.5c reports ~49.5");
        check(!justUnderSharp->inLock, "+49.5c is outside lock band");
    }

    const auto justOverSharp = apertune::analyseFrequency(pitchAtCents(69.0, 50.5));
    check(justOverSharp.has_value(), "+50.5c reading exists");
    if (justOverSharp)
    {
        check(justOverSharp->midiNote == 70, "+50.5c snaps up to A#4");
        check(justOverSharp->noteName == "A#", "+50.5c note name");
        checkNear(justOverSharp->cents, -49.5, 0.01, "+50.5c reports ~-49.5 against A#4");
    }

    const auto justUnderFlat = apertune::analyseFrequency(pitchAtCents(69.0, -49.5));
    check(justUnderFlat.has_value(), "-49.5c reading exists");
    if (justUnderFlat)
    {
        check(justUnderFlat->midiNote == 69, "-49.5c stays on A4");
        checkNear(justUnderFlat->cents, -49.5, 0.01, "-49.5c reports ~-49.5");
    }

    const auto justOverFlat = apertune::analyseFrequency(pitchAtCents(69.0, -50.5));
    check(justOverFlat.has_value(), "-50.5c reading exists");
    if (justOverFlat)
    {
        check(justOverFlat->midiNote == 68, "-50.5c snaps down to G#4");
        check(justOverFlat->noteName == "G#", "-50.5c note name");
        checkNear(justOverFlat->cents, 49.5, 0.01, "-50.5c reports ~+49.5 against G#4");
    }
}

void testLockState()
{
    check(apertune::isInLock(0.0), "isInLock: 0c");
    check(apertune::isInLock(3.0), "isInLock: +3c boundary");
    check(apertune::isInLock(-3.0), "isInLock: -3c boundary");
    check(!apertune::isInLock(3.0001), "isInLock: just outside +3c");
    check(!apertune::isInLock(-3.0001), "isInLock: just outside -3c");
    check(!apertune::isInLock(std::numeric_limits<double>::quiet_NaN()),
        "isInLock: NaN is not locked");

    const auto inside = apertune::analyseFrequency(pitchAtCents(69.0, 2.5));
    check(inside && inside->inLock, "Reading at +2.5c is locked");

    const auto justInside = apertune::analyseFrequency(pitchAtCents(69.0, 2.9));
    check(justInside && justInside->inLock, "Reading at +2.9c is locked");

    const auto outside = apertune::analyseFrequency(pitchAtCents(69.0, 4.0));
    check(outside && !outside->inLock, "Reading at +4c is not locked");
}

void testInvalidInputs()
{
    check(!apertune::analyseFrequency(0.0).has_value(),
        "analyseFrequency rejects 0 Hz");
    check(!apertune::analyseFrequency(-1.0).has_value(),
        "analyseFrequency rejects negative Hz");
    check(!apertune::analyseFrequency(std::numeric_limits<double>::quiet_NaN()).has_value(),
        "analyseFrequency rejects NaN");
    check(!apertune::analyseFrequency(std::numeric_limits<double>::infinity()).has_value(),
        "analyseFrequency rejects +inf");
    check(!apertune::analyseFrequency(-std::numeric_limits<double>::infinity()).has_value(),
        "analyseFrequency rejects -inf");
    check(!apertune::analyseFrequency(440.0, 0.0).has_value(),
        "analyseFrequency rejects concert A = 0");
    check(!apertune::analyseFrequency(440.0,
              std::numeric_limits<double>::quiet_NaN()).has_value(),
        "analyseFrequency rejects concert A = NaN");

    check(!apertune::isValidFrequency(0.0), "isValidFrequency: 0");
    check(!apertune::isValidFrequency(-10.0), "isValidFrequency: negative");
    check(!apertune::isValidFrequency(std::numeric_limits<double>::quiet_NaN()),
        "isValidFrequency: NaN");
    check(apertune::isValidFrequency(82.4068892282), "isValidFrequency: E2");
}

void testRealTimeDetectorTracksCleanNotes()
{
    apertune::RealTimePitchDetector detector;
    detector.prepare(44100.0);

    const auto e2 = feed(detector, renderSine(apertune::frequencyFromMidiNote(40.0), 0.18, 44100.0));
    check(e2.has_value(), "RealTimePitchDetector: clean E2 returns reading");
    if (e2)
    {
        check(e2->midiNote == 40, "RealTimePitchDetector: clean E2 maps to MIDI 40");
        checkNear(e2->cents, 0.0, 2.0, "RealTimePitchDetector: clean E2 cents near zero");
    }

    detector.reset();
    const auto b0 = feed(detector, renderSine(apertune::frequencyFromMidiNote(23.0), 0.24, 44100.0));
    check(b0.has_value(), "RealTimePitchDetector: clean B0 returns reading");
    if (b0)
    {
        check(b0->midiNote == 23, "RealTimePitchDetector: clean B0 maps to MIDI 23");
        checkNear(b0->cents, 0.0, 2.5, "RealTimePitchDetector: clean B0 cents near zero");
    }
}

void testRealTimeDetectorRejectsOvertoneWhenFundamentalPresent()
{
    apertune::RealTimePitchDetector detector;
    detector.prepare(44100.0);

    const auto e2Frequency = apertune::frequencyFromMidiNote(40.0);
    const auto reading = feed(detector, renderHarmonicTone(e2Frequency, 0.20, 44100.0));
    check(reading.has_value(), "RealTimePitchDetector: harmonic-rich E2 returns reading");
    if (reading)
    {
        check(reading->midiNote == 40, "RealTimePitchDetector: harmonic-rich E2 keeps fundamental");
        check(reading->midiNote != 52, "RealTimePitchDetector: harmonic-rich E2 is not tracked as octave overtone");
    }
}

void testRealTimeDetectorSilenceRelease()
{
    apertune::PitchDetectorConfig config;
    config.releaseHoldSeconds = 0.12;
    config.releaseFadeSeconds = 0.12;
    apertune::RealTimePitchDetector detector(config);
    detector.prepare(44100.0);

    const auto active = feed(detector, renderSine(440.0, 0.16, 44100.0));
    check(active.has_value(), "RealTimePitchDetector: active tone before release");

    const auto shortSilence = feed(detector, std::vector<double>(static_cast<std::size_t>(0.05 * 44100.0), 0.0));
    check(shortSilence.has_value(), "RealTimePitchDetector: short release hold keeps reading");
    if (shortSilence)
        checkNear(shortSilence->visibility, 1.0, 1e-9, "RealTimePitchDetector: release hold remains visible");

    const auto fadingSilence = feed(detector, std::vector<double>(static_cast<std::size_t>(0.12 * 44100.0), 0.0));
    check(fadingSilence.has_value(), "RealTimePitchDetector: release fade keeps reading briefly");
    if (fadingSilence)
        check(fadingSilence->visibility > 0.0 && fadingSilence->visibility < 1.0,
            "RealTimePitchDetector: release fade lowers visibility");

    const auto longSilence = feed(detector, std::vector<double>(static_cast<std::size_t>(0.20 * 44100.0), 0.0));
    check(!longSilence.has_value(), "RealTimePitchDetector: long silence clears reading");
}

void testRealTimeDetectorSmoothing()
{
    apertune::RealTimePitchDetector jitterDetector;
    jitterDetector.prepare(44100.0);

    auto reading = feed(jitterDetector,
        renderContinuousSine({
                                 { 440.0, 0.14 },
                                 { pitchAtCents(69.0, 4.0), 0.03 },
                                 { pitchAtCents(69.0, -4.0), 0.03 },
                             },
                             44100.0));
    check(reading.has_value(), "RealTimePitchDetector: jitter reading exists");
    if (reading)
        check(std::abs(reading->cents) < 4.0, "RealTimePitchDetector: smoothing damps alternating jitter");

    apertune::RealTimePitchDetector turnDetector;
    turnDetector.prepare(44100.0);
    reading = feed(turnDetector,
        renderContinuousSine({
                                 { 440.0, 0.14 },
                                 { pitchAtCents(69.0, 28.0), 0.10 },
                             },
                             44100.0));
    check(reading.has_value(), "RealTimePitchDetector: peg turn reading exists");
    if (reading)
        check(reading->cents > 18.0, "RealTimePitchDetector: deliberate peg turn is not over-smoothed");
}

void testTunerUiDeterministicFrames()
{
    const auto noSignal = apertune::deterministicTunerUiFrame(apertune::TunerUiState::noSignal);
    check(noSignal.state == apertune::TunerUiState::noSignal, "TunerUiModel: no-signal fixture state");
    check(!noSignal.hasSignal, "TunerUiModel: no-signal fixture has no live signal");
    checkNear(noSignal.ballPosition, 0.5, 1e-9, "TunerUiModel: no-signal ball is centered placeholder");
    check(!noSignal.showChevron, "TunerUiModel: no-signal hides chevron");

    const auto flat = apertune::deterministicTunerUiFrame(apertune::TunerUiState::flat);
    check(flat.state == apertune::TunerUiState::flat, "TunerUiModel: flat fixture state");
    check(flat.direction == apertune::TuneDirection::tuneUp, "TunerUiModel: flat means tune up");
    check(flat.showChevron, "TunerUiModel: flat shows chevron");
    check(flat.ballPosition < 0.5, "TunerUiModel: flat ball sits left of center");
    check(!flat.inLock && !flat.panelGlow, "TunerUiModel: flat is not locked");

    const auto near = apertune::deterministicTunerUiFrame(apertune::TunerUiState::near);
    check(near.state == apertune::TunerUiState::near, "TunerUiModel: near fixture state");
    check(near.showChevron, "TunerUiModel: near shows direction chevron");
    check(near.ballPosition > 0.0 && near.ballPosition < 0.5,
        "TunerUiModel: near-flat ball remains left of center");

    const auto lock = apertune::deterministicTunerUiFrame(apertune::TunerUiState::inTuneLock);
    check(lock.state == apertune::TunerUiState::inTuneLock, "TunerUiModel: lock fixture state");
    check(lock.inLock, "TunerUiModel: lock fixture is in lock");
    check(lock.panelGlow, "TunerUiModel: lock fixture enables panel glow");
    check(!lock.showChevron, "TunerUiModel: lock fixture hides chevron");
    checkNear(lock.ballPosition, 0.5, 1e-9, "TunerUiModel: lock fixture centers ball");
    checkNear(lock.reticleDiameter, 78.0, 1e-9, "TunerUiModel: lock fixture tightens reticle");
    check(lock.activeStringIndex >= 0
            && lock.tunedStrings[static_cast<std::size_t>(lock.activeStringIndex)],
        "TunerUiModel: lock fixture marks active string tuned");

    const auto sharp = apertune::deterministicTunerUiFrame(apertune::TunerUiState::sharp);
    check(sharp.state == apertune::TunerUiState::sharp, "TunerUiModel: sharp fixture state");
    check(sharp.direction == apertune::TuneDirection::tuneDown, "TunerUiModel: sharp means tune down");
    check(sharp.showChevron, "TunerUiModel: sharp shows chevron");
    check(sharp.ballPosition > 0.5, "TunerUiModel: sharp ball sits right of center");

    const auto muted = apertune::deterministicTunerUiFrame(apertune::TunerUiState::muted);
    check(muted.state == apertune::TunerUiState::muted, "TunerUiModel: muted fixture state");
    check(muted.muted, "TunerUiModel: muted fixture has muted flag");
    check(!muted.hasSignal, "TunerUiModel: muted fixture suppresses live signal");
    check(!muted.panelGlow, "TunerUiModel: muted fixture has no lock glow");
}
} // namespace

int main()
{
    testCoreConversion();
    testNoteAndOctaveTables();
    testGuitarBassRangeExact();
    testSharpAndFlatOffsets();
    testReferencePitchExtremes();
    testBoundaryFiftyCents();
    testLockState();
    testInvalidInputs();
    testRealTimeDetectorTracksCleanNotes();
    testRealTimeDetectorRejectsOvertoneWhenFundamentalPresent();
    testRealTimeDetectorSilenceRelease();
    testRealTimeDetectorSmoothing();
    testTunerUiDeterministicFrames();

    if (failureCount > 0)
    {
        std::cerr << failureCount << " pitch math assertion(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All pitch math assertions passed\n";
    return EXIT_SUCCESS;
}
