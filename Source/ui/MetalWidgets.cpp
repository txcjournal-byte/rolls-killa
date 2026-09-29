#include "MetalWidgets.h"

#include "../PluginProcessor.h"
#include "../engine/HatSampler.h"

namespace rk::ui::metal
{

//==============================================================================
juce::Font plateFont (float height, float kerning)
{
    return juce::Font (juce::FontOptions (height, juce::Font::bold)).withHorizontalScale (0.9f).withExtraKerningFactor (kerning);
}

juce::Image renderSteel (int width, int height, float scale)
{
    const auto w = juce::jmax (1, juce::roundToInt ((float) width * scale));
    const auto h = juce::jmax (1, juce::roundToInt ((float) height * scale));
    juce::Image img (juce::Image::RGB, w, h, false);
    juce::Random rng (7);

    // low-frequency grime (bilinear value noise)
    constexpr int gx = 24, gy = 10;
    float grid[gy + 1][gx + 1];
    for (auto& row : grid)
        for (auto& v : row)
            v = rng.nextFloat();

    auto grime = [&] (float u, float v)
    {
        const auto fx = u * gx, fy = v * gy;
        const auto x0 = juce::jmin (gx - 1, (int) fx), y0 = juce::jmin (gy - 1, (int) fy);
        const auto tx = fx - (float) x0, ty = fy - (float) y0;
        const auto a = grid[y0][x0] + (grid[y0][x0 + 1] - grid[y0][x0]) * tx;
        const auto b = grid[y0 + 1][x0] + (grid[y0 + 1][x0 + 1] - grid[y0 + 1][x0]) * tx;
        return a + (b - a) * ty;
    };

    {
        juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);
        float streak = 0.0f;
        for (int y = 0; y < h; ++y)
        {
            streak = streak * 0.82f + (rng.nextFloat() - 0.5f) * 0.09f;   // brushed rows
            const auto v = (float) y / (float) h;
            for (int x = 0; x < w; ++x)
            {
                const auto u = (float) x / (float) w;
                const auto edge = juce::jmin (u, 1.0f - u, v * 0.4f, (1.0f - v) * 0.4f);
                const auto vignette = 0.72f + 0.28f * juce::jlimit (0.0f, 1.0f, edge * 9.0f);
                const auto light = 1.06f - 0.16f * (u * 0.6f + v * 0.4f);      // light from the top left
                const auto g = grime (u, v);
                auto lum = 0.43f + streak + (rng.nextFloat() - 0.5f) * 0.05f - (g * g) * 0.13f;
                lum *= vignette * light;
                const auto warm = juce::jlimit (0.0f, 0.02f, (g - 0.6f) * 0.1f);   // a hint of rust in the dirty spots
                data.setPixelColour (x, y, juce::Colour::fromFloatRGBA (juce::jlimit (0.0f, 1.0f, lum + warm),
                                                                        juce::jlimit (0.0f, 1.0f, lum),
                                                                        juce::jlimit (0.0f, 1.0f, lum - warm * 0.6f), 1.0f));
            }
        }
    }

    juce::Graphics g (img);
    g.addTransform (juce::AffineTransform::scale (scale));

    // scratches
    for (int i = 0; i < 160; ++i)
    {
        const auto x = rng.nextFloat() * (float) width, y = rng.nextFloat() * (float) height;
        const auto len = 4.0f + rng.nextFloat() * 30.0f;
        const auto angle = (rng.nextFloat() - 0.5f) * 0.9f + (rng.nextBool() ? 0.0f : 3.14159f);
        const auto light = rng.nextBool();
        g.setColour ((light ? juce::Colours::white : juce::Colours::black).withAlpha (0.05f + rng.nextFloat() * 0.09f));
        g.drawLine (x, y, x + std::cos (angle) * len, y + std::sin (angle) * len, 0.35f + rng.nextFloat() * 0.4f);
    }

    // dirt spots
    for (int i = 0; i < 26; ++i)
    {
        const auto c = juce::Point<float> (rng.nextFloat() * (float) width, rng.nextFloat() * (float) height);
        const auto r = 4.0f + rng.nextFloat() * 22.0f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1a1410).withAlpha (0.10f + rng.nextFloat() * 0.12f), c,
                                                 juce::Colour (0xff1a1410).withAlpha (0.0f), c.translated (r, 0.0f), true));
        g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 1.4f).withCentre (c));
    }
    return img;
}

void drawScrew (juce::Graphics& g, juce::Point<float> c, float r, float angle)
{
    const auto area = juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (area.expanded (1.2f).translated (0.0f, 0.8f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc9c9c4), area.getX(), area.getY(),
                                             juce::Colour (0xff3e3e3b), area.getRight(), area.getBottom(), false));
    g.fillEllipse (area);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (area, 0.7f);

    juce::Path slot;
    slot.addRectangle (-r * 0.75f, -r * 0.14f, r * 1.5f, r * 0.28f);
    slot.addRectangle (-r * 0.14f, -r * 0.75f, r * 0.28f, r * 1.5f);
    const auto t = juce::AffineTransform::rotation (angle).translated (c);
    g.setColour (juce::Colours::white.withAlpha (0.3f));
    g.fillPath (slot, t.translated (0.0f, 0.6f));
    g.setColour (juce::Colour (0xff161615));
    g.fillPath (slot, t);
}

void drawEngravedText (juce::Graphics& g, const juce::String& text, juce::Font font, juce::Rectangle<float> r,
                       juce::Justification just, float alpha)
{
    g.setFont (font);
    g.setColour (juce::Colours::white.withAlpha (0.28f * alpha));
    g.drawText (text, r.translated (0.0f, 0.7f), just, false);
    g.setColour (colours::engrave.withAlpha (0.92f * alpha));
    g.drawText (text, r, just, false);
}

void drawRecess (juce::Graphics& g, juce::Rectangle<float> r, float corner)
{
    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.drawRoundedRectangle (r.expanded (1.0f).translated (0.0f, 0.8f), corner + 1.0f, 1.0f);
    g.setColour (juce::Colour (0xff0d0c0b));
    g.fillRoundedRectangle (r, corner);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.7f), r.getX(), r.getY(),
                                             juce::Colours::transparentBlack, r.getX(), r.getY() + juce::jmin (10.0f, r.getHeight() * 0.4f), false));
    g.fillRoundedRectangle (r, corner);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (r, corner, 1.0f);
}

void drawGroove (juce::Graphics& g, float x, float y0, float y1)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRect (juce::Rectangle<float> (x - 0.5f, y0, 1.1f, y1 - y0));
    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.fillRect (juce::Rectangle<float> (x + 0.6f, y0, 0.7f, y1 - y0));
}

void drawCap (juce::Graphics& g, juce::Rectangle<float> r, float corner, bool highlighted, bool down)
{
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (r.expanded (1.2f).translated (0.0f, 1.0f), corner + 1.0f);
    if (down)
        r = r.translated (0.0f, 0.6f);
    const auto top = juce::Colour (0xffb9b9b4).withMultipliedBrightness (highlighted ? 1.08f : 1.0f);
    g.setGradientFill (juce::ColourGradient (down ? top.darker (0.35f) : top, r.getX(), r.getY(),
                                             juce::Colour (0xff4a4a47), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, corner);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8d8d88), r.getX(), r.getY(),
                                             juce::Colour (0xff5d5d59), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r.reduced (1.6f), juce::jmax (1.0f, corner - 1.5f));
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawRoundedRectangle (r.reduced (0.6f), corner, 0.6f);
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.drawRoundedRectangle (r, corner, 0.8f);
}

void drawLed (juce::Graphics& g, juce::Point<float> c, float r, float amount)
{
    const auto area = juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
    if (amount > 0.0f)
    {
        g.setGradientFill (juce::ColourGradient (colours::ember.withAlpha (0.45f * amount), c,
                                                 colours::ember.withAlpha (0.0f), c.translated (r * 3.2f, 0.0f), true));
        g.fillEllipse (area.expanded (r * 2.2f));
    }
    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.drawEllipse (area.expanded (0.9f).translated (0.0f, 0.5f), 0.6f);
    g.setColour (juce::Colour (0xff0c0b0a));
    g.fillEllipse (area.expanded (0.6f));
    const auto on = colours::emberDeep.interpolatedWith (colours::emberHot, amount);
    g.setGradientFill (juce::ColourGradient (amount > 0.0f ? on.brighter (0.4f) : juce::Colour (0xff3a3533), c.x - r * 0.3f, c.y - r * 0.4f,
                                             amount > 0.0f ? on.darker (0.3f) : juce::Colour (0xff141211), c.x, c.y + r, true));
    g.fillEllipse (area);
}

//==============================================================================
MetalButton::MetalButton (const juce::String& name, Icon i) : juce::Button (name), icon (i)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void MetalButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (1.5f);
    drawCap (g, r, 3.0f, highlighted && isEnabled(), down);
    const auto ic = r.reduced (r.getWidth() * 0.26f).translated (0.0f, down ? 0.6f : 0.0f);
    drawIcon (g, icon, ic.translated (0.0f, 0.7f), juce::Colours::white.withAlpha (0.25f));
    drawIcon (g, icon, ic, isEnabled() ? juce::Colour (0xff1b1b1a) : juce::Colour (0xff1b1b1a).withAlpha (0.35f));
}

//==============================================================================
BarsLamps::BarsLamps (juce::AudioProcessorValueTreeState& state)
    : param (*state.getParameter (params::bars)),
      attachment (param, [this] (float v) { index = juce::roundToInt (v); repaint(); }, state.undoManager)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    attachment.sendInitialUpdate();
}

juce::Rectangle<float> BarsLamps::lamp (int i) const
{
    const auto step = (float) getWidth() / 4.0f;
    const auto d = juce::jmin (step * 0.72f, (float) getHeight() * 0.5f);
    return juce::Rectangle<float> (d, d).withCentre ({ step * ((float) i + 0.5f), (float) getHeight() - d * 0.5f - 2.0f });
}

void BarsLamps::paint (juce::Graphics& g)
{
    const auto top = (float) getHeight() - lamp (0).getHeight() - 4.0f;
    drawEngravedText (g, "BARS", plateFont (8.0f, 0.2f), { 0.0f, 0.0f, (float) getWidth(), top * 0.5f }, juce::Justification::centredLeft);
    for (int i = 0; i < 4; ++i)
    {
        const auto r = lamp (i);
        drawEngravedText (g, params::barsChoices[i], plateFont (8.5f, 0.0f), { r.getX() - 4.0f, top * 0.5f, r.getWidth() + 8.0f, top * 0.5f },
                          juce::Justification::centred);
        const auto on = i == index;
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (r.expanded (1.3f).translated (0.0f, 1.0f));
        if (on)
        {
            g.setGradientFill (juce::ColourGradient (colours::ember.withAlpha (0.55f), r.getCentre(),
                                                     colours::ember.withAlpha (0.0f), r.getCentre().translated (r.getWidth() * 1.2f, 0.0f), true));
            g.fillEllipse (r.expanded (r.getWidth() * 0.6f));
            g.setGradientFill (juce::ColourGradient (colours::emberHot, r.getCentreX() - r.getWidth() * 0.15f, r.getY() + r.getHeight() * 0.3f,
                                                     colours::emberDeep, r.getRight(), r.getBottom(), true));
        }
        else
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc4c4bf), r.getX(), r.getY(),
                                                     juce::Colour (0xff353533), r.getRight(), r.getBottom(), false));
        }
        g.fillEllipse (r);
        g.setColour (on ? juce::Colour (0xffffe0b8).withAlpha (0.8f) : juce::Colours::white.withAlpha (0.25f));
        g.fillEllipse (r.reduced (r.getWidth() * 0.32f).translated (-r.getWidth() * 0.08f, -r.getHeight() * 0.12f));
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawEllipse (r, 0.8f);
    }
}

void BarsLamps::mouseUp (const juce::MouseEvent& e)
{
    const auto i = juce::jlimit (0, 3, (int) (e.position.x / ((float) getWidth() / 4.0f)));
    attachment.setValueAsCompleteGesture ((float) i);
}

//==============================================================================
MoodSwitch::MoodSwitch()
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

float MoodSwitch::slotX (int i) const
{
    const auto pad = (float) getWidth() * 0.14f;
    return pad + ((float) getWidth() - 2.0f * pad) * (float) i * 0.5f;
}

void MoodSwitch::paint (juce::Graphics& g)
{
    const auto h = (float) getHeight();
    const char* names[] { "CHILL", "TRAP", "CRAZY" };
    for (int i = 0; i < 3; ++i)
        drawEngravedText (g, names[i], plateFont (8.0f, 0.02f), { slotX (i) - 22.0f, 0.0f, 44.0f, h * 0.48f }, juce::Justification::centred,
                          i == mood ? 1.0f : 0.7f);

    auto track = juce::Rectangle<float> (2.0f, h * 0.58f, (float) getWidth() - 4.0f, h * 0.34f);
    drawRecess (g, track, track.getHeight() * 0.5f);
    for (int i = 0; i < 3; ++i)
    {
        const auto c = juce::Point<float> (slotX (i), track.getCentreY());
        if (i == mood)
            drawLed (g, c, track.getHeight() * 0.36f, 1.0f);
        else
        {
            g.setColour (juce::Colour (0xff4a4a47));
            g.fillEllipse (juce::Rectangle<float> (3.0f, 3.0f).withCentre (c));
        }
    }
}

void MoodSwitch::mouseDown (const juce::MouseEvent& e)
{
    int best = 0;
    for (int i = 1; i < 3; ++i)
        if (std::abs (e.position.x - slotX (i)) < std::abs (e.position.x - slotX (best)))
            best = i;
    if (best != mood)
    {
        mood = best;
        repaint();
        if (onChange != nullptr)
            onChange (mood);
    }
}

//==============================================================================
void MetalKillButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto outer = getLocalBounds().toFloat().reduced (2.0f);
    const auto corner = outer.getHeight() * 0.42f;

    // steel bezel
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (outer.expanded (1.0f).translated (0.0f, 1.5f), corner);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd0d0cb), outer.getX(), outer.getY(),
                                             juce::Colour (0xff323230), outer.getX(), outer.getBottom(), false));
    g.fillRoundedRectangle (outer, corner);
    auto inner = outer.reduced (outer.getHeight() * 0.11f);
    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.fillRoundedRectangle (inner.expanded (1.0f), corner * 0.8f);

    // red cap
    auto cap = inner.reduced (1.5f);
    if (down)
        cap = cap.reduced (1.0f).translated (0.0f, 0.8f);
    const auto red = juce::Colour (0xffe8391c).withMultipliedBrightness (highlighted ? 1.12f : 1.0f);
    g.setGradientFill (juce::ColourGradient (red.brighter (0.25f), cap.getX(), cap.getY(),
                                             juce::Colour (0xff7a1206), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, corner * 0.72f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (down ? 0.12f : 0.3f), cap.getX(), cap.getY(),
                                             juce::Colours::transparentWhite, cap.getX(), cap.getCentreY(), false));
    g.fillRoundedRectangle (cap.reduced (2.0f).withTrimmedBottom (cap.getHeight() * 0.45f), corner * 0.6f);
    g.setColour (juce::Colour (0xffff8a60).withAlpha (0.55f));
    g.drawRoundedRectangle (cap.reduced (0.8f), corner * 0.72f, 0.8f);

    // engraved KILL
    juce::GlyphArrangement ga;
    ga.addFittedText (juce::Font (juce::FontOptions (cap.getHeight() * 0.62f, juce::Font::bold)).withHorizontalScale (0.95f),
                      "KILL", cap.getX(), cap.getY(), cap.getWidth(), cap.getHeight(), juce::Justification::centred, 1);
    juce::Path letters;
    ga.createPath (letters);
    g.setColour (juce::Colour (0xffff9a7a).withAlpha (0.45f));
    g.fillPath (letters, juce::AffineTransform::translation (0.0f, 0.9f));
    g.setColour (juce::Colour (0xff2a0602));
    g.fillPath (letters);
}

//==============================================================================
void WaveWindow::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    drawRecess (g, r, 2.5f);
    if (dragOver)
    {
        g.setColour (colours::ember.withAlpha (0.8f));
        g.drawRoundedRectangle (r, 2.5f, 1.0f);
    }

    auto area = r.reduced (4.0f, 4.0f);
    const auto mid = area.getCentreY();
    if (sample != nullptr && sample->audio.getNumSamples() > 0)
    {
        const auto& audio = sample->audio;
        const auto n = audio.getNumSamples();
        const auto* d = audio.getReadPointer (0);
        const auto width = (int) area.getWidth() * 2;
        const auto peak = juce::jmax (0.001f, audio.getMagnitude (0, 0, n));

        juce::Path wave;
        for (int x = 0; x < width; ++x)
        {
            const auto s0 = (int) ((double) x / width * n);
            const auto s1 = juce::jmax (s0 + 1, (int) ((double) (x + 1) / width * n));
            float hi = 0.0f;
            for (int i = s0; i < juce::jmin (s1, n); ++i)
                hi = juce::jmax (hi, std::abs (d[i]));
            const auto a = juce::jmax (0.3f, hi / peak * area.getHeight() * 0.5f);
            wave.addRectangle (area.getX() + (float) x * 0.5f, mid - a, 0.5f, a * 2.0f);
        }
        g.setColour (colours::ember.withAlpha (0.3f));
        g.fillPath (wave, juce::AffineTransform::scale (1.0f, 1.12f, area.getCentreX(), mid));
        g.setColour (colours::ember);
        g.fillPath (wave);
    }
    if (dragOver)
    {
        g.setColour (colours::emberHot);
        g.setFont (plateFont (7.0f, 0.0f));
        g.drawText ("DROP WAV", area, juce::Justification::centred, false);
    }
}

HatPicker::HatPicker (juce::AudioProcessorValueTreeState& state, std::function<juce::StringArray()> names)
    : getNames (std::move (names)),
      param (*state.getParameter (params::hat)),
      attachment (param, [this] (float v) { index = juce::roundToInt (v); repaint(); if (onChange) onChange(); }, state.undoManager)
{
    addAndMakeVisible (wave);
    addAndMakeVisible (prev);
    addAndMakeVisible (next);
    prev.onClick = [this] { step (-1); };
    next.onClick = [this] { step (1); };
    prev.setTooltip ("Previous hi-hat");
    next.setTooltip ("Next hi-hat");
    wave.setTooltip ("Drop your own WAV here");
    attachment.sendInitialUpdate();
}

juce::Rectangle<int> HatPicker::namePlate() const
{
    return getLocalBounds().removeFromBottom (13).reduced (12, 0);
}

void HatPicker::resized()
{
    auto r = getLocalBounds();
    r.removeFromBottom (16);
    const auto bw = 13;
    prev.setBounds (r.removeFromLeft (bw).withSizeKeepingCentre (bw, 17));
    next.setBounds (r.removeFromRight (bw).withSizeKeepingCentre (bw, 17));
    wave.setBounds (r.reduced (3, 0));
}

void HatPicker::paint (juce::Graphics& g)
{
    auto plate = namePlate().toFloat();
    drawCap (g, plate, 2.0f, false, false);
    g.setColour (juce::Colour (0xff2a2a28));
    g.fillEllipse (juce::Rectangle<float> (2.0f, 2.0f).withCentre ({ plate.getX() + 3.5f, plate.getCentreY() }));
    g.fillEllipse (juce::Rectangle<float> (2.0f, 2.0f).withCentre ({ plate.getRight() - 3.5f, plate.getCentreY() }));
    drawEngravedText (g, getNames()[index].toUpperCase(), plateFont (7.6f, 0.04f), plate.reduced (6.0f, 0.0f), juce::Justification::centred);
}

void HatPicker::step (int delta)
{
    const auto names = getNames();
    const auto n = names.size();
    auto i = index + delta;
    for (int tries = 0; tries < n; ++tries, i += delta)
    {
        i = (i % n + n) % n;
        if (names[i] != "Custom WAV")
            break;
    }
    attachment.setValueAsCompleteGesture ((float) i);
}

void HatPicker::mouseUp (const juce::MouseEvent& e)
{
    if (! namePlate().contains (e.getPosition()))
        return;

    juce::PopupMenu menu;
    const auto names = getNames();
    for (int i = 0; i < names.size(); ++i)
        menu.addItem (i + 1, names[i], names[i] != "Custom WAV", i == index);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [safe = juce::Component::SafePointer<HatPicker> (this)] (int result)
                        {
                            if (safe != nullptr && result > 0)
                                safe->attachment.setValueAsCompleteGesture ((float) (result - 1));
                        });
}

//==============================================================================
void DragDawButton::paint (juce::Graphics& g)
{
    const auto hot = isMouseOver() || dragging;
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    drawCap (g, r, 4.0f, hot, dragging);
    const auto ic = r.reduced (r.getWidth() * 0.24f).translated (0.0f, dragging ? 0.6f : 0.0f);
    drawIcon (g, Icon::exportFile, ic.translated (0.0f, 0.7f), juce::Colours::white.withAlpha (0.25f));
    drawIcon (g, Icon::exportFile, ic, hot ? colours::emberDeep : juce::Colour (0xff1b1b1a));
}

//==============================================================================
void RoundPlayButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat();
    const auto label = r.removeFromTop (12.0f);
    drawEngravedText (g, hostPlaying ? "SYNC" : previewing ? "STOP" : "PLAY", plateFont (8.5f, 0.12f), label, juce::Justification::centred);

    const auto d = juce::jmin (r.getWidth(), r.getHeight()) - 3.0f;
    auto ring = juce::Rectangle<float> (d, d).withCentre (r.getCentre());
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (ring.expanded (1.2f).translated (0.0f, 1.2f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd2d2cd), ring.getX(), ring.getY(),
                                             juce::Colour (0xff2e2e2c), ring.getRight(), ring.getBottom(), false));
    g.fillEllipse (ring);
    auto face = ring.reduced (d * 0.12f);
    if (down)
        face = face.reduced (0.6f).translated (0.0f, 0.6f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2a28), face.getX(), face.getY(),
                                             juce::Colour (0xff070606), face.getX(), face.getBottom(), false));
    g.fillEllipse (face);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawEllipse (face.reduced (0.6f), 0.6f);

    const auto c = face.getCentre();
    const auto s = face.getWidth() * 0.42f;
    juce::Path shape;
    if (previewing)
        shape.addRoundedRectangle (c.x - s * 0.36f, c.y - s * 0.36f, s * 0.72f, s * 0.72f, 1.5f);
    else
        shape.addTriangle (c.x - s * 0.34f, c.y - s * 0.46f, c.x - s * 0.34f, c.y + s * 0.46f, c.x + s * 0.5f, c.y);

    const auto lit = ! hostPlaying;
    const auto col = lit ? colours::ember.brighter (highlighted ? 0.25f : 0.0f) : colours::emberDeep.withAlpha (0.7f);
    if (lit)
    {
        g.setGradientFill (juce::ColourGradient (colours::ember.withAlpha (0.35f), c, colours::ember.withAlpha (0.0f), c.translated (s * 1.2f, 0.0f), true));
        g.fillEllipse (face);
    }
    g.setGradientFill (juce::ColourGradient (col.brighter (0.4f), c.x, c.y - s * 0.5f, col.darker (0.3f), c.x, c.y + s * 0.5f, false));
    g.fillPath (shape);
}

ModeToggle::ModeToggle (juce::AudioProcessorValueTreeState& state)
    : param (*state.getParameter (params::playMode)),
      attachment (param, [this] (float v) { mode = juce::roundToInt (v); repaint(); }, state.undoManager)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    attachment.sendInitialUpdate();
}

void ModeToggle::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto top = r.removeFromTop (r.getHeight() * 0.5f);
    drawEngravedText (g, "MIDI", plateFont (7.6f, 0.04f), top.removeFromLeft (top.getWidth() * 0.46f), juce::Justification::centredRight,
                      mode == 0 ? 1.0f : 0.6f);
    drawEngravedText (g, "|", plateFont (7.6f, 0.0f), top.removeFromLeft (top.getWidth() * 0.15f), juce::Justification::centred, 0.6f);
    drawEngravedText (g, "HOST", plateFont (7.6f, 0.04f), top, juce::Justification::centredLeft, mode == 1 ? 1.0f : 0.6f);

    auto track = r.withSizeKeepingCentre (r.getWidth() - 10.0f, juce::jmin (9.0f, r.getHeight() - 2.0f));
    drawRecess (g, track, track.getHeight() * 0.5f);
    const auto knobD = track.getHeight() - 2.0f;
    const auto x = mode == 0 ? track.getX() + 1.0f + knobD * 0.5f : track.getRight() - 1.0f - knobD * 0.5f;
    drawLed (g, { x, track.getCentreY() }, knobD * 0.5f, 1.0f);
}

void ModeToggle::mouseUp (const juce::MouseEvent& e)
{
    const auto newMode = e.position.x < (float) getWidth() * 0.5f ? 0 : 1;
    attachment.setValueAsCompleteGesture ((float) (newMode == mode ? 1 - mode : newMode));
}

//==============================================================================
BpmLcd::BpmLcd (RollsKillaProcessor& p) : proc (p)
{
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void BpmLcd::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    drawRecess (g, r, 3.0f);
    auto glass = r.reduced (3.0f);
    g.setColour (juce::Colour (0xff140a06));
    g.fillRoundedRectangle (glass, 2.0f);

    const auto manual = proc.getTargetBpm() > 0.0;
    const auto bpm = juce::String (juce::roundToInt (proc.getEffectiveBpm())) + " BPM";
    auto main = glass.removeFromTop (glass.getHeight() * 0.64f);
    const auto font = juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), main.getHeight() * 0.78f, juce::Font::bold))
                          .withHorizontalScale (0.86f);
    g.setFont (font);
    g.setColour (colours::ember.withAlpha (0.08f));
    g.drawText ("888 BPM", main, juce::Justification::centredBottom, false);   // unlit segments
    g.setColour (colours::ember.withAlpha (0.35f));
    g.drawText (bpm, main.translated (0.0f, 0.5f).expanded (1.0f, 0.0f), juce::Justification::centredBottom, false);
    g.setColour (colours::ember.brighter (0.2f));
    g.drawText (bpm, main, juce::Justification::centredBottom, false);

    g.setColour (manual ? colours::emberHot : colours::ember.withAlpha (0.85f));
    g.setFont (plateFont (6.8f, 0.1f));
    g.drawText (manual ? "SET" : "AUTO", glass, juce::Justification::centred, false);

    // SYNC LED: blinks on every beat while the DAW plays
    const auto beat = proc.getPlayheadBeat();
    const auto on = proc.isHostPlaying() && beat >= 0.0 && beat - std::floor (beat) < 0.2;
    drawLed (g, { glass.getRight() - 4.0f, glass.getCentreY() }, 1.8f, on ? 1.0f : 0.0f);
}

void BpmLcd::mouseDown (const juce::MouseEvent&)
{
    startBpm = proc.getEffectiveBpm();
}

void BpmLcd::mouseDrag (const juce::MouseEvent& e)
{
    proc.setTargetBpm (std::round (startBpm - e.getDistanceFromDragStartY() * 0.5));
    repaint();
}

void BpmLcd::mouseDoubleClick (const juce::MouseEvent&)
{
    proc.setTargetBpm (0.0);
    repaint();
}

//==============================================================================
juce::Rectangle<float> HistoryLeds::dot (int i) const
{
    const auto step = (float) getWidth() / (float) RollsKillaProcessor::kMaxKillHistory;
    const auto d = juce::jmin (step * 0.62f, (float) getHeight() - 3.0f);
    return juce::Rectangle<float> (d, d).withCentre ({ step * ((float) i + 0.5f), (float) getHeight() * 0.5f });
}

void HistoryLeds::paint (juce::Graphics& g)
{
    const auto count = (int) proc.getKillHistory().size();
    for (int i = 0; i < RollsKillaProcessor::kMaxKillHistory; ++i)
    {
        const auto r = dot (i);
        const auto current = i == proc.getKillHistoryPosition() && i < count;
        drawLed (g, r.getCentre(), r.getWidth() * 0.5f, current ? 1.0f : (i == hover && i < count ? 0.35f : 0.0f));
        if (i < count && ! current && i != hover)
        {
            g.setColour (colours::ember.withAlpha (0.18f));
            g.fillEllipse (r.reduced (r.getWidth() * 0.3f));
        }
    }
}

void HistoryLeds::mouseMove (const juce::MouseEvent& e)
{
    const auto& h = proc.getKillHistory();
    int found = -1;
    for (int i = 0; i < (int) h.size(); ++i)
        if (dot (i).expanded (3.0f).contains (e.position))
            found = i;
    if (found != hover)
    {
        hover = found;
        setTooltip (found >= 0 ? h[(size_t) found].label : juce::String ("The last KILLs - click a light to go back to that roll"));
        setMouseCursor (found >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void HistoryLeds::mouseExit (const juce::MouseEvent&)
{
    hover = -1;
    repaint();
}

void HistoryLeds::mouseUp (const juce::MouseEvent&)
{
    if (hover >= 0)
    {
        proc.restoreKillHistory (hover);
        repaint();
    }
}

//==============================================================================
BluntSlider::BluntSlider (juce::AudioProcessorValueTreeState& state)
    : param (*state.getParameter (params::puff)),
      attachment (param, [this] (float v) { value = v / 100.0f; repaint(); }, state.undoManager)
{
    setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    attachment.sendInitialUpdate();
}

juce::Rectangle<float> BluntSlider::track() const
{
    return { 44.0f, 4.0f, (float) getWidth() - 88.0f, 10.0f };
}

juce::Rectangle<float> BluntSlider::body() const
{
    const auto top = 21.0f;
    return { 8.0f, top, (float) getWidth() - 18.0f, juce::jmin (32.0f, (float) getHeight() - top - 2.0f) };
}

float BluntSlider::handleX() const
{
    const auto t = track();
    return t.getX() + value * t.getWidth();
}

float BluntSlider::valueAt (juce::Point<float> p) const
{
    // on the track: follow the track; on the blunt: the ring follows the mouse
    const auto t = track();
    if (p.y < body().getY() - 2.0f)
        return juce::jlimit (0.0f, 1.0f, (p.x - t.getX()) / t.getWidth());
    const auto b = body();
    return juce::jlimit (0.0f, 1.0f, (p.x - (b.getX() + 30.0f)) / (b.getWidth() - 54.0f));
}

juce::Point<float> BluntSlider::getEmberTip() const
{
    const auto b = body();
    return { b.getRight() - 3.0f, b.getCentreY() - b.getHeight() * 0.15f };
}

void BluntSlider::setGlow (float g)
{
    if (std::abs (g - glow) > 0.01f)
    {
        glow = g;
        repaint (body().expanded (40.0f).withTrimmedLeft (body().getWidth() * 0.7f).toNearestInt());
    }
}

void BluntSlider::mouseDown (const juce::MouseEvent& e)
{
    attachment.beginGesture();
    attachment.setValueAsPartOfGesture (valueAt (e.position) * 100.0f);
}

void BluntSlider::mouseDrag (const juce::MouseEvent& e)
{
    attachment.setValueAsPartOfGesture (valueAt (e.position) * 100.0f);
}

void BluntSlider::mouseUp (const juce::MouseEvent&)
{
    attachment.endGesture();
}

void BluntSlider::mouseDoubleClick (const juce::MouseEvent&)
{
    attachment.setValueAsCompleteGesture (0.0f);
}

void BluntSlider::paintBlunt (juce::Graphics& g, juce::Rectangle<float> b, float)
{
    const auto cy = b.getCentreY();
    const auto hw = b.getHeight() * 0.5f;
    const auto x0 = b.getX() + 6.0f, x1 = b.getRight() - 6.0f;
    juce::Random rng (3);

    // radius along the blunt: a twisted slim tail on the left, fat and round on the right
    auto radius = [&] (float x)
    {
        const auto t = juce::jlimit (0.0f, 1.0f, (x - x0) / (x1 - x0));
        const auto tail = juce::jmin (1.0f, t * 7.0f);
        return hw * (0.34f + 0.52f * std::sqrt (tail) + 0.12f * t);
    };

    juce::Path body;
    constexpr int steps = 60;
    std::array<float, steps + 1> wobbleTop {}, wobbleBottom {};
    for (int i = 0; i <= steps; ++i)
    {
        wobbleTop[(size_t) i] = (rng.nextFloat() - 0.5f) * 1.1f;
        wobbleBottom[(size_t) i] = (rng.nextFloat() - 0.5f) * 0.9f;
    }
    for (int i = 0; i <= steps; ++i)
    {
        const auto x = x0 + (x1 - x0) * (float) i / steps;
        const auto y = cy - radius (x) + wobbleTop[(size_t) i];
        i == 0 ? body.startNewSubPath (x, y) : body.lineTo (x, y);
    }
    body.quadraticTo (x1 + 3.5f, cy, x1, cy + radius (x1));
    for (int i = steps; i >= 0; --i)
    {
        const auto x = x0 + (x1 - x0) * (float) i / steps;
        body.lineTo (x, cy + radius (x) + wobbleBottom[(size_t) i]);
    }
    body.closeSubPath();
    bodyPath = body;

    // crumpled twisted tip on the left
    juce::Path tip;
    tip.startNewSubPath (x0 + 1.0f, cy - radius (x0) * 0.9f);
    tip.lineTo (x0 - 5.0f, cy - 2.5f);
    tip.lineTo (x0 - 7.5f, cy + 0.5f);
    tip.lineTo (x0 - 4.0f, cy + 2.8f);
    tip.lineTo (x0 + 1.0f, cy + radius (x0) * 0.9f);
    tip.closeSubPath();

    // shadow on the plate
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillPath (body, juce::AffineTransform::translation (1.5f, hw * 0.38f));
    g.fillPath (tip, juce::AffineTransform::translation (1.5f, hw * 0.3f));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6a4428), 0.0f, cy - hw, juce::Colour (0xff1c0f07), 0.0f, cy + hw, false));
    g.fillPath (tip);
    g.setColour (juce::Colour (0xff140a04).withAlpha (0.6f));
    g.drawLine (x0 - 6.0f, cy + 0.5f, x0 + 1.0f, cy - 3.0f, 0.6f);
    g.drawLine (x0 - 4.0f, cy + 2.0f, x0 + 1.0f, cy + 3.5f, 0.6f);

    juce::Graphics::ScopedSaveState save (g);
    g.reduceClipRegion (body);
    const auto area = b.expanded (4.0f);

    juce::ColourGradient leaf (juce::Colour (0xff6b4527), 0.0f, cy - hw, juce::Colour (0xff1a0d05), 0.0f, cy + hw, false);
    leaf.addColour (0.18, juce::Colour (0xffa0703f));
    leaf.addColour (0.42, juce::Colour (0xff774a27));
    leaf.addColour (0.75, juce::Colour (0xff3c2211));
    g.setGradientFill (leaf);
    g.fillRect (area);

    // blotchy leaf colour
    for (int i = 0; i < 26; ++i)
    {
        const auto c = juce::Point<float> (x0 + rng.nextFloat() * (x1 - x0), cy + (rng.nextFloat() - 0.5f) * hw * 1.6f);
        const auto r = 3.0f + rng.nextFloat() * 9.0f;
        const auto col = rng.nextBool() ? juce::Colour (0xff2a1608) : juce::Colour (0xffb98a55);
        g.setGradientFill (juce::ColourGradient (col.withAlpha (0.22f), c, col.withAlpha (0.0f), c.translated (r, 0.0f), true));
        g.fillEllipse (juce::Rectangle<float> (r * 2.4f, r * 1.4f).withCentre (c));
    }

    // spiral seams of the wrap (uneven)
    for (float x = x0 + 10.0f; x < x1 + 10.0f; x += 26.0f + rng.nextFloat() * 16.0f)
    {
        const auto r = radius (x);
        const auto lean = r * (0.9f + rng.nextFloat() * 0.5f);
        juce::Path seam;
        seam.startNewSubPath (x - lean, cy + r);
        seam.cubicTo (x - lean * 0.5f, cy + r * 0.3f, x - lean * 0.1f + rng.nextFloat() * 2.0f, cy - r * 0.2f, x + lean * 0.45f, cy - r);
        g.setColour (juce::Colour (0xff120802).withAlpha (0.7f));
        g.strokePath (seam, juce::PathStrokeType (1.1f));
        g.setColour (juce::Colour (0xffd6a46e).withAlpha (0.3f));
        g.strokePath (seam, juce::PathStrokeType (0.6f), juce::AffineTransform::translation (1.2f, -0.2f));
    }

    // wrinkles: light ridges on top, dark folds below
    for (int i = 0; i < 150; ++i)
    {
        const auto x = x0 + rng.nextFloat() * (x1 - x0);
        const auto r = radius (x);
        const auto v = (rng.nextFloat() - 0.5f) * 1.8f;
        const auto y = cy + v * r;
        const auto len = 1.5f + rng.nextFloat() * 5.0f;
        const auto light = v < 0.1f && rng.nextFloat() < 0.65f;
        g.setColour ((light ? juce::Colour (0xffe0b27c) : juce::Colour (0xff0e0602)).withAlpha (0.1f + rng.nextFloat() * 0.2f));
        juce::Path w;
        w.startNewSubPath (x, y);
        w.quadraticTo (x + len * 0.5f, y - len * (0.2f + rng.nextFloat() * 0.4f), x + len, y - len * 0.1f);
        g.strokePath (w, juce::PathStrokeType (0.45f + rng.nextFloat() * 0.35f));
    }

    // long leaf veins
    for (int i = 0; i < 7; ++i)
    {
        const auto x = x0 + 12.0f + rng.nextFloat() * (x1 - x0 - 20.0f);
        const auto r = radius (x);
        g.setColour (juce::Colour (0xffc99a64).withAlpha (0.16f));
        g.drawLine (x, cy + r * 0.8f, x + r * 1.6f, cy - r * 0.7f, 0.5f);
    }

    // cylinder shading + glossy top edge
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.0f), 0.0f, cy - hw * 0.1f,
                                             juce::Colours::black.withAlpha (0.5f), 0.0f, cy + hw, false));
    g.fillRect (area);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.14f), 0.0f, cy - hw * 0.75f,
                                             juce::Colours::white.withAlpha (0.0f), 0.0f, cy - hw * 0.35f, false));
    g.fillRect (area);

    // charred band next to the ember
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0b0604).withAlpha (0.0f), x1 - 24.0f, cy,
                                             juce::Colour (0xff0b0604).withAlpha (0.95f), x1 - 6.0f, cy, false));
    g.fillRect (juce::Rectangle<float> (x1 - 24.0f, cy - hw - 2.0f, 30.0f, hw * 2.0f + 4.0f));
}

void BluntSlider::paint (juce::Graphics& g)
{
    const auto b = body();
    const auto t = track();

    // PUFF 0 ------ MAX
    drawEngravedText (g, "PUFF", plateFont (10.0f, 0.1f), { 0.0f, 0.0f, t.getX() - 12.0f, 16.0f }, juce::Justification::centredLeft);
    drawEngravedText (g, "0", plateFont (8.5f, 0.0f), { t.getX() - 12.0f, 0.0f, 10.0f, 16.0f }, juce::Justification::centredRight);
    drawEngravedText (g, "MAX", plateFont (8.5f, 0.05f), { t.getRight() + 3.0f, 0.0f, 30.0f, 16.0f }, juce::Justification::centredLeft);
    auto line = juce::Rectangle<float> (t.getX(), 8.0f, t.getWidth(), 2.4f);
    drawRecess (g, line, 1.2f);
    g.setColour (colours::ember.withAlpha (0.85f));
    g.fillRoundedRectangle (line.withWidth (value * line.getWidth()).reduced (0.0f, 0.5f), 1.0f);
    for (int i = 0; i <= 10; ++i)
    {
        const auto x = t.getX() + t.getWidth() * (float) i / 10.0f;
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRect (juce::Rectangle<float> (x - 0.4f, 12.5f, 0.8f, i % 5 == 0 ? 4.0f : 2.5f));
    }

    // the blunt (cached - redrawn only when the scale changes)
    const auto scale = (float) g.getInternalContext().getPhysicalPixelScaleFactor();
    if (bodyCache.isNull() || std::abs (cacheScale - scale) > 0.01f)
    {
        cacheScale = scale;
        bodyCache = juce::Image (juce::Image::ARGB, juce::roundToInt ((float) getWidth() * scale), juce::roundToInt ((float) getHeight() * scale), true);
        juce::Graphics cg (bodyCache);
        cg.addTransform (juce::AffineTransform::scale (scale));
        paintBlunt (cg, b, 0.0f);
    }
    g.drawImage (bodyCache, getLocalBounds().toFloat());

    // ember: grey ash when unlit, a glowing coal with the PUFF amount and the beat
    const auto heat = value > 0.0f ? juce::jlimit (0.0f, 1.0f, 0.45f + 0.55f * juce::jmax (glow, value * 0.6f)) : 0.0f;
    {
        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (bodyPath);
        const auto x1 = b.getRight() - 6.0f;
        const auto top = b.getY() - 2.0f, bottom = b.getBottom() + 2.0f;

        // jagged burn line
        juce::Random rng (9);
        juce::Path burnt;
        burnt.startNewSubPath (x1 + 6.0f, top);
        auto burnX = [&] (float y) { juce::ignoreUnused (y); return x1 - 9.0f - rng.nextFloat() * 4.0f; };
        juce::Path edge;
        for (float y = top; y <= bottom; y += 2.2f)
        {
            const auto x = burnX (y);
            burnt.lineTo (x, y);
            y <= top ? edge.startNewSubPath (x, y) : edge.lineTo (x, y);
        }
        burnt.lineTo (x1 + 6.0f, bottom);
        burnt.closeSubPath();

        if (heat > 0.0f)
        {
            juce::ColourGradient coal (colours::emberDeep.withAlpha (1.0f), x1 - 12.0f, b.getCentreY(),
                                       colours::emberHot, x1 + 1.0f, b.getCentreY() - 2.0f, false);
            coal.addColour (0.45, colours::ember.interpolatedWith (colours::emberDeep, 1.0f - heat));
            g.setGradientFill (coal);
            g.fillPath (burnt);
            // hot spots pulsing with the pull
            for (int i = 0; i < 9; ++i)
            {
                const auto p = juce::Point<float> (x1 - 7.0f + rng.nextFloat() * 8.0f, b.getY() + b.getHeight() * rng.nextFloat());
                g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffff0c0).withAlpha (0.8f * heat), p,
                                                         juce::Colour (0xfffff0c0).withAlpha (0.0f), p.translated (2.5f + 2.0f * heat, 0.0f), true));
                g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre (p));
            }
            g.setColour (colours::ember.withAlpha (0.9f * heat));
            g.strokePath (edge, juce::PathStrokeType (1.0f));
            g.setColour (colours::ember.withAlpha (0.25f * heat));
            g.strokePath (edge, juce::PathStrokeType (3.0f));
        }
        else
        {
            g.setColour (juce::Colour (0xff2a2725));
            g.fillPath (burnt);
        }

        // ash cap on the very tip
        auto ash = juce::Rectangle<float> (x1 - 2.0f, top, 10.0f, bottom - top);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb5afa8).withAlpha (heat > 0.0f ? 0.55f : 0.85f), ash.getX(), ash.getY(),
                                                 juce::Colour (0xff4a4543).withAlpha (heat > 0.0f ? 0.55f : 0.9f), ash.getRight(), ash.getBottom(), false));
        g.fillRect (ash);
        for (int i = 0; i < 26; ++i)
        {
            g.setColour ((rng.nextBool() ? juce::Colour (0xffe2ddd6) : juce::Colour (0xff2a2624)).withAlpha (0.7f));
            g.fillEllipse (juce::Rectangle<float> (1.1f, 1.1f).withCentre ({ x1 - 4.0f + rng.nextFloat() * 10.0f,
                                                                             b.getY() + b.getHeight() * rng.nextFloat() }));
        }
    }

    // steel ring = slider handle
    const auto hx = juce::jlimit (b.getX() + 30.0f, b.getRight() - 24.0f, b.getX() + 30.0f + value * (b.getWidth() - 54.0f));
    auto ring = juce::Rectangle<float> (13.0f, b.getHeight() + 6.0f).withCentre ({ hx, b.getCentreY() });
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (ring.translated (1.2f, 2.0f), 2.5f);
    juce::ColourGradient steel (juce::Colour (0xff5a5a56), ring.getX(), 0.0f, juce::Colour (0xff4a4a47), ring.getRight(), 0.0f, false);
    steel.addColour (0.3, juce::Colour (0xffdadad4));
    steel.addColour (0.55, juce::Colour (0xff9a9a95));
    g.setGradientFill (steel);
    g.fillRoundedRectangle (ring, 2.5f);
    for (float gx = ring.getX() + 3.5f; gx < ring.getRight() - 2.0f; gx += 3.0f)
    {
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRect (juce::Rectangle<float> (gx, ring.getY() + 2.0f, 0.7f, ring.getHeight() - 4.0f));
    }
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawRoundedRectangle (ring, 2.5f, 0.8f);

    // knob on the track follows the ring
    const auto kx = handleX();
    g.setColour (colours::emberHot);
    g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre ({ kx, 9.2f }));
}

//==============================================================================
SmokeOverlay::SmokeOverlay()
{
    setInterceptsMouseClicks (false, false);
}

void SmokeOverlay::tick (float dt, float amount, float glow, juce::Point<float> origin)
{
    glowAt = origin.translated (-3.0f, 3.0f);
    glowAmount = amount > 0.0f ? juce::jlimit (0.0f, 1.0f, 0.45f + 0.55f * juce::jmax (glow, amount * 0.6f)) : 0.0f;
    for (int i = 0; i < count;)
    {
        auto& p = puffs[(size_t) i];
        p.age += dt;
        if (p.age >= p.life)
        {
            p = puffs[(size_t) --count];
            continue;
        }
        p.phase += dt * 2.4f;
        p.x += (p.vx + std::sin (p.phase) * 5.0f) * dt;
        p.y += p.vy * dt;
        p.vy *= 0.995f;
        ++i;
    }

    if (amount > 0.0f)
    {
        spawnAccumulator += dt * (2.0f + 14.0f * amount * (0.4f + glow));
        while (spawnAccumulator >= 1.0f && count < (int) puffs.size())
        {
            spawnAccumulator -= 1.0f;
            puffs[(size_t) count++] = { origin.x + rng.nextFloat() * 2.0f, origin.y - 2.0f,
                                        -2.0f + rng.nextFloat() * 5.0f, -(10.0f + rng.nextFloat() * 10.0f) * (0.7f + amount),
                                        0.0f, 2.0f + rng.nextFloat() * 1.6f, 2.0f + rng.nextFloat() * 2.0f, rng.nextFloat() * 6.28f };
        }
    }
    else
    {
        spawnAccumulator = 0.0f;
    }
    repaint();
}

void SmokeOverlay::paint (juce::Graphics& g)
{
    if (glowAmount > 0.0f)
    {
        g.setGradientFill (juce::ColourGradient (colours::ember.withAlpha (0.55f * glowAmount), glowAt,
                                                 colours::ember.withAlpha (0.0f), glowAt.translated (22.0f + 16.0f * glowAmount, 0.0f), true));
        g.fillEllipse (juce::Rectangle<float> (84.0f, 84.0f).withCentre (glowAt));
    }
    for (int i = 0; i < count; ++i)
    {
        const auto& p = puffs[(size_t) i];
        const auto t = p.age / p.life;
        const auto size = p.size + t * 18.0f;
        const auto alpha = 0.4f * (1.0f - t) * juce::jmin (1.0f, t * 6.0f) * juce::jlimit (0.0f, 1.0f, (p.y - 2.0f) / 30.0f);
        const auto c = juce::Point<float> (p.x, p.y);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe8e4e0).withAlpha (alpha), c,
                                                 juce::Colour (0xffe8e4e0).withAlpha (0.0f), c.translated (size, 0.0f), true));
        g.fillEllipse (juce::Rectangle<float> (size * 2.0f, size * 2.0f).withCentre (c));
    }
}

} // namespace rk::ui::metal
