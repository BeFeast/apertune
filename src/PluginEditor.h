#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

#include <memory>
#include <utility>
#include <vector>

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
    void mouseDown(const juce::MouseEvent& event) override;

private:
    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void setSettingsVisible(bool shouldShow);
    void paintTunerFace(juce::Graphics& graphics, juce::Rectangle<float> panelBounds);
    void paintSettingsPanel(juce::Graphics& graphics, juce::Rectangle<float> panelBounds);

    void setInstrumentScope(int scopeIndex);
    void setTuningPreset(int presetIndex);
    void setAccidental(int spellingIndex);

    ApertuneAudioProcessor& audioProcessor;

    bool showSettings { false };
    juce::Image grainImage;

    // Title-bar chrome (transparent hit targets; icons are painted).
    juce::ToggleButton muteButton;
    juce::TextButton settingsButton;
    juce::TextButton closeSettingsButton;

    // Settings reference slider (APVTS-attached). The instrument/tuning/accidental
    // controls are painted and dispatched through mouseDown against these hit lists.
    juce::Slider concertASlider;
    std::vector<std::pair<juce::Rectangle<int>, int>> instrumentHits;
    std::vector<std::pair<juce::Rectangle<int>, int>> tuningHits;
    std::vector<std::pair<juce::Rectangle<int>, int>> accidentalHits;

    juce::AudioProcessorValueTreeState::ButtonAttachment muteAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment concertAAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ApertuneAudioProcessorEditor)
};
