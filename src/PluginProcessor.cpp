#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>

namespace
{
constexpr auto muteParameterId = "mute";
constexpr auto concertAParameterId = "concertA";
constexpr auto displayUnitParameterId = "displayUnit";
constexpr auto instrumentScopeParameterId = "instrumentScope";
constexpr auto tuningPresetParameterId = "tuningPreset";
constexpr auto accidentalSpellingParameterId = "accidentalSpelling";

template <typename Enum>
Enum parameterChoice(const juce::AudioProcessorValueTreeState& state, const char* parameterId, Enum fallback) noexcept
{
    if (const auto* value = state.getRawParameterValue(parameterId))
        return static_cast<Enum>(static_cast<int>(std::round(value->load())));
    return fallback;
}
} // namespace

ApertuneAudioProcessor::ApertuneAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "ApertuneState", createParameterLayout())
{
}

ApertuneAudioProcessor::~ApertuneAudioProcessor()
{
    stopAnalysisThread();
}

juce::AudioProcessorValueTreeState::ParameterLayout ApertuneAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { muteParameterId, 1 }, "Mute", false));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { concertAParameterId, 1 },
        "Concert A",
        juce::NormalisableRange<float> {
            static_cast<float>(apertune::minConcertAHz),
            static_cast<float>(apertune::maxConcertAHz),
            static_cast<float>(apertune::concertAStepHz) },
        static_cast<float>(apertune::defaultConcertAHz)));
    parameters.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { displayUnitParameterId, 1 },
        "Display Unit",
        juce::StringArray { "Cents", "Hz" },
        static_cast<int>(apertune::DisplayUnit::cents)));
    parameters.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { instrumentScopeParameterId, 1 },
        "Instrument Scope",
        juce::StringArray { "Bass", "Guitar", "Custom" },
        static_cast<int>(apertune::InstrumentScope::guitar)));
    parameters.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { tuningPresetParameterId, 1 },
        "Tuning Preset",
        juce::StringArray {
            "Bass 4 Standard",
            "Bass 5 Low B",
            "Bass 6 Standard",
            "Guitar 6 Standard",
            "Guitar 7 Low B",
            "Guitar 8 Standard",
            "Guitar 9 Standard",
            "Custom" },
        static_cast<int>(apertune::TuningPreset::guitar6Standard)));
    parameters.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { accidentalSpellingParameterId, 1 },
        "Accidental Spelling",
        juce::StringArray { "Sharps", "Flats" },
        static_cast<int>(apertune::AccidentalSpelling::sharps)));
    return { parameters.begin(), parameters.end() };
}

void ApertuneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    stopAnalysisThread();

    currentSampleRate = sampleRate;
    pitchDetector.prepare(sampleRate);

    {
        const juce::SpinLock::ScopedLockType lock(readingLock);
        publishedReading.reset();
    }

    const auto block = std::max(0, samplesPerBlock);
    monoMix.assign(static_cast<std::size_t>(block), 0.0f);
    analysisChunk.assign(static_cast<std::size_t>(std::max(2048, block)), 0.0f);

    const auto fifoSize = std::max(16384, block * 8);
    fifoBuffer.assign(static_cast<std::size_t>(fifoSize), 0.0f);
    sampleFifo.setTotalSize(fifoSize);
    sampleFifo.reset();

    startAnalysisThread();
}

void ApertuneAudioProcessor::releaseResources()
{
    stopAnalysisThread();
}

void ApertuneAudioProcessor::startAnalysisThread()
{
    stopAnalysisThread();
    analysisShouldExit.store(false, std::memory_order_relaxed);
    analysisThread = std::thread([this] { runAnalysis(); });
}

void ApertuneAudioProcessor::stopAnalysisThread()
{
    analysisShouldExit.store(true, std::memory_order_relaxed);
    if (analysisThread.joinable())
        analysisThread.join();
}

void ApertuneAudioProcessor::runAnalysis()
{
    while (! analysisShouldExit.load(std::memory_order_relaxed))
    {
        const auto ready = sampleFifo.getNumReady();
        if (ready > 0)
        {
            if (static_cast<int>(analysisChunk.size()) < ready)
                analysisChunk.resize(static_cast<std::size_t>(ready));

            int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
            sampleFifo.prepareToRead(ready, start1, size1, start2, size2);
            if (size1 > 0)
                std::copy(fifoBuffer.begin() + start1, fifoBuffer.begin() + start1 + size1, analysisChunk.begin());
            if (size2 > 0)
                std::copy(fifoBuffer.begin() + start2, fifoBuffer.begin() + start2 + size2, analysisChunk.begin() + size1);
            sampleFifo.finishedRead(size1 + size2);

            const auto concertA = static_cast<double>(*state.getRawParameterValue(concertAParameterId));
            auto reading = pitchDetector.processSamples(
                analysisChunk.data(), static_cast<std::size_t>(size1 + size2), concertA);

            const juce::SpinLock::ScopedLockType lock(readingLock);
            publishedReading = reading;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(4));
    }
}

bool ApertuneAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainInput = layouts.getMainInputChannelSet();
    const auto mainOutput = layouts.getMainOutputChannelSet();
    return mainInput == mainOutput
        && (mainOutput == juce::AudioChannelSet::mono() || mainOutput == juce::AudioChannelSet::stereo());
}

void ApertuneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    processAudio(buffer);
}

void ApertuneAudioProcessor::processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    processAudio(buffer);
}

template <typename SampleType>
void ApertuneAudioProcessor::processAudio(juce::AudioBuffer<SampleType>& buffer)
{
    const auto channelCount = buffer.getNumChannels();
    const auto sampleCount = buffer.getNumSamples();

    // Tap the input (pre-mute) into the lock-free FIFO for the analysis thread. No detection here.
    if (channelCount > 0 && sampleCount > 0 && sampleFifo.getTotalSize() > 1)
    {
        if (static_cast<int>(monoMix.size()) < sampleCount)
            monoMix.resize(static_cast<std::size_t>(sampleCount));

        const auto gain = 1.0f / static_cast<float>(channelCount);
        for (int sample = 0; sample < sampleCount; ++sample)
        {
            float sum = 0.0f;
            for (int channel = 0; channel < channelCount; ++channel)
                sum += static_cast<float>(buffer.getReadPointer(channel)[sample]);
            monoMix[static_cast<std::size_t>(sample)] = sum * gain;
        }

        const auto toWrite = std::min(sampleCount, sampleFifo.getFreeSpace());
        if (toWrite > 0)
        {
            int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
            sampleFifo.prepareToWrite(toWrite, start1, size1, start2, size2);
            if (size1 > 0)
                std::copy(monoMix.begin(), monoMix.begin() + size1, fifoBuffer.begin() + start1);
            if (size2 > 0)
                std::copy(monoMix.begin() + size1, monoMix.begin() + size1 + size2, fifoBuffer.begin() + start2);
            sampleFifo.finishedWrite(size1 + size2);
        }
    }

    // Mute affects the OUTPUT only; the detection tap above already captured the live signal.
    if (*state.getRawParameterValue(muteParameterId) > 0.5f)
        buffer.clear();
}

juce::AudioProcessorEditor* ApertuneAudioProcessor::createEditor()
{
    return new ApertuneAudioProcessorEditor(*this);
}

bool ApertuneAudioProcessor::hasEditor() const { return true; }
const juce::String ApertuneAudioProcessor::getName() const { return JucePlugin_Name; }
bool ApertuneAudioProcessor::acceptsMidi() const { return false; }
bool ApertuneAudioProcessor::producesMidi() const { return false; }
bool ApertuneAudioProcessor::isMidiEffect() const { return false; }
double ApertuneAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int ApertuneAudioProcessor::getNumPrograms() { return 1; }
int ApertuneAudioProcessor::getCurrentProgram() { return 0; }
void ApertuneAudioProcessor::setCurrentProgram(int) {}
const juce::String ApertuneAudioProcessor::getProgramName(int) { return {}; }
void ApertuneAudioProcessor::changeProgramName(int, const juce::String&) {}

void ApertuneAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void ApertuneAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        state.replaceState(juce::ValueTree::fromXml(*xml));
}

apertune::TunerSettings ApertuneAudioProcessor::getTunerSettings() const
{
    apertune::TunerSettings settings;
    settings.displayUnit = parameterChoice(state, displayUnitParameterId, apertune::DisplayUnit::cents);
    settings.instrumentScope = parameterChoice(state, instrumentScopeParameterId, apertune::InstrumentScope::guitar);
    settings.tuningPreset = apertune::coercePresetForScope(
        parameterChoice(state, tuningPresetParameterId, apertune::TuningPreset::guitar6Standard),
        settings.instrumentScope);
    settings.accidentalSpelling = parameterChoice(state, accidentalSpellingParameterId, apertune::AccidentalSpelling::sharps);
    return settings;
}

std::optional<apertune::PitchReading> ApertuneAudioProcessor::getLastPitchReading() const
{
    const juce::SpinLock::ScopedLockType lock(readingLock);
    return publishedReading;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ApertuneAudioProcessor();
}
