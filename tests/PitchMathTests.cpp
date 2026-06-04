#include "PitchMath.h"

#include <cmath>
#include <iostream>

namespace
{
bool near(double actual, double expected, double tolerance)
{
    return std::abs(actual - expected) <= tolerance;
}

int fail(const char* message)
{
    std::cerr << message << '\n';
    return 1;
}
} // namespace

int main()
{
    if (!near(apertune::midiNoteFromFrequency(440.0), 69.0, 0.000001))
        return fail("A4 should map to MIDI note 69");

    if (!near(apertune::frequencyFromMidiNote(69.0), 440.0, 0.000001))
        return fail("MIDI note 69 should map to 440 Hz");

    const auto e2 = apertune::analyseFrequency(82.406889);
    if (!e2 || e2->noteName != "E" || e2->midiNote != 40 || !near(e2->cents, 0.0, 0.01))
        return fail("Guitar low E should analyse as E2");

    const auto sharpA = apertune::analyseFrequency(446.0);
    if (!sharpA || sharpA->midiNote != 69 || sharpA->cents <= 0.0)
        return fail("446 Hz should read sharp of A4");

    if (apertune::analyseFrequency(0.0).has_value())
        return fail("Invalid frequencies should not produce readings");

    return 0;
}
