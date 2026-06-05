#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

#include <memory>

class ApertuneAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::Timer,
      private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit ApertuneAudioProcessorEditor(ApertuneAudioProcessor& processor);
    ~ApertuneAudioProcessorEditor() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    ApertuneAudioProcessor& audioProcessor;
    juce::ToggleButton muteButton;
    juce::Slider concertASlider;
    juce::ComboBox displayUnitBox;
    juce::ComboBox instrumentScopeBox;
    juce::ComboBox tuningPresetBox;
    juce::ComboBox accidentalSpellingBox;
    juce::AudioProcessorValueTreeState::ButtonAttachment muteAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment concertAAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> displayUnitAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> instrumentScopeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> accidentalSpellingAttachment;

    void refreshPresetItems();
    void coercePresetForSelectedScope();
    void updatePresetParameterFromCombo();
    void syncPresetComboToParameter();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ApertuneAudioProcessorEditor)
};
