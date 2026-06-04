#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PitchMath.h"

class ApertuneAudioProcessor final : public juce::AudioProcessor
{
public:
    ApertuneAudioProcessor();
    ~ApertuneAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getState() noexcept { return state; }
    std::optional<apertune::PitchReading> getLastPitchReading() const;

private:
    template <typename SampleType>
    void processAudio(juce::AudioBuffer<SampleType>& buffer);

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState state;
    double currentSampleRate = 44100.0;
    std::optional<apertune::PitchReading> lastPitchReading;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ApertuneAudioProcessor)
};
