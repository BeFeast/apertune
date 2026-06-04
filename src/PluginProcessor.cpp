#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <memory>
#include <vector>

namespace
{
constexpr auto muteParameterId = "mute";
constexpr auto concertAParameterId = "concertA";

template <typename SampleType>
double estimateZeroCrossingFrequency(const juce::AudioBuffer<SampleType>& buffer, double sampleRate)
{
    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() < 2 || sampleRate <= 0.0)
        return 0.0;

    const auto* channel = buffer.getReadPointer(0);
    int crossings = 0;

    for (int i = 1; i < buffer.getNumSamples(); ++i)
    {
        if ((channel[i - 1] <= SampleType{} && channel[i] > SampleType{})
            || (channel[i - 1] >= SampleType{} && channel[i] < SampleType{}))
            ++crossings;
    }

    return (static_cast<double>(crossings) * sampleRate) / (2.0 * static_cast<double>(buffer.getNumSamples()));
}
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
        juce::NormalisableRange<float> { 430.0f, 450.0f, 0.1f },
        440.0f));
    return { parameters.begin(), parameters.end() };
}

void ApertuneAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
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
    const auto estimatedFrequency = estimateZeroCrossingFrequency(buffer, currentSampleRate);
    const auto concertA = static_cast<double>(*state.getRawParameterValue(concertAParameterId));
    lastPitchReading = apertune::analyseFrequency(estimatedFrequency, concertA);

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
