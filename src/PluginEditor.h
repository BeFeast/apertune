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

    void setSettingsVisible(bool shouldShow);
    void paintTunerFace(juce::Graphics& graphics, juce::Rectangle<float> panelBounds);
    void paintSettingsPanel(juce::Graphics& graphics, juce::Rectangle<float> panelBounds);

    void setDisplayUnit(int unitIndex);
    void nudgeConcertA(float deltaHz);

    ApertuneAudioProcessor& audioProcessor;

    bool showSettings { false };
    juce::Image grainImage;

    // Title-bar chrome (transparent hit targets; icons are painted).
    juce::ToggleButton muteButton;        // speaker; toggles the mute parameter
    juce::TextButton settingsButton;       // gear; opens settings
    juce::TextButton closeSettingsButton;  // close X; returns to the tuner face

    // Footer controls on the tuner face.
    juce::TextButton unitCentsButton;      // Cents segment of the unit toggle
    juce::TextButton unitHzButton;         // Hz segment of the unit toggle
    juce::TextButton refUpButton;          // reference-pitch stepper up
    juce::TextButton refDownButton;        // reference-pitch stepper down

    // Settings controls (still the interim generic panel; faithful picker is Step 3).
    juce::Slider concertASlider;
    juce::ComboBox instrumentScopeBox;
    juce::ComboBox tuningPresetBox;
    juce::ComboBox accidentalSpellingBox;

    juce::AudioProcessorValueTreeState::ButtonAttachment muteAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment concertAAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> instrumentScopeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> accidentalSpellingAttachment;

    void refreshPresetItems();
    void coercePresetForSelectedScope();
    void updatePresetParameterFromCombo();
    void syncPresetComboToParameter();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ApertuneAudioProcessorEditor)
};
