#include "KillaLookAndFeel.h"

namespace rk::ui
{

KillaLookAndFeel::KillaLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, colours::background);
    setColour (juce::TextButton::buttonColourId, colours::panelLight);
    setColour (juce::TextButton::buttonOnColourId, colours::accent);
    setColour (juce::TextButton::textColourOffId, colours::text);
    setColour (juce::TextButton::textColourOnId, colours::accent);
    setColour (juce::ComboBox::backgroundColourId, colours::panelLight);
    setColour (juce::ComboBox::textColourId, colours::text);
    setColour (juce::ComboBox::outlineColourId, colours::outline);
    setColour (juce::ComboBox::arrowColourId, colours::text);
    setColour (juce::PopupMenu::backgroundColourId, colours::panel);
    setColour (juce::PopupMenu::textColourId, colours::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::accent.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, colours::text);
    setColour (juce::Label::textColourId, colours::text);
    setColour (juce::Slider::textBoxTextColourId, colours::text);
    setColour (juce::Slider::textBoxBackgroundColourId, colours::background);
    setColour (juce::Slider::textBoxOutlineColourId, colours::outline);
    setColour (juce::Slider::textBoxHighlightColourId, colours::accent.withAlpha (0.4f));
    setColour (juce::TextEditor::backgroundColourId, colours::background);
    setColour (juce::TextEditor::textColourId, colours::text);
    setColour (juce::TextEditor::highlightColourId, colours::accent.withAlpha (0.35f));
    setColour (juce::TextEditor::outlineColourId, colours::outline);
    setColour (juce::TextEditor::focusedOutlineColourId, colours::accent);
    setColour (juce::CaretComponent::caretColourId, colours::accent);
    setColour (juce::ScrollBar::thumbColourId, colours::outlineLight);
    setColour (juce::AlertWindow::backgroundColourId, colours::panel);
    setColour (juce::AlertWindow::textColourId, colours::text);
    setColour (juce::AlertWindow::outlineColourId, colours::accent);
    setColour (juce::TooltipWindow::backgroundColourId, colours::panel);
    setColour (juce::TooltipWindow::textColourId, colours::text);
    setColour (juce::TooltipWindow::outlineColourId, colours::outlineLight);
}

void KillaLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                                         float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto centre = bounds.getCentre();
    const auto radius = size * 0.5f - 4.0f;
    const auto knobRadius = radius * 0.74f;
    const auto enabled = slider.isEnabled();
    const auto hot = slider.isMouseOverOrDragging();

    // scale ticks
    g.setColour (colours::textDark);
    for (int i = 0; i <= 10; ++i)
    {
        const auto a = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
        const auto p = centre.getPointOnCircumference (radius + 1.5f, a);
        g.fillEllipse (p.x - 0.9f, p.y - 0.9f, 1.8f, 1.8f);
    }

    // value arc (bipolar sliders draw from the centre)
    const auto bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const auto zeroPos = bipolar ? (float) slider.valueToProportionOfLength (0.0) : 0.0f;
    const auto a0 = startAngle + (endAngle - startAngle) * juce::jmin (zeroPos, pos);
    const auto a1 = startAngle + (endAngle - startAngle) * juce::jmax (zeroPos, pos);
    const auto arcR = radius - 3.0f;

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (colours::outline);
    g.strokePath (track, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (a1 - a0 > 0.001f && enabled)
    {
        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, a0, a1, true);
        for (int i = 3; i > 0; --i)
        {
            g.setColour (colours::accent.withAlpha (0.10f * (hot ? 1.6f : 1.0f)));
            g.strokePath (arc, juce::PathStrokeType (2.5f + (float) i * 2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setColour (colours::accent);
        g.strokePath (arc, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // knob body
    const auto body = juce::Rectangle<float> (knobRadius * 2.0f, knobRadius * 2.0f).withCentre (centre);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (body.translated (0.0f, 2.5f).expanded (1.5f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2c33), body.getX(), body.getY(),
                                             juce::Colour (0xff0b0b0d), body.getRight(), body.getBottom(), false));
    g.fillEllipse (body);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.10f), body.getCentreX(), body.getY(),
                                             juce::Colours::transparentWhite, body.getCentreX(), body.getCentreY(), false));
    g.fillEllipse (body.reduced (1.5f));
    g.setColour (juce::Colours::black);
    g.drawEllipse (body, 1.0f);

    // pointer
    const auto angle = startAngle + (endAngle - startAngle) * pos;
    const auto p0 = centre.getPointOnCircumference (knobRadius * 0.25f, angle);
    const auto p1 = centre.getPointOnCircumference (knobRadius * 0.85f, angle);
    g.setColour (enabled ? colours::text : colours::textDark);
    g.drawLine ({ p0, p1 }, 2.2f);
}

void KillaLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hi, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const auto on = b.getToggleState();
    const auto corner = 4.0f;

    g.setColour (down ? colours::background : (hi ? colours::panelLight.brighter (0.08f) : colours::panelLight));
    g.fillRoundedRectangle (r, corner);

    if (on)
    {
        drawGlow (g, r, corner, colours::accent, 0.8f, 3);
        g.setColour (colours::accent.withAlpha (0.12f));
        g.fillRoundedRectangle (r, corner);
        g.setColour (colours::accent);
    }
    else
    {
        g.setColour (hi ? colours::outlineLight : colours::outline);
    }

    g.drawRoundedRectangle (r, corner, 1.0f);
}

juce::Font KillaLookAndFeel::getTextButtonFont (juce::TextButton&, int h)
{
    return uiFont (juce::jmin (13.0f, (float) h * 0.5f), false);
}

void KillaLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (b.getToggleState() ? colours::accent : (b.isEnabled() ? colours::text : colours::textDark));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (4, 0), juce::Justification::centred, 1);
}

void KillaLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f);
    g.setColour (colours::panelLight);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (box.isMouseOver() ? colours::outlineLight : colours::outline);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    juce::Path arrow;
    const auto cx = (float) w - 12.0f, cy = (float) h * 0.5f;
    arrow.startNewSubPath (cx - 4.0f, cy - 2.0f);
    arrow.lineTo (cx, cy + 2.0f);
    arrow.lineTo (cx + 4.0f, cy - 2.0f);
    g.setColour (colours::text);
    g.strokePath (arrow, juce::PathStrokeType (1.5f));
}

juce::Font KillaLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return uiFont (juce::jmin (13.0f, (float) box.getHeight() * 0.55f), false);
}

void KillaLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (6, 1, box.getWidth() - 24, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

void KillaLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (colours::panel);
    g.setColour (colours::outlineLight);
    g.drawRect (0, 0, w, h);
}

void KillaLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                                          bool isHighlighted, bool isTicked, bool, const juce::String& text,
                                          const juce::String&, const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour (colours::outline);
        g.fillRect (area.reduced (6, 0).withHeight (1).withY (area.getCentreY()));
        return;
    }

    if (isHighlighted && isActive)
    {
        g.setColour (colours::accent.withAlpha (0.2f));
        g.fillRect (area);
    }

    g.setColour (isTicked ? colours::accent : (isActive ? colours::text : colours::textDark));
    g.setFont (getPopupMenuFont());
    g.drawFittedText (text, area.reduced (12, 0), juce::Justification::centredLeft, 1);
}

juce::Font KillaLookAndFeel::getPopupMenuFont()
{
    return uiFont (14.0f, false);
}

void KillaLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor& ed)
{
    g.setColour (ed.hasKeyboardFocus (true) ? colours::accent.withAlpha (0.7f) : colours::outline);
    g.drawRoundedRectangle (juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f), 4.0f, 1.0f);
}

void KillaLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor&)
{
    g.setColour (colours::background);
    g.fillRoundedRectangle (juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h), 4.0f);
}

void KillaLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                                      int thumbStart, int thumbSize, bool over, bool down)
{
    auto thumb = vertical ? juce::Rectangle<int> (x, thumbStart, w, thumbSize) : juce::Rectangle<int> (thumbStart, y, thumbSize, h);
    g.setColour ((over || down) ? colours::accent.withAlpha (0.6f) : colours::outlineLight);
    g.fillRoundedRectangle (thumb.toFloat().reduced (2.0f), 3.0f);
}

juce::Label* KillaLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (uiFont (13.0f, false));
    l->setJustificationType (juce::Justification::centred);
    return l;
}

juce::Font KillaLookAndFeel::getLabelFont (juce::Label& l)
{
    return l.getFont();
}

void KillaLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    if (dynamic_cast<juce::Slider*> (label.getParentComponent()) != nullptr)
    {
        // value box under the knobs
        const auto r = label.getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (colours::background);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (label.isBeingEdited() ? colours::accent : colours::outline);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);

        if (! label.isBeingEdited())
        {
            g.setColour (colours::text);
            g.setFont (label.getFont());
            g.drawFittedText (label.getText(), label.getLocalBounds().reduced (2, 0), juce::Justification::centred, 1, 1.0f);
        }
        return;
    }

    LookAndFeel_V4::drawLabel (g, label);
}

} // namespace rk::ui
