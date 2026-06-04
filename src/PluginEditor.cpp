#include "PluginEditor.h"

#include <cmath>

namespace
{
juce::Colour background() { return juce::Colour::fromRGB(18, 20, 22); }
juce::Colour panel() { return juce::Colour::fromRGB(31, 34, 36); }
juce::Colour ivory() { return juce::Colour::fromRGB(232, 225, 210); }
juce::Colour brass() { return juce::Colour::fromRGB(202, 156, 74); }
juce::Colour signalRed() { return juce::Colour::fromRGB(208, 64, 58); }
} // namespace

ApertuneAudioProcessorEditor::ApertuneAudioProcessorEditor(ApertuneAudioProcessor& owner)
    : AudioProcessorEditor(&owner),
      audioProcessor(owner),
      muteAttachment(audioProcessor.getState(), "mute", muteButton),
      concertAAttachment(audioProcessor.getState(), "concertA", concertASlider)
{
    setSize(720, 420);

    muteButton.setButtonText("Mute");
    addAndMakeVisible(muteButton);

    concertASlider.setSliderStyle(juce::Slider::LinearHorizontal);
    concertASlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 22);
    concertASlider.setTextValueSuffix(" Hz");
    addAndMakeVisible(concertASlider);
}

void ApertuneAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(background());

    auto bounds = getLocalBounds().toFloat().reduced(28.0f);
    graphics.setColour(panel());
    graphics.fillRoundedRectangle(bounds, 8.0f);

    graphics.setColour(ivory());
    graphics.setFont(juce::FontOptions(20.0f, juce::Font::bold));
    graphics.drawText("APERTUNE", bounds.removeFromTop(38.0f).toNearestInt(), juce::Justification::centred);

    auto stringRow = bounds.removeFromTop(34.0f);
    graphics.setFont(juce::FontOptions(14.0f));
    graphics.drawText("B   E   A   D   G   B   E", stringRow.toNearestInt(), juce::Justification::centred);

    auto scale = bounds.removeFromTop(46.0f).reduced(42.0f, 0.0f);
    graphics.setColour(ivory().withAlpha(0.7f));
    graphics.drawText("-50", scale.removeFromLeft(80.0f).toNearestInt(), juce::Justification::centredLeft);
    graphics.drawText("0", scale.withSizeKeepingCentre(80.0f, scale.getHeight()).toNearestInt(), juce::Justification::centred);
    graphics.drawText("+50", scale.removeFromRight(80.0f).toNearestInt(), juce::Justification::centredRight);

    auto track = bounds.removeFromTop(88.0f).reduced(36.0f, 0.0f);
    const auto centerX = track.getCentreX();
    graphics.setColour(ivory().withAlpha(0.25f));
    for (int i = 0; i <= 40; ++i)
    {
        const auto x = track.getX() + track.getWidth() * static_cast<float>(i) / 40.0f;
        const auto tickHeight = (i % 10 == 0) ? 34.0f : 18.0f;
        graphics.drawVerticalLine(static_cast<int>(std::round(x)), track.getCentreY() - tickHeight, track.getCentreY() + tickHeight);
    }

    graphics.setColour(brass());
    graphics.drawLine(centerX, track.getY(), centerX, track.getBottom(), 2.0f);
    graphics.drawEllipse(centerX - 34.0f, track.getCentreY() - 34.0f, 68.0f, 68.0f, 2.0f);

    auto reading = audioProcessor.getLastPitchReading();
    const auto cents = reading ? juce::jlimit(-50.0, 50.0, reading->cents) : 0.0;
    const auto ballX = centerX + static_cast<float>(cents / 50.0) * (track.getWidth() * 0.5f);
    graphics.setColour(signalRed());
    graphics.fillEllipse(ballX - 9.0f, track.getCentreY() - 9.0f, 18.0f, 18.0f);

    auto noteRow = bounds.removeFromTop(78.0f);
    graphics.setColour(ivory());
    graphics.setFont(juce::FontOptions(44.0f, juce::Font::bold));
    graphics.drawText(reading ? juce::String(reading->noteName.data()) : juce::String("--"),
        noteRow.toNearestInt(),
        juce::Justification::centred);

    graphics.setFont(juce::FontOptions(13.0f));
    graphics.setColour(ivory().withAlpha(0.72f));
    graphics.drawText("Guitar / Bass      Rangefinder focus lock      CLAP / VST3 / AU",
        bounds.removeFromBottom(34.0f).toNearestInt(),
        juce::Justification::centred);
}

void ApertuneAudioProcessorEditor::resized()
{
    auto controls = getLocalBounds().reduced(40).removeFromBottom(34);
    muteButton.setBounds(controls.removeFromLeft(110));
    controls.removeFromLeft(24);
    concertASlider.setBounds(controls.removeFromLeft(260));
}
