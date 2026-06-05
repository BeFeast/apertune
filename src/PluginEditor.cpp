#include "PluginEditor.h"

#include "TunerUiModel.h"
#include "UiFonts.h"

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

// ---- Direction A tokens -------------------------------------------------
juce::Colour bg() { return juce::Colour::fromRGB(12, 13, 15); }
juce::Colour panelTop() { return juce::Colour::fromHSL(222.0f / 360.0f, 0.09f, 0.040f, 1.0f); }
juce::Colour panelBottom() { return juce::Colour::fromHSL(224.0f / 360.0f, 0.11f, 0.020f, 1.0f); }
juce::Colour textPrimary() { return juce::Colour::fromRGB(236, 238, 241); }
juce::Colour textSecondary() { return juce::Colour::fromRGB(194, 198, 204); }
juce::Colour textMuted() { return juce::Colour::fromRGB(108, 113, 119); }
juce::Colour iconColour() { return juce::Colour::fromRGB(139, 144, 152); }
juce::Colour stringIdle() { return juce::Colour::fromRGB(91, 97, 106); }
juce::Colour stringBorder() { return juce::Colour::fromRGB(42, 46, 52); }
juce::Colour tickDim() { return juce::Colour::fromRGB(52, 56, 62); }
juce::Colour tickMajor() { return juce::Colour::fromRGB(86, 92, 100); }
juce::Colour octaveCol() { return juce::Colour::fromRGB(125, 130, 138); }
juce::Colour rowName() { return juce::Colour::fromRGB(232, 234, 237); }
juce::Colour radioIdle() { return juce::Colour::fromRGB(74, 79, 86); }
juce::Colour grn() { return juce::Colour::fromHSL(145.0f / 360.0f, 0.88f, 0.56f, 1.0f); }

juce::Colour tuneColour(const apertune::TunerUiFrame& frame)
{
    if (frame.muted || !frame.hasSignal)
        return textMuted();
    const auto a = std::abs(static_cast<float>(frame.cents));
    if (a <= 3.0f)
        return juce::Colour::fromHSL(145.0f / 360.0f, 0.80f, 0.52f, 1.0f);
    const auto t = juce::jmin((a - 3.0f) / 16.0f, 1.0f);
    const auto hue = 36.0f - t * 30.0f;
    return juce::Colour::fromHSL(hue / 360.0f, 0.92f, 0.56f, 1.0f);
}

juce::String centsText(const apertune::TunerUiFrame& frame)
{
    if (!frame.hasSignal)
        return "--.-";
    const auto sign = frame.cents > 0.05 ? "+" : "";
    return sign + juce::String(frame.cents, 1);
}

juce::Font grotesk(float h, int weight)
{
    auto tf = weight >= 700 ? apertune::ui::groteskBold()
            : weight >= 600 ? apertune::ui::groteskSemi()
                            : apertune::ui::groteskMedium();
    return apertune::ui::font(tf, h);
}
juce::Font mono(float h, bool bold = false)
{
    return apertune::ui::font(bold ? apertune::ui::monoBold() : apertune::ui::monoRegular(), h);
}

juce::Image makeGrain()
{
    const int n = 160;
    juce::Image img(juce::Image::ARGB, n, n, true);
    juce::Random rng(20260605);
    juce::Image::BitmapData data(img, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x)
            data.setPixelColour(x, y, juce::Colours::white.withAlpha(static_cast<float>(rng.nextInt(256)) / 255.0f));
    return img;
}

void drawReticle(juce::Graphics& g, juce::Rectangle<float> b, juce::Colour colour)
{
    const auto corner = 19.0f;
    const auto stroke = juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    juce::Path p;
    p.startNewSubPath(b.getX(), b.getY() + corner); p.lineTo(b.getX(), b.getY()); p.lineTo(b.getX() + corner, b.getY());
    p.startNewSubPath(b.getRight() - corner, b.getY()); p.lineTo(b.getRight(), b.getY()); p.lineTo(b.getRight(), b.getY() + corner);
    p.startNewSubPath(b.getX(), b.getBottom() - corner); p.lineTo(b.getX(), b.getBottom()); p.lineTo(b.getX() + corner, b.getBottom());
    p.startNewSubPath(b.getRight() - corner, b.getBottom()); p.lineTo(b.getRight(), b.getBottom()); p.lineTo(b.getRight(), b.getBottom() - corner);
    g.setColour(colour);
    g.strokePath(p, stroke);
}

void drawChevron(juce::Graphics& g, float cx, float cy, bool right, juce::Colour colour)
{
    const auto s = 9.0f;
    juce::Path p;
    if (right) { p.startNewSubPath(cx - s * 0.5f, cy - s); p.lineTo(cx + s * 0.5f, cy); p.lineTo(cx - s * 0.5f, cy + s); }
    else       { p.startNewSubPath(cx + s * 0.5f, cy - s); p.lineTo(cx - s * 0.5f, cy); p.lineTo(cx + s * 0.5f, cy + s); }
    g.setColour(colour);
    g.strokePath(p, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void drawSpeaker(juce::Graphics& g, juce::Rectangle<float> a, juce::Colour colour, bool muted)
{
    // Faithful to the design IconSpeaker SVG on a 24x24 viewbox.
    auto vx = [&](float v) { return a.getX() + v / 24.0f * a.getWidth(); };
    auto vy = [&](float v) { return a.getY() + v / 24.0f * a.getHeight(); };
    const auto unit = a.getWidth() / 24.0f;

    juce::Path body;
    body.startNewSubPath(vx(11.0f), vy(5.0f));
    body.lineTo(vx(6.0f), vy(9.0f));
    body.lineTo(vx(3.0f), vy(9.0f));
    body.lineTo(vx(3.0f), vy(15.0f));
    body.lineTo(vx(6.0f), vy(15.0f));
    body.lineTo(vx(11.0f), vy(19.0f));
    body.closeSubPath();
    g.setColour(colour);
    g.fillPath(body);

    if (muted)
    {
        g.drawLine(vx(14.0f), vy(7.0f), vx(21.0f), vy(17.0f), 1.8f * unit);
    }
    else
    {
        const auto centre = juce::Point<float>(vx(12.0f), vy(12.0f));
        const auto rightward = juce::MathConstants<float>::halfPi; // 3 o'clock (clockwise from top)
        for (auto rv : { 5.0f, 8.5f })
        {
            juce::Path wave;
            wave.addCentredArc(centre.x, centre.y, rv * unit, rv * unit, 0.0f,
                               rightward - 0.78f, rightward + 0.78f, true);
            g.strokePath(wave, juce::PathStrokeType(1.6f * unit, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
}

void drawGearIcon(juce::Graphics& g, juce::Rectangle<float> a, juce::Colour colour)
{
    const auto c = a.getCentre();
    const auto r = juce::jmin(a.getWidth(), a.getHeight()) * 0.5f;
    const auto body = r * 0.5f, teeth = r * 0.92f, hole = r * 0.2f;
    g.setColour(colour);
    for (int i = 0; i < 8; ++i)
    {
        const auto ang = juce::MathConstants<float>::twoPi * static_cast<float>(i) / 8.0f;
        const auto ca = std::cos(ang), sa = std::sin(ang);
        g.drawLine(c.x + ca * body, c.y + sa * body, c.x + ca * teeth, c.y + sa * teeth, 1.8f);
    }
    g.drawEllipse(c.x - body, c.y - body, body * 2.0f, body * 2.0f, 1.6f);
    g.fillEllipse(c.x - hole, c.y - hole, hole * 2.0f, hole * 2.0f);
}

void drawClose(juce::Graphics& g, juce::Rectangle<float> a, juce::Colour colour)
{
    g.setColour(colour);
    g.drawLine(a.getX() + 0.22f * a.getWidth(), a.getY() + 0.22f * a.getHeight(),
               a.getRight() - 0.22f * a.getWidth(), a.getBottom() - 0.22f * a.getHeight(), 1.8f);
    g.drawLine(a.getRight() - 0.22f * a.getWidth(), a.getY() + 0.22f * a.getHeight(),
               a.getX() + 0.22f * a.getWidth(), a.getBottom() - 0.22f * a.getHeight(), 1.8f);
}

void drawStepArrow(juce::Graphics& g, juce::Rectangle<float> a, bool up, juce::Colour colour)
{
    juce::Path p;
    const auto m = a.reduced(a.getWidth() * 0.28f, a.getHeight() * 0.30f);
    if (up) { p.startNewSubPath(m.getX(), m.getBottom()); p.lineTo(m.getCentreX(), m.getY()); p.lineTo(m.getRight(), m.getBottom()); }
    else    { p.startNewSubPath(m.getX(), m.getY()); p.lineTo(m.getCentreX(), m.getBottom()); p.lineTo(m.getRight(), m.getY()); }
    g.setColour(colour);
    g.fillPath(p);
}

void makeTransparent(juce::TextButton& b)
{
    b.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    b.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
}

// A horizontal segmented control. Fills `hits` with (cellRect, index); highlights `active`.
void drawSegmented(juce::Graphics& g, juce::Rectangle<float> box, const juce::StringArray& labels, int active,
                   std::vector<std::pair<juce::Rectangle<int>, int>>& hits)
{
    g.setColour(juce::Colours::white.withAlpha(0.13f));
    g.drawRoundedRectangle(box, 8.0f, 1.0f);
    const auto cellW = box.getWidth() / static_cast<float>(labels.size());
    for (int i = 0; i < labels.size(); ++i)
    {
        auto cell = box.withWidth(cellW).translated(static_cast<float>(i) * cellW, 0.0f);
        const auto on = i == active;
        if (on)
        {
            g.setColour(grn().withAlpha(0.15f));
            g.fillRoundedRectangle(cell.reduced(1.5f), 6.5f);
        }
        if (i > 0)
        {
            g.setColour(juce::Colours::white.withAlpha(0.08f));
            g.drawLine(cell.getX(), box.getY() + 5.0f, cell.getX(), box.getBottom() - 5.0f, 1.0f);
        }
        g.setColour(on ? grn() : iconColour());
        g.setFont(grotesk(12.0f, 500));
        g.drawText(labels[i], cell.toNearestInt(), juce::Justification::centred);
        hits.push_back({ cell.toNearestInt(), i });
    }
}
} // namespace

ApertuneAudioProcessorEditor::ApertuneAudioProcessorEditor(ApertuneAudioProcessor& owner)
    : AudioProcessorEditor(&owner),
      audioProcessor(owner),
      muteAttachment(audioProcessor.getState(), muteParameterId, muteButton),
      concertAAttachment(audioProcessor.getState(), concertAParameterId, concertASlider)
{
    setSize(600, 460);
    grainImage = makeGrain();

    muteButton.setColour(juce::ToggleButton::textColourId, juce::Colours::transparentBlack);
    muteButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::transparentBlack);
    muteButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::transparentBlack);
    muteButton.setTooltip("Mute output while tuning");
    addAndMakeVisible(muteButton);

    makeTransparent(settingsButton);
    settingsButton.setTooltip("Settings");
    settingsButton.onClick = [this] { setSettingsVisible(true); };
    addAndMakeVisible(settingsButton);

    makeTransparent(closeSettingsButton);
    closeSettingsButton.setTooltip("Back to tuner");
    closeSettingsButton.onClick = [this] { setSettingsVisible(false); };
    addAndMakeVisible(closeSettingsButton);

    makeTransparent(unitCentsButton);
    makeTransparent(unitHzButton);
    unitCentsButton.onClick = [this] { setDisplayUnit(0); };
    unitHzButton.onClick = [this] { setDisplayUnit(1); };
    addAndMakeVisible(unitCentsButton);
    addAndMakeVisible(unitHzButton);

    makeTransparent(refUpButton);
    makeTransparent(refDownButton);
    refUpButton.onClick = [this] { nudgeConcertA(0.1f); };
    refDownButton.onClick = [this] { nudgeConcertA(-0.1f); };
    addAndMakeVisible(refUpButton);
    addAndMakeVisible(refDownButton);

    concertASlider.setSliderStyle(juce::Slider::LinearHorizontal);
    concertASlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    concertASlider.setColour(juce::Slider::trackColourId, grn());
    concertASlider.setColour(juce::Slider::backgroundColourId, juce::Colour::fromRGB(38, 41, 46));
    concertASlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible(concertASlider);

    audioProcessor.getState().addParameterListener(tuningPresetParameterId, this);
    audioProcessor.getState().addParameterListener(instrumentScopeParameterId, this);

    setSettingsVisible(false);
    startTimerHz(30);
}

ApertuneAudioProcessorEditor::~ApertuneAudioProcessorEditor()
{
    stopTimer();
    audioProcessor.getState().removeParameterListener(tuningPresetParameterId, this);
    audioProcessor.getState().removeParameterListener(instrumentScopeParameterId, this);
}

void ApertuneAudioProcessorEditor::timerCallback() { repaint(); }

void ApertuneAudioProcessorEditor::setDisplayUnit(int unitIndex)
{
    if (auto* p = audioProcessor.getState().getParameter(displayUnitParameterId))
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(unitIndex)));
    repaint();
}

void ApertuneAudioProcessorEditor::nudgeConcertA(float deltaHz)
{
    if (auto* p = audioProcessor.getState().getParameter(concertAParameterId))
    {
        const auto cur = audioProcessor.getState().getRawParameterValue(concertAParameterId)->load();
        const auto nv = juce::jlimit(432.0f, 448.0f, cur + deltaHz);
        p->setValueNotifyingHost(p->convertTo0to1(nv));
    }
    repaint();
}

void ApertuneAudioProcessorEditor::setInstrumentScope(int scopeIndex)
{
    auto& state = audioProcessor.getState();
    if (auto* p = state.getParameter(instrumentScopeParameterId))
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(scopeIndex)));
    const auto scope = static_cast<apertune::InstrumentScope>(scopeIndex);
    const auto coerced = apertune::coercePresetForScope(audioProcessor.getTunerSettings().tuningPreset, scope);
    if (auto* tp = state.getParameter(tuningPresetParameterId))
        tp->setValueNotifyingHost(tp->convertTo0to1(static_cast<float>(static_cast<int>(coerced))));
    repaint();
}

void ApertuneAudioProcessorEditor::setTuningPreset(int presetIndex)
{
    if (auto* p = audioProcessor.getState().getParameter(tuningPresetParameterId))
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(presetIndex)));
    repaint();
}

void ApertuneAudioProcessorEditor::setAccidental(int spellingIndex)
{
    if (auto* p = audioProcessor.getState().getParameter(accidentalSpellingParameterId))
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(spellingIndex)));
    repaint();
}

void ApertuneAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    if (!showSettings)
        return;
    const auto pos = event.getPosition();
    for (const auto& h : instrumentHits) if (h.first.contains(pos)) { setInstrumentScope(h.second); return; }
    for (const auto& h : tuningHits)     if (h.first.contains(pos)) { setTuningPreset(h.second); return; }
    for (const auto& h : accidentalHits) if (h.first.contains(pos)) { setAccidental(h.second); return; }
}

void ApertuneAudioProcessorEditor::setSettingsVisible(bool shouldShow)
{
    showSettings = shouldShow;

    settingsButton.setVisible(!shouldShow);
    closeSettingsButton.setVisible(shouldShow);
    unitCentsButton.setVisible(!shouldShow);
    unitHzButton.setVisible(!shouldShow);
    refUpButton.setVisible(!shouldShow);
    refDownButton.setVisible(!shouldShow);

    concertASlider.setVisible(shouldShow);

    repaint();
}

void ApertuneAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(bg());
    const auto panelBounds = getLocalBounds().toFloat();
    if (showSettings)
        paintSettingsPanel(graphics, panelBounds);
    else
        paintTunerFace(graphics, panelBounds);
}

void ApertuneAudioProcessorEditor::paintTunerFace(juce::Graphics& graphics, juce::Rectangle<float> panelBounds)
{
    const auto muted = *audioProcessor.getState().getRawParameterValue(muteParameterId) > 0.5f;
    const auto concertA = static_cast<double>(*audioProcessor.getState().getRawParameterValue(concertAParameterId));
    const auto settings = audioProcessor.getTunerSettings();
    const auto frame = apertune::makeTunerUiFrame(audioProcessor.getLastPitchReading(), muted, concertA, settings);
    const auto accent = tuneColour(frame);

    juce::ColourGradient grad(panelTop(), panelBounds.getX(), panelBounds.getY(), panelBottom(), panelBounds.getX(), panelBounds.getBottom(), false);
    graphics.setGradientFill(grad);
    graphics.fillRect(panelBounds);
    juce::ColourGradient hi(juce::Colours::white.withAlpha(0.045f), panelBounds.getCentreX(), panelBounds.getY(),
                            juce::Colours::transparentWhite, panelBounds.getCentreX(), panelBounds.getY() + panelBounds.getHeight() * 0.55f, false);
    graphics.setGradientFill(hi);
    graphics.fillRect(panelBounds);
    graphics.setTiledImageFill(grainImage, 0, 0, 0.05f);
    graphics.fillRect(panelBounds);

    auto titleBar = panelBounds.removeFromTop(50.0f);
    graphics.setColour(juce::Colours::white.withAlpha(0.025f));
    graphics.fillRect(titleBar);
    graphics.setColour(juce::Colours::white.withAlpha(0.06f));
    graphics.drawLine(titleBar.getX(), titleBar.getBottom(), titleBar.getRight(), titleBar.getBottom(), 1.0f);
    graphics.setColour(textPrimary());
    graphics.setFont(grotesk(16.0f, 600));
    graphics.drawText("Tuner", titleBar.withTrimmedLeft(22.0f).toNearestInt(), juce::Justification::centredLeft);
    drawSpeaker(graphics, muteButton.getBounds().toFloat().reduced(3.0f), muted ? textMuted() : iconColour(), muted);
    drawGearIcon(graphics, settingsButton.getBounds().toFloat().reduced(3.0f), iconColour());

    if (frame.inLock)
    {
        graphics.setColour(accent.withAlpha(0.55f));
        graphics.drawRect(getLocalBounds().toFloat(), 1.5f);
        for (int i = 1; i <= 5; ++i)
        {
            graphics.setColour(accent.withAlpha(0.05f * static_cast<float>(6 - i)));
            graphics.drawRect(getLocalBounds().toFloat().reduced(static_cast<float>(i) * 2.0f), 1.0f);
        }
    }

    const auto bodyLeft = 30.0f, bodyRight = panelBounds.getRight() - 30.0f, bodyW = bodyRight - bodyLeft;
    const auto centreX = (bodyLeft + bodyRight) * 0.5f;

    {
        auto row = juce::Rectangle<float>(bodyLeft, 84.0f, bodyW, 30.0f);
        const auto n = static_cast<int>(frame.stringLabels.size());
        const auto chip = 30.0f, gap = 10.0f;
        const auto total = chip * static_cast<float>(n) + gap * static_cast<float>(juce::jmax(0, n - 1));
        auto x = row.getCentreX() - total * 0.5f;
        for (int i = 0; i < n; ++i)
        {
            const auto dot = juce::Rectangle<float>(x, row.getCentreY() - chip * 0.5f, chip, chip);
            const auto tuned = frame.tunedStrings[static_cast<std::size_t>(i)];
            const auto active = frame.activeStringIndex == i;
            graphics.setColour(tuned ? grn().withAlpha(0.14f) : juce::Colours::transparentBlack);
            graphics.fillEllipse(dot);
            graphics.setColour(active ? accent : (tuned ? grn().withAlpha(0.55f) : stringBorder()));
            graphics.drawEllipse(dot, 1.5f);
            if (active)
                graphics.drawEllipse(dot.expanded(3.0f), 1.2f);
            graphics.setColour(active ? accent : (tuned ? grn() : stringIdle()));
            graphics.setFont(mono(13.0f, true));
            graphics.drawText(frame.stringLabels[static_cast<std::size_t>(i)], dot.toNearestInt(), juce::Justification::centred);
            x += chip + gap;
        }
    }

    {
        auto row = juce::Rectangle<float>(bodyLeft, 126.0f, bodyW, 30.0f);
        graphics.setFont(mono(14.0f));
        graphics.setColour(textMuted());
        graphics.drawText("-50", row.removeFromLeft(44.0f).toNearestInt(), juce::Justification::centredLeft);
        graphics.drawText("+50", row.removeFromRight(44.0f).toNearestInt(), juce::Justification::centredRight);
        graphics.setFont(mono(22.0f));
        graphics.setColour(frame.inLock ? accent : juce::Colour::fromRGB(207, 210, 214));
        graphics.drawText(centsText(frame), juce::Rectangle<float>(centreX - 60.0f, row.getY(), 120.0f, row.getHeight()).toNearestInt(), juce::Justification::centred);
        if (frame.showChevron)
        {
            const auto sharpSide = frame.cents > 0.0;
            drawChevron(graphics, sharpSide ? centreX + 78.0f : centreX - 78.0f, row.getCentreY(), sharpSide, accent);
        }
    }

    {
        auto track = juce::Rectangle<float>(bodyLeft, 168.0f, bodyW, 150.0f);
        const auto cy = track.getCentreY();
        for (int i = 0; i <= 40; ++i)
        {
            const auto tx = track.getX() + track.getWidth() * static_cast<float>(i) / 40.0f;
            const auto isMid = i == 20;
            const auto isMajor = i % 5 == 0;
            const auto hh = (isMid ? 56.0f : (isMajor ? 20.0f : 12.0f)) * 0.5f;
            graphics.setColour(isMid ? textPrimary().withAlpha(0.42f) : (isMajor ? tickMajor() : tickDim()));
            graphics.drawLine(tx, cy - hh, tx, cy + hh, isMid ? 2.0f : 1.5f);
        }
        const auto rd = static_cast<float>(frame.reticleDiameter);
        drawReticle(graphics, juce::Rectangle<float>(centreX - rd * 0.5f, cy - rd * 0.5f, rd, rd),
                    frame.inLock ? accent : juce::Colour::fromRGB(74, 81, 88));
        const auto bd = 60.0f;
        const auto bx = track.getX() + track.getWidth() * static_cast<float>(frame.ballPosition);
        const auto ball = juce::Rectangle<float>(bx - bd * 0.5f, cy - bd * 0.5f, bd, bd);
        for (int i = 4; i >= 1; --i)
        {
            graphics.setColour(accent.withAlpha(static_cast<float>(frame.visibility) * 0.06f));
            graphics.fillEllipse(ball.expanded(static_cast<float>(i) * 4.0f));
        }
        graphics.setColour(accent.withAlpha(static_cast<float>(frame.visibility)));
        if (frame.hasSignal || frame.muted)
            graphics.fillEllipse(ball);
        else
            graphics.drawEllipse(ball, 2.0f);
        graphics.setColour(textPrimary().withAlpha(frame.inLock ? 0.7f : 0.32f));
        graphics.drawEllipse(ball.reduced(9.0f), frame.inLock ? 2.0f : 1.2f);
    }

    {
        auto row = juce::Rectangle<float>(bodyLeft, 322.0f, bodyW, 86.0f);
        graphics.setFont(grotesk(19.0f, 500));
        graphics.setColour(textMuted());
        graphics.drawText(frame.hasSignal ? juce::String(frame.previousNoteName.data()) : juce::String("--"),
                          row.removeFromLeft(96.0f).toNearestInt(), juce::Justification::centredLeft);
        graphics.drawText(frame.hasSignal ? juce::String(frame.nextNoteName.data()) : juce::String("--"),
                          row.removeFromRight(96.0f).toNearestInt(), juce::Justification::centredRight);

        const auto note = (frame.hasSignal || frame.muted) ? juce::String(frame.noteName.data()) : juce::String("--");
        auto noteFont = apertune::ui::font(apertune::ui::archivoBlack(), 86.0f);
        graphics.setColour(frame.inLock ? accent : (frame.hasSignal ? textPrimary() : textMuted()));
        graphics.setFont(noteFont);
        auto noteBox = row.withSizeKeepingCentre(180.0f, row.getHeight());
        graphics.drawText(note, noteBox.toNearestInt(), juce::Justification::centred);
        if (frame.hasSignal || frame.muted)
        {
            juce::GlyphArrangement glyphs;
            glyphs.addLineOfText(noteFont, note, 0.0f, 0.0f);
            const auto glyphRight = noteBox.getCentreX() + glyphs.getBoundingBox(0, -1, true).getWidth() * 0.5f;
            graphics.setFont(mono(22.0f, true));
            graphics.setColour(octaveCol());
            graphics.drawText(juce::String(frame.octave),
                              juce::Rectangle<float>(glyphRight + 5.0f, row.getCentreY() + 2.0f, 36.0f, 28.0f).toNearestInt(),
                              juce::Justification::centredLeft);
        }
    }

    {
        const auto toggle = unitCentsButton.getBounds().toFloat().getUnion(unitHzButton.getBounds().toFloat());
        graphics.setColour(juce::Colours::white.withAlpha(0.13f));
        graphics.drawRoundedRectangle(toggle, 8.0f, 1.0f);
        const auto centsOn = settings.displayUnit == apertune::DisplayUnit::cents;
        auto seg = [&](juce::Rectangle<float> r, const char* label, bool on)
        {
            if (on)
            {
                graphics.setColour(juce::Colours::white.withAlpha(0.10f));
                graphics.fillRoundedRectangle(r.reduced(1.5f), 6.5f);
            }
            graphics.setColour(on ? juce::Colours::white : iconColour());
            graphics.setFont(grotesk(13.0f, 500));
            graphics.drawText(label, r.toNearestInt(), juce::Justification::centred);
        };
        seg(unitCentsButton.getBounds().toFloat(), "Cents", centsOn);
        seg(unitHzButton.getBounds().toFloat(), "Hz", !centsOn);

        graphics.setFont(mono(15.0f));
        graphics.setColour(textSecondary());
        const auto hzText = (frame.hasSignal || frame.muted) ? juce::String(frame.frequencyHz, 1) + " Hz" : juce::String("--.- Hz");
        graphics.drawText(hzText, juce::Rectangle<float>(centreX - 80.0f, toggle.getCentreY() - 12.0f, 160.0f, 24.0f).toNearestInt(), juce::Justification::centred);

        const auto stepBox = refUpButton.getBounds().toFloat().getUnion(refDownButton.getBounds().toFloat());
        const auto refBox = juce::Rectangle<float>(stepBox.getRight() - 86.0f, toggle.getY(), 86.0f, toggle.getHeight());
        graphics.setColour(juce::Colours::white.withAlpha(0.13f));
        graphics.drawRoundedRectangle(refBox, 8.0f, 1.0f);
        graphics.setFont(mono(14.0f));
        graphics.setColour(textPrimary());
        graphics.drawText(juce::String(concertA, 1), refBox.withTrimmedRight(20.0f).withTrimmedLeft(10.0f).toNearestInt(), juce::Justification::centredLeft);
        drawStepArrow(graphics, refUpButton.getBounds().toFloat(), true, textMuted());
        drawStepArrow(graphics, refDownButton.getBounds().toFloat(), false, textMuted());
    }
}

void ApertuneAudioProcessorEditor::paintSettingsPanel(juce::Graphics& graphics, juce::Rectangle<float> panelBounds)
{
    juce::ColourGradient grad(panelTop(), panelBounds.getX(), panelBounds.getY(), panelBottom(), panelBounds.getX(), panelBounds.getBottom(), false);
    graphics.setGradientFill(grad);
    graphics.fillRect(panelBounds);
    graphics.setTiledImageFill(grainImage, 0, 0, 0.05f);
    graphics.fillRect(panelBounds);

    auto titleBar = panelBounds.removeFromTop(50.0f);
    graphics.setColour(juce::Colours::white.withAlpha(0.025f));
    graphics.fillRect(titleBar);
    graphics.setColour(juce::Colours::white.withAlpha(0.06f));
    graphics.drawLine(titleBar.getX(), titleBar.getBottom(), titleBar.getRight(), titleBar.getBottom(), 1.0f);
    graphics.setColour(textPrimary());
    graphics.setFont(grotesk(16.0f, 600));
    graphics.drawText(juce::String::fromUTF8("Tuner \xc2\xb7 Settings"), titleBar.withTrimmedLeft(22.0f).toNearestInt(), juce::Justification::centredLeft);
    drawClose(graphics, closeSettingsButton.getBounds().toFloat().reduced(4.0f), iconColour());

    instrumentHits.clear();
    tuningHits.clear();
    accidentalHits.clear();

    const auto settings = audioProcessor.getTunerSettings();
    const auto concertA = static_cast<double>(*audioProcessor.getState().getRawParameterValue(concertAParameterId));
    const auto bodyLeft = 30.0f, bodyW = panelBounds.getRight() - 30.0f - bodyLeft;

    auto sectionLabel = [&](const juce::String& text, float y)
    {
        graphics.setColour(textMuted());
        graphics.setFont(grotesk(11.0f, 600).withExtraKerningFactor(0.14f));
        graphics.drawText(text.toUpperCase(), juce::Rectangle<float>(bodyLeft, y, bodyW, 16.0f).toNearestInt(), juce::Justification::centredLeft);
    };

    // Reference pitch
    sectionLabel("Reference pitch", 82.0f);
    graphics.setColour(juce::Colours::white);
    graphics.setFont(mono(30.0f, true));
    graphics.drawText(juce::String(concertA, 1), juce::Rectangle<float>(bodyLeft, 102.0f, 110.0f, 40.0f).toNearestInt(), juce::Justification::centredLeft);
    graphics.setColour(textMuted());
    graphics.setFont(mono(13.0f));
    graphics.drawText("Hz", juce::Rectangle<float>(bodyLeft + 96.0f, 114.0f, 24.0f, 20.0f).toNearestInt(), juce::Justification::centredLeft);
    {
        const auto sb = concertASlider.getBounds().toFloat();
        graphics.setFont(mono(11.0f));
        graphics.setColour(textMuted());
        graphics.drawText("432", juce::Rectangle<float>(sb.getX(), sb.getBottom(), 40.0f, 14.0f).toNearestInt(), juce::Justification::centredLeft);
        graphics.drawText("440", juce::Rectangle<float>(sb.getCentreX() - 20.0f, sb.getBottom(), 40.0f, 14.0f).toNearestInt(), juce::Justification::centred);
        graphics.drawText("448", juce::Rectangle<float>(sb.getRight() - 40.0f, sb.getBottom(), 40.0f, 14.0f).toNearestInt(), juce::Justification::centredRight);
    }

    // Tuning preset
    sectionLabel("Tuning preset", 162.0f);
    drawSegmented(graphics, juce::Rectangle<float>(bodyLeft, 182.0f, bodyW, 32.0f),
                  { juce::String::fromUTF8("Bass \xc2\xb7 4-6"),
                    juce::String::fromUTF8("Guitar \xc2\xb7 6-9"),
                    "Custom" },
                  static_cast<int>(settings.instrumentScope), instrumentHits);

    auto y = 224.0f;
    const auto rowH = 34.0f, rowGap = 6.0f;
    for (const auto preset : apertune::presetsForScope(settings.instrumentScope))
    {
        const auto def = apertune::tuningDefinitionForPreset(preset);
        juce::String notes;
        for (const auto& s : def.stringLabels)
            notes += (notes.isEmpty() ? "" : " ") + juce::String(s);
        const auto row = juce::Rectangle<float>(bodyLeft, y, bodyW, rowH);
        const auto on = preset == settings.tuningPreset;
        if (on)
        {
            graphics.setColour(grn().withAlpha(0.09f));
            graphics.fillRoundedRectangle(row, 8.0f);
            graphics.setColour(grn().withAlpha(0.55f));
            graphics.drawRoundedRectangle(row, 8.0f, 1.0f);
        }
        else
        {
            graphics.setColour(juce::Colours::white.withAlpha(0.03f));
            graphics.fillRoundedRectangle(row, 8.0f);
        }
        const auto radio = juce::Rectangle<float>(row.getX() + 12.0f, row.getCentreY() - 8.0f, 16.0f, 16.0f);
        graphics.setColour(on ? grn() : radioIdle());
        graphics.drawEllipse(radio, 1.6f);
        if (on)
        {
            graphics.setColour(grn());
            graphics.fillEllipse(radio.withSizeKeepingCentre(8.0f, 8.0f));
        }
        graphics.setColour(rowName());
        graphics.setFont(grotesk(15.0f, 500));
        graphics.drawText(juce::String(apertune::tuningPresetName(preset).data()),
                          juce::Rectangle<float>(radio.getRight() + 12.0f, row.getY(), 134.0f, rowH).toNearestInt(), juce::Justification::centredLeft);
        graphics.setColour(iconColour());
        graphics.setFont(mono(13.0f));
        graphics.drawText(notes, juce::Rectangle<float>(radio.getRight() + 150.0f, row.getY(),
                          row.getRight() - (radio.getRight() + 150.0f) - 56.0f, rowH).toNearestInt(), juce::Justification::centredLeft);
        graphics.setColour(textMuted());
        graphics.setFont(mono(11.0f));
        graphics.drawText(juce::String(static_cast<int>(def.stringLabels.size())) + " str",
                          row.withTrimmedRight(12.0f).toNearestInt(), juce::Justification::centredRight);
        tuningHits.push_back({ row.toNearestInt(), static_cast<int>(preset) });
        y += rowH + rowGap;
    }

    // Accidentals
    const auto accLabelY = juce::jmin(y + 6.0f, panelBounds.getBottom() - 70.0f);
    sectionLabel("Accidentals", accLabelY);
    drawSegmented(graphics, juce::Rectangle<float>(bodyLeft, accLabelY + 20.0f, 200.0f, 30.0f),
                  { "Sharps", "Flats" }, static_cast<int>(settings.accidentalSpelling), accidentalHits);
}

void ApertuneAudioProcessorEditor::resized()
{
    auto full = getLocalBounds();
    auto titleBar = full.removeFromTop(50).reduced(22, 12);

    auto iconRow = titleBar.removeFromRight(70);
    auto rightIcon = iconRow.removeFromRight(26);
    iconRow.removeFromRight(14);
    auto leftIcon = iconRow.removeFromRight(26);
    muteButton.setBounds(leftIcon);
    settingsButton.setBounds(rightIcon);
    closeSettingsButton.setBounds(rightIcon);

    const int bodyLeft = 30, bodyRight = getWidth() - 30;
    auto footer = juce::Rectangle<int>(bodyLeft, 406, bodyRight - bodyLeft, 32);
    unitCentsButton.setBounds(footer.getX(), footer.getY(), 64, 32);
    unitHzButton.setBounds(footer.getX() + 64, footer.getY(), 50, 32);
    auto refField = juce::Rectangle<int>(footer.getRight() - 86, footer.getY(), 86, 32);
    auto steppers = refField.removeFromRight(20).reduced(2);
    refUpButton.setBounds(steppers.removeFromTop(steppers.getHeight() / 2));
    refDownButton.setBounds(steppers);

    // settings reference slider (to the right of the big value)
    concertASlider.setBounds(bodyLeft + 150, 108, (bodyRight - (bodyLeft + 150)), 22);
}

void ApertuneAudioProcessorEditor::parameterChanged(const juce::String&, float)
{
    juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer<ApertuneAudioProcessorEditor>(this)]
    {
        if (safeThis != nullptr)
            safeThis->repaint();
    });
}
