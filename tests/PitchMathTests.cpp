#include "PitchMath.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

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

    if (failureCount > 0)
    {
        std::cerr << failureCount << " pitch math assertion(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All pitch math assertions passed\n";
    return EXIT_SUCCESS;
}
