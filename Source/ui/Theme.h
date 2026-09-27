#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace rk::ui
{

// Killa series palette
namespace colours
{
    inline const juce::Colour background   { 0xff0e0e10 };
    inline const juce::Colour panel        { 0xff141417 };
    inline const juce::Colour panelLight   { 0xff1b1b1f };
    inline const juce::Colour outline      { 0xff2a2a30 };
    inline const juce::Colour outlineLight { 0xff3a3a42 };
    inline const juce::Colour accent       { 0xffff2e3e };
    inline const juce::Colour accentDark   { 0xffb3121f };
    inline const juce::Colour accentPink   { 0xffff7ba5 };
    inline const juce::Colour text         { 0xfff2f2f4 };
    inline const juce::Colour textDim      { 0xff9a9aa3 };
    inline const juce::Colour textDark     { 0xff5c5c66 };
}

/** Bold, slightly condensed UI font (JUCE default typeface, no font files needed). */
inline juce::Font uiFont (float height, bool bold = true)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain)).withHorizontalScale (0.92f);
}

/** Wide-tracked label font ("HI-HAT ROLL PRESETS", section titles). */
inline juce::Font labelFont (float height)
{
    return uiFont (height).withExtraKerningFactor (0.18f);
}

/** Soft glow around a rectangle (cheap: a few translucent strokes). */
inline void drawGlow (juce::Graphics& g, juce::Rectangle<float> r, float corner, juce::Colour c, float strength = 1.0f, int layers = 5)
{
    for (int i = layers; i > 0; --i)
    {
        const auto grow = (float) i * 1.6f;
        g.setColour (c.withAlpha (0.06f * strength * (float) (layers - i + 1) / (float) layers));
        g.drawRoundedRectangle (r.expanded (grow), corner + grow, 2.0f);
    }
}

/** Panel background used by all sections. */
inline void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float corner = 6.0f)
{
    g.setGradientFill (juce::ColourGradient (colours::panelLight, r.getX(), r.getY(), colours::panel, r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, corner);
    g.setColour (colours::outline);
    g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.0f);
}

/** Colour of a note by pitch offset: deep red (low) -> red (root) -> pink -> white (high). */
inline juce::Colour pitchColour (int pitch)
{
    if (pitch <= 0)
        return colours::accent.interpolatedWith (colours::accentDark, juce::jlimit (0.0f, 1.0f, (float) -pitch / 12.0f));
    if (pitch <= 12)
        return colours::accent.interpolatedWith (colours::accentPink, (float) pitch / 12.0f);
    return colours::accentPink.interpolatedWith (juce::Colours::white, juce::jlimit (0.0f, 1.0f, (float) (pitch - 12) / 12.0f));
}

} // namespace rk::ui
