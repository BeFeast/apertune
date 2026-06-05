#include "PluginEditor.h"

#include "TunerUiModel.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
constexpr auto muteParameterId = "mute";
constexpr auto concertAParameterId = "concertA";
constexpr auto displayUnitParameterId = "displayUnit";
constexpr auto instrumentScopeParameterId = "instrumentScope";
constexpr auto tuningPresetParameterId = "tuningPreset";
constexpr auto accidentalSpellingParameterId = "accidentalSpelling";

juce::Colour background() { return juce::Colour::fromRGB(13, 15, 17); }
juce::Colour panelTop() { return juce::Colour::fromRGB(29, 32, 36); }
juce::Colour panelBottom() { return juce::Colour::fromRGB(16, 18, 21); }
juce::Colour border() { return juce::Colour::fromRGB(52, 56, 62); }
juce::Colour textPrimary() { return juce::Colour::fromRGB(236, 238, 241); }
juce::Colour textSecondary() { return juce::Colour::fromRGB(194, 198, 204); }
juce::Colour textMuted() { return juce::Colour::fromRGB(108, 113, 119); }
juce::Colour tickDim() { return juce::Colour::fromRGB(52, 56, 62); }
juce::Colour tickMajor() { return juce::Colour::fromRGB(86, 92, 100); }
juce::Colour lockGreen() { return juce::Colour::fromRGB(39, 230, 117); }
juce::Colour nearAmber() { return juce::Colour::fromRGB(246, 149, 39); }
juce::Colour offRed() { return juce::Colour::fromRGB(229, 72, 61); }

juce::Colour tuneColour(const apertune::TunerUiFrame& frame)
{
    if (frame.muted || !frame.hasSignal)
        return textMuted();
    if (frame.inLock)
        return lockGreen();
    return frame.state == apertune::TunerUiState::near ? nearAmber() : offRed();
}

juce::String centsText(const apertune::TunerUiFrame& frame)
{
    if (!frame.hasSignal)
        return "--.-";
    const auto sign = frame.cents > 0.05 ? "+" : "";
    return sign + juce::String(frame.cents, 1);
}

void drawReticle(juce::Graphics& graphics, juce::Rectangle<float> bounds, juce::Colour colour)
{
    const auto corner = bounds.getWidth() * 0.24f;
    const auto stroke = juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    juce::Path path;

    path.startNewSubPath(bounds.getX(), bounds.getY() + corner);
    path.lineTo(bounds.getX(), bounds.getY());
    path.lineTo(bounds.getX() + corner, bounds.getY());

    path.startNewSubPath(bounds.getRight() - corner, bounds.getY());
    path.lineTo(bounds.getRight(), bounds.getY());
    path.lineTo(bounds.getRight(), bounds.getY() + corner);

    path.startNewSubPath(bounds.getX(), bounds.getBottom() - corner);
    path.lineTo(bounds.getX(), bounds.getBottom());
    path.lineTo(bounds.getX() + corner, bounds.getBottom());

    path.startNewSubPath(bounds.getRight() - corner, bounds.getBottom());
    path.lineTo(bounds.getRight(), bounds.getBottom());
    path.lineTo(bounds.getRight(), bounds.getBottom() - corner);

    graphics.setColour(colour);
    graphics.strokePath(path, stroke);
}

void drawChevron(juce::Graphics& graphics, juce::Rectangle<float> bounds, apertune::TuneDirection direction, juce::Colour colour)
{
    juce::Path path;
    if (direction == apertune::TuneDirection::tuneUp)
    {
        path.startNewSubPath(bounds.getRight(), bounds.getY());
        path.lineTo(bounds.getX(), bounds.getCentreY());
        path.lineTo(bounds.getRight(), bounds.getBottom());
    }
    else if (direction == apertune::TuneDirection::tuneDown)
    {
        path.startNewSubPath(bounds.getX(), bounds.getY());
        path.lineTo(bounds.getRight(), bounds.getCentreY());
        path.lineTo(bounds.getX(), bounds.getBottom());
    }
    else
    {
        return;
    }

    graphics.setColour(colour);
    graphics.strokePath(path, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void styleComboBox(juce::ComboBox& comboBox)
{
    comboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGB(24, 27, 31));
    comboBox.setColour(juce::ComboBox::outlineColourId, border());
    comboBox.setColour(juce::ComboBox::textColourId, textPrimary());
    comboBox.setColour(juce::ComboBox::arrowColourId, textSecondary());
    comboBox.setColour(juce::PopupMenu::backgroundColourId, juce::Colour::fromRGB(24, 27, 31));
    comboBox.setColour(juce::PopupMenu::textColourId, textPrimary());
    comboBox.setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour::fromRGB(46, 52, 58));
    comboBox.setColour(juce::PopupMenu::highlightedTextColourId, textPrimary());
}
} // namespace

ApertuneAudioProcessorEditor::ApertuneAudioProcessorEditor(ApertuneAudioProcessor& owner)
    : AudioProcessorEditor(&owner),
      audioProcessor(owner),
      muteAttachment(audioProcessor.getState(), muteParameterId, muteButton),
      concertAAttachment(audioProcessor.getState(), concertAParameterId, concertASlider)
{
    setSize(760, 500);

    muteButton.setButtonText("Mute");
    muteButton.setColour(juce::ToggleButton::textColourId, textSecondary());
    muteButton.setColour(juce::ToggleButton::tickColourId, lockGreen());
    muteButton.setColour(juce::ToggleButton::tickDisabledColourId, textMuted());
    addAndMakeVisible(muteButton);

    concertASlider.setSliderStyle(juce::Slider::LinearHorizontal);
    concertASlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 22);
    concertASlider.setTextValueSuffix(" Hz");
    concertASlider.setColour(juce::Slider::trackColourId, lockGreen().withAlpha(0.55f));
    concertASlider.setColour(juce::Slider::backgroundColourId, juce::Colour::fromRGB(38, 42, 47));
    concertASlider.setColour(juce::Slider::thumbColourId, textPrimary());
    concertASlider.setColour(juce::Slider::textBoxTextColourId, textPrimary());
    concertASlider.setColour(juce::Slider::textBoxOutlineColourId, border());
    addAndMakeVisible(concertASlider);

    displayUnitBox.addItem(juce::String(apertune::displayUnitName(apertune::DisplayUnit::cents).data()), 1);
    displayUnitBox.addItem(juce::String(apertune::displayUnitName(apertune::DisplayUnit::hertz).data()), 2);
    instrumentScopeBox.addItem(juce::String(apertune::instrumentScopeName(apertune::InstrumentScope::bass).data()), 1);
    instrumentScopeBox.addItem(juce::String(apertune::instrumentScopeName(apertune::InstrumentScope::guitar).data()), 2);
    instrumentScopeBox.addItem(juce::String(apertune::instrumentScopeName(apertune::InstrumentScope::custom).data()), 3);
    accidentalSpellingBox.addItem(juce::String(apertune::accidentalSpellingName(apertune::AccidentalSpelling::sharps).data()), 1);
    accidentalSpellingBox.addItem(juce::String(apertune::accidentalSpellingName(apertune::AccidentalSpelling::flats).data()), 2);

    for (auto* comboBox : { &displayUnitBox, &instrumentScopeBox, &tuningPresetBox, &accidentalSpellingBox })
    {
        styleComboBox(*comboBox);
        addAndMakeVisible(*comboBox);
    }

    displayUnitAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getState(), displayUnitParameterId, displayUnitBox);
    instrumentScopeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getState(), instrumentScopeParameterId, instrumentScopeBox);
    accidentalSpellingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getState(), accidentalSpellingParameterId, accidentalSpellingBox);

    audioProcessor.getState().addParameterListener(tuningPresetParameterId, this);
    audioProcessor.getState().addParameterListener(instrumentScopeParameterId, this);

    instrumentScopeBox.onChange = [this]
    {
        refreshPresetItems();
        coercePresetForSelectedScope();
        repaint();
    };
    tuningPresetBox.onChange = [this]
    {
        updatePresetParameterFromCombo();
        repaint();
    };
    accidentalSpellingBox.onChange = [this] { repaint(); };

    refreshPresetItems();
    syncPresetComboToParameter();

    startTimerHz(30);
}

ApertuneAudioProcessorEditor::~ApertuneAudioProcessorEditor()
{
    stopTimer();
    audioProcessor.getState().removeParameterListener(tuningPresetParameterId, this);
    audioProcessor.getState().removeParameterListener(instrumentScopeParameterId, this);
}

void ApertuneAudioProcessorEditor::timerCallback()
{
    repaint();
}

void ApertuneAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(background());

    const auto muted = *audioProcessor.getState().getRawParameterValue(muteParameterId) > 0.5f;
    const auto concertA = static_cast<double>(*audioProcessor.getState().getRawParameterValue(concertAParameterId));
    const auto settings = audioProcessor.getTunerSettings();
    const auto frame = apertune::makeTunerUiFrame(audioProcessor.getLastPitchReading(), muted, concertA, settings);
    const auto accent = tuneColour(frame);

    auto panelBounds = getLocalBounds().toFloat().reduced(26.0f);
    juce::ColourGradient panelGradient(panelTop(), panelBounds.getX(), panelBounds.getY(), panelBottom(), panelBounds.getX(), panelBounds.getBottom(), false);
    graphics.setGradientFill(panelGradient);
    graphics.fillRoundedRectangle(panelBounds, 8.0f);

    if (frame.panelGlow)
    {
        graphics.setColour(lockGreen().withAlpha(0.14f));
        graphics.fillRoundedRectangle(panelBounds.reduced(2.0f), 8.0f);
        graphics.setColour(lockGreen().withAlpha(0.55f));
        graphics.drawRoundedRectangle(panelBounds.reduced(1.0f), 8.0f, 1.8f);
    }
    else
    {
        graphics.setColour(border().withAlpha(0.85f));
        graphics.drawRoundedRectangle(panelBounds, 8.0f, 1.0f);
    }

    auto content = panelBounds.reduced(28.0f, 0.0f);
    auto titleBar = content.removeFromTop(50.0f);
    graphics.setColour(juce::Colour::fromRGB(19, 21, 24).withAlpha(0.62f));
    graphics.fillRect(panelBounds.withHeight(50.0f));
    graphics.setColour(border().withAlpha(0.7f));
    graphics.drawLine(panelBounds.getX(), titleBar.getBottom(), panelBounds.getRight(), titleBar.getBottom(), 1.0f);

    graphics.setColour(textPrimary());
    graphics.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    graphics.drawText("Tuner", titleBar.removeFromLeft(150.0f).toNearestInt(), juce::Justification::centredLeft);
    graphics.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    graphics.setColour(textMuted());
    graphics.drawText("APERTUNE", titleBar.withSizeKeepingCentre(160.0f, titleBar.getHeight()).toNearestInt(), juce::Justification::centred);
    graphics.setColour(textMuted());
    graphics.drawLine(panelBounds.getRight() - 42.0f, panelBounds.getY() + 18.0f, panelBounds.getRight() - 30.0f, panelBounds.getY() + 30.0f, 1.6f);
    graphics.drawLine(panelBounds.getRight() - 30.0f, panelBounds.getY() + 18.0f, panelBounds.getRight() - 42.0f, panelBounds.getY() + 30.0f, 1.6f);

    content.removeFromTop(24.0f);

    auto strings = content.removeFromTop(34.0f);
    const auto dotSize = 30.0f;
    const auto gap = 10.0f;
    const auto totalWidth = dotSize * static_cast<float>(frame.stringLabels.size()) + gap * static_cast<float>(frame.stringLabels.size() - 1);
    auto x = strings.getCentreX() - totalWidth * 0.5f;
    for (auto index = 0; index < static_cast<int>(frame.stringLabels.size()); ++index)
    {
        const auto dot = juce::Rectangle<float>(x, strings.getCentreY() - dotSize * 0.5f, dotSize, dotSize);
        const auto tuned = frame.tunedStrings[static_cast<std::size_t>(index)];
        const auto active = frame.activeStringIndex == index;
        graphics.setColour(tuned ? lockGreen().withAlpha(0.16f) : juce::Colours::transparentBlack);
        graphics.fillEllipse(dot);
        graphics.setColour(active ? accent : (tuned ? lockGreen().withAlpha(0.7f) : border()));
        graphics.drawEllipse(dot, active ? 2.2f : 1.4f);
        if (active)
            graphics.drawEllipse(dot.expanded(4.0f), 1.2f);
        graphics.setColour(tuned || active ? textPrimary() : textMuted());
        graphics.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        graphics.drawText(frame.stringLabels[static_cast<std::size_t>(index)], dot.toNearestInt(), juce::Justification::centred);
        x += dotSize + gap;
    }

    content.removeFromTop(20.0f);

    auto scale = content.removeFromTop(42.0f).reduced(36.0f, 0.0f);
    graphics.setFont(juce::FontOptions(14.0f));
    graphics.setColour(textMuted());
    graphics.drawText("-50", scale.removeFromLeft(70.0f).toNearestInt(), juce::Justification::centredLeft);
    graphics.drawText("+50", scale.removeFromRight(70.0f).toNearestInt(), juce::Justification::centredRight);
    graphics.setFont(juce::FontOptions(22.0f));
    graphics.setColour(frame.inLock ? lockGreen() : textSecondary());
    graphics.drawText(centsText(frame), scale.withSizeKeepingCentre(110.0f, scale.getHeight()).toNearestInt(), juce::Justification::centred);
    if (frame.showChevron)
    {
        const auto chevronX = frame.direction == apertune::TuneDirection::tuneUp
            ? scale.getCentreX() - 88.0f
            : scale.getCentreX() + 68.0f;
        drawChevron(graphics, { chevronX, scale.getY() + 9.0f, 20.0f, 20.0f }, frame.direction, accent);
    }

    auto track = content.removeFromTop(150.0f).reduced(28.0f, 0.0f);
    const auto centerX = track.getCentreX();
    const auto centerY = track.getCentreY();
    for (int i = 0; i <= 40; ++i)
    {
        const auto tickX = track.getX() + track.getWidth() * static_cast<float>(i) / 40.0f;
        const auto isCenter = i == 20;
        const auto isMajor = i % 10 == 0;
        const auto height = isCenter ? 56.0f : (isMajor ? 20.0f : 12.0f);
        graphics.setColour(isCenter ? textPrimary().withAlpha(0.42f) : (isMajor ? tickMajor() : tickDim()));
        graphics.drawLine(tickX, centerY - height * 0.5f, tickX, centerY + height * 0.5f, isCenter ? 2.0f : 1.5f);
    }

    const auto reticleSize = static_cast<float>(frame.reticleDiameter);
    drawReticle(graphics,
        juce::Rectangle<float>(centerX - reticleSize * 0.5f, centerY - reticleSize * 0.5f, reticleSize, reticleSize),
        frame.inLock ? lockGreen() : juce::Colour::fromRGB(74, 81, 88));

    const auto ballDiameter = 60.0f;
    const auto ballX = track.getX() + track.getWidth() * static_cast<float>(frame.ballPosition);
    const auto ball = juce::Rectangle<float>(ballX - ballDiameter * 0.5f, centerY - ballDiameter * 0.5f, ballDiameter, ballDiameter);
    graphics.setColour(accent.withAlpha(static_cast<float>(frame.visibility) * 0.18f));
    graphics.fillEllipse(ball.expanded(8.0f));
    graphics.setColour(accent.withAlpha(static_cast<float>(frame.visibility)));
    if (frame.hasSignal || frame.muted)
        graphics.fillEllipse(ball);
    else
        graphics.drawEllipse(ball, 2.0f);
    graphics.setColour(textPrimary().withAlpha(frame.inLock ? 0.72f : 0.34f));
    graphics.drawEllipse(ball.reduced(9.0f), frame.inLock ? 2.0f : 1.2f);

    auto noteRow = content.removeFromTop(92.0f).reduced(36.0f, 0.0f);
    graphics.setFont(juce::FontOptions(19.0f));
    graphics.setColour(textMuted());
    graphics.drawText(frame.hasSignal ? juce::String(frame.previousNoteName.data()) : juce::String("--"),
        noteRow.removeFromLeft(110.0f).toNearestInt(),
        juce::Justification::centredLeft);
    graphics.drawText(frame.hasSignal ? juce::String(frame.nextNoteName.data()) : juce::String("--"),
        noteRow.removeFromRight(110.0f).toNearestInt(),
        juce::Justification::centredRight);

    const auto note = frame.hasSignal || frame.muted ? juce::String(frame.noteName.data()) : juce::String("--");
    juce::Font noteFont(juce::FontOptions(82.0f, juce::Font::bold));
    graphics.setColour(frame.inLock ? lockGreen() : (frame.hasSignal ? textPrimary() : textMuted()));
    graphics.setFont(noteFont);
    auto noteCenterBox = noteRow.withSizeKeepingCentre(170.0f, noteRow.getHeight());
    graphics.drawText(note, noteCenterBox.toNearestInt(), juce::Justification::centred);
    if (frame.hasSignal || frame.muted)
    {
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(noteFont, note, 0.0f, 0.0f);
        const auto glyphWidth = glyphs.getBoundingBox(0, -1, true).getWidth();
        const auto glyphRight = noteCenterBox.getCentreX() + glyphWidth * 0.5f;
        juce::Rectangle<float> octaveBox(glyphRight + 4.0f, noteRow.getCentreY() - 1.0f, 40.0f, 32.0f);
        graphics.setFont(juce::FontOptions(24.0f, juce::Font::bold));
        graphics.setColour(textMuted());
        graphics.drawText(juce::String(frame.octave), octaveBox.toNearestInt(), juce::Justification::centredLeft);
    }

    auto footer = panelBounds.reduced(28.0f, 0.0f).removeFromBottom(54.0f);
    graphics.setColour(border().withAlpha(0.7f));
    graphics.drawLine(panelBounds.getX(), footer.getY(), panelBounds.getRight(), footer.getY(), 1.0f);

    graphics.setFont(juce::FontOptions(15.0f));
    graphics.setColour(textSecondary());
    const auto footerText = settings.displayUnit == apertune::DisplayUnit::hertz
        ? ((frame.hasSignal || frame.muted) ? juce::String(frame.frequencyHz, 1) + " Hz" : "--.- Hz")
        : ((frame.hasSignal || frame.muted) ? centsText(frame) + " cents" : "--.- cents");
    graphics.drawText(footerText,
        footer.withSizeKeepingCentre(150.0f, footer.getHeight()).toNearestInt(),
        juce::Justification::centred);
}

void ApertuneAudioProcessorEditor::resized()
{
    auto panelBounds = getLocalBounds().reduced(26);
    auto titleBar = panelBounds.removeFromTop(50).reduced(28, 10);
    muteButton.setBounds(titleBar.removeFromRight(96));

    auto footer = getLocalBounds().reduced(26).removeFromBottom(54).reduced(28, 11);
    concertASlider.setBounds(footer.removeFromRight(200));
    footer.removeFromRight(10);
    accidentalSpellingBox.setBounds(footer.removeFromRight(80));
    footer.removeFromRight(10);
    tuningPresetBox.setBounds(footer.removeFromRight(150));
    footer.removeFromRight(10);
    instrumentScopeBox.setBounds(footer.removeFromRight(88));
    displayUnitBox.setBounds(footer.removeFromLeft(80));
}

void ApertuneAudioProcessorEditor::refreshPresetItems()
{
    const auto selectedScope = static_cast<apertune::InstrumentScope>(
        std::max(0, instrumentScopeBox.getSelectedItemIndex()));
    const auto selectedId = tuningPresetBox.getSelectedId();
    tuningPresetBox.clear(juce::dontSendNotification);

    for (const auto preset : apertune::presetsForScope(selectedScope))
    {
        tuningPresetBox.addItem(
            juce::String(apertune::tuningPresetName(preset).data()),
            static_cast<int>(preset) + 1);
    }

    if (selectedId > 0 && tuningPresetBox.indexOfItemId(selectedId) >= 0)
        tuningPresetBox.setSelectedId(selectedId, juce::dontSendNotification);
}

void ApertuneAudioProcessorEditor::coercePresetForSelectedScope()
{
    const auto selectedScope = static_cast<apertune::InstrumentScope>(
        std::max(0, instrumentScopeBox.getSelectedItemIndex()));
    const auto selectedPreset = static_cast<apertune::TuningPreset>(
        std::max(0, tuningPresetBox.getSelectedId() - 1));
    const auto coercedPreset = apertune::coercePresetForScope(selectedPreset, selectedScope);
    tuningPresetBox.setSelectedId(static_cast<int>(coercedPreset) + 1, juce::sendNotificationSync);
}

void ApertuneAudioProcessorEditor::updatePresetParameterFromCombo()
{
    if (tuningPresetBox.getSelectedId() <= 0)
        return;

    if (auto* parameter = audioProcessor.getState().getParameter(tuningPresetParameterId))
    {
        const auto presetIndex = static_cast<float>(tuningPresetBox.getSelectedId() - 1);
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(presetIndex));
        parameter->endChangeGesture();
    }
}

void ApertuneAudioProcessorEditor::syncPresetComboToParameter()
{
    const auto settings = audioProcessor.getTunerSettings();
    refreshPresetItems();
    tuningPresetBox.setSelectedId(static_cast<int>(settings.tuningPreset) + 1, juce::dontSendNotification);
}

void ApertuneAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float)
{
    if (parameterID == tuningPresetParameterId || parameterID == instrumentScopeParameterId)
    {
        juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer<ApertuneAudioProcessorEditor>(this)]
        {
            if (safeThis != nullptr)
            {
                safeThis->syncPresetComboToParameter();
                safeThis->repaint();
            }
        });
    }
}
