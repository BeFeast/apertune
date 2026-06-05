#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PitchMath.h"
#include "TunerUiModel.h"

#include <atomic>
#include <optional>
#include <thread>
#include <vector>

class ApertuneAudioProcessor final : public juce::AudioProcessor
{
public:
    ApertuneAudioProcessor();
    ~ApertuneAudioProcessor() override;

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
    apertune::TunerSettings getTunerSettings() const;
    std::vector<int> getCustomTuning() const;
    void setCustomTuning(const std::vector<int>& midiNotes);
    std::optional<apertune::PitchReading> getLastPitchReading() const;

private:
    template <typename SampleType>
    void processAudio(juce::AudioBuffer<SampleType>& buffer);

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void startAnalysisThread();
    void stopAnalysisThread();
    void runAnalysis();

    juce::AudioProcessorValueTreeState state;
    double currentSampleRate = 44100.0;

    // Pitch detection runs OFF the audio thread (it is O(n*m) YIN) so processBlock stays real-time
    // safe. The audio thread only mono-mixes and pushes into a lock-free FIFO.
    apertune::RealTimePitchDetector pitchDetector;
    juce::AbstractFifo sampleFifo { 1 };
    std::vector<float> fifoBuffer;     // backing store for sampleFifo
    std::vector<float> monoMix;        // audio-thread mono mixdown scratch (pre-sized)
    std::vector<float> analysisChunk;  // analysis-thread drain scratch

    // Published reading: analysis thread writes, UI thread reads. Audio thread never touches it.
    mutable juce::SpinLock readingLock;
    std::optional<apertune::PitchReading> publishedReading;

    std::thread analysisThread;
    std::atomic<bool> analysisShouldExit { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ApertuneAudioProcessor)
};
