#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <algorithm>
#include <memory>

namespace
{
constexpr auto muteParameterId = "mute";
constexpr auto concertAParameterId = "concertA";
} // namespace

ApertuneAudioProcessor::ApertuneAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "ApertuneState", createParameterLayout())
{
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
            0.1f },
        static_cast<float>(apertune::defaultConcertAHz)));
    return { parameters.begin(), parameters.end() };
}

void ApertuneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    pitchDetector.prepare(sampleRate);
    monoScratch.reserve(static_cast<std::size_t>(std::max(0, samplesPerBlock)));
    lastPitchReading.reset();
}

void ApertuneAudioProcessor::releaseResources() {}

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
    const auto concertA = static_cast<double>(*state.getRawParameterValue(concertAParameterId));
    const auto channelCount = buffer.getNumChannels();
    const auto sampleCount = buffer.getNumSamples();

    if (channelCount > 0 && sampleCount > 0)
    {
        monoScratch.assign(static_cast<std::size_t>(sampleCount), 0.0);
        for (int channel = 0; channel < channelCount; ++channel)
        {
            const auto* input = buffer.getReadPointer(channel);
            for (int sample = 0; sample < sampleCount; ++sample)
                monoScratch[static_cast<std::size_t>(sample)] += static_cast<double>(input[sample]);
        }

        const auto gain = 1.0 / static_cast<double>(channelCount);
        for (auto& sample : monoScratch)
            sample *= gain;

        lastPitchReading = pitchDetector.processSamples(monoScratch.data(), monoScratch.size(), concertA);
    }

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

std::optional<apertune::PitchReading> ApertuneAudioProcessor::getLastPitchReading() const
{
    return lastPitchReading;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ApertuneAudioProcessor();
}
