#include "Widgets.h"

#include "../engine/HatSampler.h"

namespace rk::ui
{

void drawIcon (juce::Graphics& g, Icon icon, juce::Rectangle<float> r, juce::Colour colour)
{
    const auto s = juce::jmin (r.getWidth(), r.getHeight());
    const auto c = r.getCentre();
    const auto h = s * 0.5f;
    juce::Path p;
    auto stroke = juce::PathStrokeType (juce::jmax (1.3f, s * 0.09f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    bool fill = false;

    switch (icon)
    {
        case Icon::prev:
            p.startNewSubPath (c.x + h * 0.3f, c.y - h * 0.6f); p.lineTo (c.x - h * 0.3f, c.y); p.lineTo (c.x + h * 0.3f, c.y + h * 0.6f);
            break;
        case Icon::next:
            p.startNewSubPath (c.x - h * 0.3f, c.y - h * 0.6f); p.lineTo (c.x + h * 0.3f, c.y); p.lineTo (c.x - h * 0.3f, c.y + h * 0.6f);
            break;
        case Icon::chevronDown:
            p.startNewSubPath (c.x - h * 0.5f, c.y - h * 0.2f); p.lineTo (c.x, c.y + h * 0.3f); p.lineTo (c.x + h * 0.5f, c.y - h * 0.2f);
            break;
        case Icon::undo:
        case Icon::redo:
        {
            // hook arrow: tip on the left, tail curls down to the right (mirrored for redo)
            const juce::Point<float> tip (c.x - h * 0.75f, c.y - h * 0.3f);
            p.startNewSubPath (tip);
            p.lineTo (c.x + h * 0.1f, c.y - h * 0.3f);
            p.cubicTo (c.x + h * 0.85f, c.y - h * 0.3f, c.x + h * 0.85f, c.y + h * 0.6f, c.x + h * 0.1f, c.y + h * 0.6f);
            p.lineTo (c.x - h * 0.35f, c.y + h * 0.6f);
            p.startNewSubPath (tip.x + h * 0.38f, tip.y - h * 0.36f);
            p.lineTo (tip);
            p.lineTo (tip.x + h * 0.38f, tip.y + h * 0.36f);
            if (icon == Icon::redo)
                p.applyTransform (juce::AffineTransform::scale (-1.0f, 1.0f, c.x, c.y));
            break;
        }
        case Icon::play:
            p.addTriangle (c.x - h * 0.4f, c.y - h * 0.55f, c.x - h * 0.4f, c.y + h * 0.55f, c.x + h * 0.55f, c.y);
            fill = true;
            break;
        case Icon::stop:
            p.addRectangle (c.x - h * 0.45f, c.y - h * 0.45f, h * 0.9f, h * 0.9f);
            fill = true;
            break;
        case Icon::star:
        case Icon::starFilled:
            p.addStar (c, 5, h * 0.38f, h * 0.9f, 0.0f);
            fill = icon == Icon::starFilled;
            break;
        case Icon::folder:
            p.startNewSubPath (c.x - h * 0.8f, c.y - h * 0.5f);
            p.lineTo (c.x - h * 0.3f, c.y - h * 0.5f);
            p.lineTo (c.x - h * 0.15f, c.y - h * 0.3f);
            p.lineTo (c.x + h * 0.8f, c.y - h * 0.3f);
            p.lineTo (c.x + h * 0.8f, c.y + h * 0.55f);
            p.lineTo (c.x - h * 0.8f, c.y + h * 0.55f);
            p.closeSubPath();
            break;
        case Icon::lock:
        case Icon::unlock:
            p.addRoundedRectangle (c.x - h * 0.55f, c.y - h * 0.05f, h * 1.1f, h * 0.8f, h * 0.1f);
            p.startNewSubPath (c.x - h * 0.33f, c.y - h * 0.05f);
            p.lineTo (c.x - h * 0.33f, c.y - h * 0.4f);
            p.addCentredArc (c.x, c.y - h * 0.4f, h * 0.33f, h * 0.33f, 0.0f, -juce::MathConstants<float>::halfPi,
                             juce::MathConstants<float>::halfPi, false);
            if (icon == Icon::lock)
                p.lineTo (c.x + h * 0.33f, c.y - h * 0.05f);
            else
                p.lineTo (c.x + h * 0.33f, c.y - h * 0.3f);
            if (icon == Icon::lock)
            {
                g.setColour (colour);
                g.fillRoundedRectangle (c.x - h * 0.55f, c.y - h * 0.05f, h * 1.1f, h * 0.8f, h * 0.1f);
            }
            break;
        case Icon::close:
            p.startNewSubPath (c.x - h * 0.5f, c.y - h * 0.5f); p.lineTo (c.x + h * 0.5f, c.y + h * 0.5f);
            p.startNewSubPath (c.x + h * 0.5f, c.y - h * 0.5f); p.lineTo (c.x - h * 0.5f, c.y + h * 0.5f);
            break;
        case Icon::search:
            p.addEllipse (c.x - h * 0.6f, c.y - h * 0.6f, h * 0.95f, h * 0.95f);
            p.startNewSubPath (c.x + h * 0.2f, c.y + h * 0.2f); p.lineTo (c.x + h * 0.65f, c.y + h * 0.65f);
            break;
        case Icon::exportFile:
            p.startNewSubPath (c.x, c.y + h * 0.2f); p.lineTo (c.x, c.y - h * 0.7f);
            p.startNewSubPath (c.x - h * 0.35f, c.y - h * 0.35f); p.lineTo (c.x, c.y - h * 0.7f); p.lineTo (c.x + h * 0.35f, c.y - h * 0.35f);
            p.startNewSubPath (c.x - h * 0.7f, c.y); p.lineTo (c.x - h * 0.7f, c.y + h * 0.65f);
            p.lineTo (c.x + h * 0.7f, c.y + h * 0.65f); p.lineTo (c.x + h * 0.7f, c.y);
            break;
        case Icon::midiDrag:
        {
            // file with folded corner + arrow out
            p.startNewSubPath (c.x - h * 0.7f, c.y - h * 0.95f);
            p.lineTo (c.x + h * 0.15f, c.y - h * 0.95f);
            p.lineTo (c.x + h * 0.6f, c.y - h * 0.5f);
            p.lineTo (c.x + h * 0.6f, c.y - h * 0.15f);
            p.startNewSubPath (c.x + h * 0.6f, c.y + h * 0.5f);
            p.lineTo (c.x + h * 0.6f, c.y + h * 0.95f);
            p.lineTo (c.x - h * 0.7f, c.y + h * 0.95f);
            p.closeSubPath();
            p.startNewSubPath (c.x + h * 0.15f, c.y - h * 0.95f); p.lineTo (c.x + h * 0.15f, c.y - h * 0.5f); p.lineTo (c.x + h * 0.6f, c.y - h * 0.5f);
            p.startNewSubPath (c.x - h * 0.25f, c.y + h * 0.18f); p.lineTo (c.x + h * 1.0f, c.y + h * 0.18f);
            p.startNewSubPath (c.x + h * 0.7f, c.y - h * 0.12f); p.lineTo (c.x + h * 1.0f, c.y + h * 0.18f); p.lineTo (c.x + h * 0.7f, c.y + h * 0.48f);
            break;
        }
    }

    g.setColour (colour);
    if (fill)
        g.fillPath (p);
    else
        g.strokePath (p, stroke);
}

//==============================================================================
IconButton::IconButton (const juce::String& name, Icon i, bool drawFrame) : juce::Button (name), icon (i), frame (drawFrame)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void IconButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);

    if (frame)
    {
        g.setColour (down ? colours::background : (highlighted ? colours::panelLight.brighter (0.08f) : colours::panelLight));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (getToggleState() ? colours::accent : (highlighted ? colours::outlineLight : colours::outline));
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
    }

    const auto col = ! isEnabled() ? colours::textDark
                   : getToggleState() ? colours::accent
                   : (highlighted ? iconColour.brighter (0.3f) : iconColour);

    if (label.isEmpty())
    {
        const auto s = juce::jmin (r.getWidth(), r.getHeight()) * (frame ? 0.5f : 0.7f);
        drawIcon (g, icon, juce::Rectangle<float> (s, s).withCentre (r.getCentre()), col);
        return;
    }

    auto content = r.reduced (6.0f, 0.0f);
    const auto s = juce::jmin (16.0f, r.getHeight() * 0.5f);
    g.setFont (uiFont (juce::jmin (12.5f, r.getHeight() * 0.42f)).withExtraKerningFactor (0.06f));
    const auto textW = (float) juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), label) + 6.0f;
    const auto total = s + 8.0f + textW;
    auto row = content.withSizeKeepingCentre (juce::jmin (total, content.getWidth()), r.getHeight());
    drawIcon (g, icon, row.removeFromLeft (s).withSizeKeepingCentre (s, s), col);
    row.removeFromLeft (8.0f);
    g.setColour (isEnabled() ? colours::text : colours::textDark);
    g.drawText (label, row, juce::Justification::centredLeft, false);
}

//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& t, bool isSmall)
    : title (t), small (isSmall), attachment (state, paramId, slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, isSmall ? 58 : 76, isSmall ? 18 : 22);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    slider.setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    slider.setVelocityBasedMode (false);
    slider.setMouseDragSensitivity (180);
    if (auto* p = state.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    addAndMakeVisible (slider);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (small ? 16 : 20);
    slider.setBounds (r);
}

void Knob::paint (juce::Graphics& g)
{
    g.setColour (colours::text.withAlpha (0.85f));
    g.setFont (labelFont (small ? 10.5f : 12.0f));
    g.drawText (title, getLocalBounds().removeFromTop (small ? 16 : 20), juce::Justification::centred, false);
}

//==============================================================================
SegmentedChoice::SegmentedChoice (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::StringArray& labels)
    : param (*state.getParameter (paramId)),
      attachment (param, [this] (float v) { update (v); }, state.undoManager)
{
    for (int i = 0; i < labels.size(); ++i)
    {
        auto* b = buttons.add (new juce::TextButton (labels[i]));
        b->setClickingTogglesState (false);
        b->setMouseCursor (juce::MouseCursor::PointingHandCursor);
        b->onClick = [this, i] { attachment.setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }
    attachment.sendInitialUpdate();
}

void SegmentedChoice::update (float value)
{
    const auto index = juce::roundToInt (value);
    for (int i = 0; i < buttons.size(); ++i)
        buttons[i]->setToggleState (i == index, juce::dontSendNotification);
}

void SegmentedChoice::resized()
{
    auto r = getLocalBounds();
    const auto w = r.getWidth() / juce::jmax (1, buttons.size());
    for (int i = 0; i < buttons.size(); ++i)
        buttons[i]->setBounds (i == buttons.size() - 1 ? r : r.removeFromLeft (w));
}

//==============================================================================
void ToggleSwitch::paintButton (juce::Graphics& g, bool highlighted, bool)
{
    auto r = getLocalBounds().toFloat().withSizeKeepingCentre (38.0f, 20.0f);
    const auto on = getToggleState();

    if (on)
        drawGlow (g, r, 10.0f, colours::accent, 0.9f, 4);

    g.setColour (on ? colours::accent.withAlpha (0.85f) : colours::panelLight);
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (on ? colours::accent : (highlighted ? colours::outlineLight : colours::outline));
    g.drawRoundedRectangle (r, 10.0f, 1.0f);

    const auto knob = juce::Rectangle<float> (14.0f, 14.0f).withCentre ({ on ? r.getRight() - 10.0f : r.getX() + 10.0f, r.getCentreY() });
    g.setColour (on ? juce::Colours::white : colours::textDim);
    g.fillEllipse (knob);
}

//==============================================================================
KillButton::KillButton() : juce::Button ("KILL")
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTooltip ("KILL - new variation of this preset (right-click: back to original)");
}

void KillButton::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
    {
        juce::Button::mouseDown (e);
        return;
    }

    juce::PopupMenu menu;
    if (onSamePresetVariation != nullptr)
        menu.addItem (2, "New variation of this preset only");
    menu.addItem (1, "Back to the original preset", seedText.isNotEmpty());
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [safe = juce::Component::SafePointer<KillButton> (this)] (int result)
                        {
                            if (safe == nullptr)
                                return;
                            if (result == 1 && safe->onBackToOriginal != nullptr)
                                safe->onBackToOriginal();
                            if (result == 2 && safe->onSamePresetVariation != nullptr)
                                safe->onSamePresetVariation();
                        });
}

void KillButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (6.0f);
    const auto corner = 8.0f;
    if (down)
        r = r.reduced (1.5f);

    drawGlow (g, r, corner, colours::accent, highlighted ? 2.2f : 1.5f, 7);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff4452), r.getX(), r.getY(),
                                             juce::Colour (0xff9e0c18), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, corner);

    // metallic bevel
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.28f), r.getX(), r.getY(),
                                             juce::Colours::transparentWhite, r.getX(), r.getY() + r.getHeight() * 0.45f, false));
    g.fillRoundedRectangle (r.reduced (2.0f), corner - 2.0f);
    g.setColour (juce::Colour (0xffff8a94));
    g.drawRoundedRectangle (r.reduced (1.0f), corner, 1.2f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (r, corner, 1.0f);

    // text
    auto textArea = r;
    if (caption.isNotEmpty())
    {
        auto cap = textArea.removeFromBottom (r.getHeight() * 0.26f);
        g.setColour (juce::Colour (0xff2a0206).withAlpha (0.85f));
        g.setFont (labelFont (juce::jmax (7.5f, r.getHeight() * 0.13f)));
        g.drawText (caption, cap.translated (0.0f, -r.getHeight() * 0.06f), juce::Justification::centred, false);
    }
    auto font = juce::Font (juce::FontOptions (textArea.getHeight() * 0.86f, juce::Font::bold)).withHorizontalScale (1.05f);
    juce::GlyphArrangement ga;
    ga.addFittedText (font, "KILL", textArea.getX(), textArea.getY(), textArea.getWidth(), textArea.getHeight(), juce::Justification::centred, 1);
    juce::Path letters;
    ga.createPath (letters);
    letters.applyTransform (juce::AffineTransform::shear (-0.12f, 0.0f).translated (r.getCentreY() * 0.12f, 0.0f));
    g.setColour (juce::Colour (0xff4a0008).withAlpha (0.6f));
    g.fillPath (letters, juce::AffineTransform::translation (0.0f, 2.0f));
    g.setColour (juce::Colour (0xff120203));
    g.fillPath (letters);

    if (seedText.isNotEmpty() && caption.isEmpty())
    {
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.setFont (uiFont (11.0f));
        g.drawText (seedText, r.reduced (10.0f, 6.0f), juce::Justification::bottomRight, false);
    }
}

//==============================================================================
void DragMidiZone::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const auto hot = isMouseOver() || dragging;

    juce::Path border;
    border.addRoundedRectangle (r, 6.0f);
    juce::Path dashed;
    const float dashes[] { 5.0f, 4.0f };
    juce::PathStrokeType (1.2f).createDashedStroke (dashed, border, dashes, 2);
    g.setColour (hot ? colours::accent : colours::outlineLight);
    g.fillPath (dashed);

    if (hot)
    {
        g.setColour (colours::accent.withAlpha (0.06f));
        g.fillRoundedRectangle (r, 6.0f);
    }

    if (vertical)
    {
        auto c = r.reduced (6.0f);
        const auto iconSize = juce::jmin (c.getHeight() * 0.5f, 34.0f);
        drawIcon (g, Icon::midiDrag, c.removeFromTop (c.getHeight() * 0.62f).withSizeKeepingCentre (iconSize, iconSize),
                  hot ? colours::accent : colours::textDim);
        g.setColour (hot ? colours::accent : colours::text);
        g.setFont (labelFont (11.5f));
        g.drawText ("DRAG MIDI", c, juce::Justification::centredTop, false);
        return;
    }

    auto content = r.reduced (12.0f);
    const auto iconSize = juce::jmin (content.getHeight() * 0.7f, 42.0f);
    auto row = content.withSizeKeepingCentre (juce::jmin (content.getWidth(), iconSize + 150.0f), content.getHeight());
    drawIcon (g, Icon::midiDrag, row.removeFromLeft (iconSize).withSizeKeepingCentre (iconSize, iconSize),
              hot ? colours::accent : colours::textDim);
    row.removeFromLeft (14.0f);

    g.setColour (colours::text);
    g.setFont (labelFont (15.0f));
    g.drawText ("DRAG MIDI", row.removeFromTop (row.getHeight() * 0.55f), juce::Justification::bottomLeft, false);
    g.setColour (colours::textDim);
    g.setFont (uiFont (12.0f, false));
    g.drawText ("drag to your DAW", row, juce::Justification::topLeft, false);
}

void DragMidiZone::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6 || createFile == nullptr)
        return;

    const auto file = createFile();
    if (! file.existsAsFile())
        return;

    dragging = true;
    repaint();
    juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this,
                                                                [safe = juce::Component::SafePointer<DragMidiZone> (this)]
                                                                {
                                                                    if (safe != nullptr)
                                                                    {
                                                                        safe->dragging = false;
                                                                        safe->repaint();
                                                                    }
                                                                });
}

//==============================================================================
void WaveformView::setSample (std::shared_ptr<const HatSample> s)
{
    sample = std::move (s);
    repaint();
}

void WaveformView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (colours::background);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (dragOver ? colours::accent : colours::outline);
    g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);

    auto area = r.reduced (8.0f, 6.0f);
    const auto mid = area.getCentreY();
    g.setColour (colours::outline);
    g.drawHorizontalLine ((int) mid, area.getX(), area.getRight());

    if (sample != nullptr && sample->audio.getNumSamples() > 0)
    {
        const auto& audio = sample->audio;
        const auto n = audio.getNumSamples();
        const auto* d = audio.getReadPointer (0);
        const auto width = (int) area.getWidth();
        const auto peak = juce::jmax (0.001f, audio.getMagnitude (0, 0, n));

        juce::Path wave;
        for (int x = 0; x < width; ++x)
        {
            const auto s0 = (int) ((double) x / width * n);
            const auto s1 = juce::jmax (s0 + 1, (int) ((double) (x + 1) / width * n));
            float lo = 0.0f, hi = 0.0f;
            for (int i = s0; i < juce::jmin (s1, n); ++i)
            {
                lo = juce::jmin (lo, d[i]);
                hi = juce::jmax (hi, d[i]);
            }
            if (hi - lo < peak * 0.01f)
                continue;
            const auto px = area.getX() + (float) x;
            wave.addRectangle (px, mid - hi / peak * area.getHeight() * 0.5f, 1.0f,
                               juce::jmax (1.0f, (hi - lo) / peak * area.getHeight() * 0.5f));
        }
        g.setColour (colours::accent.withAlpha (0.25f));
        g.fillPath (wave, juce::AffineTransform::translation (0.0f, 0.5f));
        g.setColour (colours::accent);
        g.fillPath (wave);
    }

    g.setColour (dragOver ? colours::accent : colours::textDark);
    g.setFont (uiFont (11.0f, false));
    g.drawText (dragOver ? "drop to load" : "drop WAV", area, juce::Justification::centredRight, false);
}

bool WaveformView::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac"))
            return true;
    return false;
}

void WaveformView::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    repaint();
    for (const auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac") && onFileDropped != nullptr)
        {
            onFileDropped (juce::File (f));
            break;
        }
}

//==============================================================================
HatSelector::HatSelector (juce::AudioProcessorValueTreeState& state, std::function<juce::StringArray()> names)
    : getNames (std::move (names)),
      param (*state.getParameter ("hat")),
      attachment (param, [this] (float v) { index = juce::roundToInt (v); repaint(); if (onChange) onChange(); }, state.undoManager)
{
    addAndMakeVisible (prev);
    addAndMakeVisible (next);
    prev.onClick = [this] { step (-1); };
    next.onClick = [this] { step (1); };
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    attachment.sendInitialUpdate();
}

void HatSelector::step (int delta)
{
    const auto n = getNames().size();
    auto i = index + delta;
    // skip the empty custom slot when stepping
    const auto names = getNames();
    for (int tries = 0; tries < n; ++tries, i += delta)
    {
        i = (i % n + n) % n;
        if (names[i] != "Custom WAV")
            break;
    }
    setIndex (i);
}

void HatSelector::setIndex (int newIndex)
{
    attachment.setValueAsCompleteGesture ((float) newIndex);
}

void HatSelector::resized()
{
    auto r = getLocalBounds();
    prev.setBounds (r.removeFromLeft (r.getHeight()));
    next.setBounds (r.removeFromRight (r.getHeight()));
}

void HatSelector::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (colours::panelLight);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (isMouseOver (true) ? colours::outlineLight : colours::outline);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    g.drawVerticalLine ((int) r.getHeight() + 1, r.getY() + 3.0f, r.getBottom() - 3.0f);
    g.drawVerticalLine ((int) (r.getRight() - r.getHeight()), r.getY() + 3.0f, r.getBottom() - 3.0f);

    g.setColour (colours::text);
    g.setFont (uiFont (14.0f, false));
    g.drawText (getNames()[index], getLocalBounds().reduced (getHeight(), 0), juce::Justification::centred, true);
}

void HatSelector::mouseUp (const juce::MouseEvent& e)
{
    if (! getLocalBounds().reduced (getHeight(), 0).contains (e.getPosition()))
        return;

    juce::PopupMenu menu;
    const auto names = getNames();
    for (int i = 0; i < names.size(); ++i)
        menu.addItem (i + 1, names[i], names[i] != "Custom WAV", i == index);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()),
                        [safe = juce::Component::SafePointer<HatSelector> (this)] (int result)
                        {
                            if (safe != nullptr && result > 0)
                                safe->setIndex (result - 1);
                        });
}

} // namespace rk::ui
