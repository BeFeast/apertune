#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

class ApertuneAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ApertuneAudioProcessorEditor(ApertuneAudioProcessor& processor);
    ~ApertuneAudioProcessorEditor() override = default;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    ApertuneAudioProcessor& audioProcessor;
    juce::ToggleButton muteButton;
    juce::Slider concertASlider;
    juce::AudioProcessorValueTreeState::ButtonAttachment muteAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment concertAAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ApertuneAudioProcessorEditor)
};
