#include "SkinWidgets.h"

#include "../PluginProcessor.h"

#include <MiniSkin.h>

namespace rk::ui::skin
{

namespace
{
    // geometry of the design picture (keep in sync with tools/mini_skin/make_skin.py)
    constexpr float kLamps[] { 1632.0f, 1712.0f, 1792.0f, 1873.0f };
    constexpr float kLampY = 154.0f, kLampR = 30.0f;
    constexpr float kDotX0 = 868.0f, kDotStep = 41.0f, kDotY = 378.0f, kDotR = 15.0f;
    constexpr float kMood[] { 117.0f, 211.0f, 307.0f };
    constexpr float kMoodY = 428.0f, kMoodR = 13.0f;
    constexpr float kTrackX0 = 883.0f, kTrackX1 = 1390.0f, kTrackY = 432.0f;
    constexpr float kBluntX0 = 748.0f, kBluntX1 = 1560.0f, kBluntY0 = 450.0f, kBluntY1 = 578.0f;
    constexpr float kRingX0 = 830.0f, kRingX1 = 1420.0f;      // ring centre travel
    constexpr float kRingW = 60.0f, kRingY0 = 444.0f, kRingY1 = 586.0f;
    constexpr float kEmberX = 1494.0f, kEmberY = 524.0f;

    juce::Image load (const void* data, int size)
    {
        return juce::ImageFileFormat::loadFrom (data, (size_t) size);   // no global ImageCache copy
    }

    void drawSprite (juce::Graphics& g, const juce::Image& img, juce::Rectangle<float> r)
    {
        g.drawImage (img, r, juce::RectanglePlacement::stretchToFit);
    }
}

/*  The skin images live only while a Mini window is open (the editor holds a SharedImages).
    Never keep juce::Images in a static: on Windows they would be freed while the DLL unloads,
    after the graphics system (Direct2D) is gone - that can freeze the host when it quits. */
SharedImages::SharedImages()
{
    auto& i = images_;
    i.plate      = load (MiniSkin::plate_jpg, MiniSkin::plate_jpgSize);
    i.ring       = load (MiniSkin::ring_png, MiniSkin::ring_pngSize);
    i.lampOn     = load (MiniSkin::lamp_on_png, MiniSkin::lamp_on_pngSize);
    i.lampOff    = load (MiniSkin::lamp_off_png, MiniSkin::lamp_off_pngSize);
    i.dotOn      = load (MiniSkin::dot_on_png, MiniSkin::dot_on_pngSize);
    i.dotOff     = load (MiniSkin::dot_off_png, MiniSkin::dot_off_pngSize);
    i.moodOn     = load (MiniSkin::mood_on_png, MiniSkin::mood_on_pngSize);
    i.moodOff    = load (MiniSkin::mood_off_png, MiniSkin::mood_off_pngSize);
    i.toggleMidi = load (MiniSkin::toggle_midi_png, MiniSkin::toggle_midi_pngSize);
    i.toggleHost = load (MiniSkin::toggle_host_png, MiniSkin::toggle_host_pngSize);
}

const Images& images()
{
    // the editor keeps the shared instance alive, so this finds the loaded one (no reload)
    const juce::SharedResourcePointer<SharedImages> shared;
    return shared->get();
}

void drawPressOverlay (juce::Graphics& g, juce::Rectangle<float> r, float corner, bool highlighted, bool down, bool enabled)
{
    if (! enabled)
    {
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (r, corner);
        return;
    }
    if (down)
    {
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (r, corner);
    }
    else if (highlighted)
    {
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillRoundedRectangle (r, corner);
    }
}

//==============================================================================
SkinButton::SkinButton (const juce::String& name, float cornerPx) : juce::Button (name), corner (cornerPx)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void SkinButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    drawPressOverlay (g, getLocalBounds().toFloat().reduced (1.0f), corner, highlighted, down, isEnabled());
}

void SkinKillButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat();
    auto cap = r.reduced (r.getWidth() * 0.1f, r.getHeight() * 0.14f);
    if (highlighted && ! down)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff3a1a).withAlpha (0.22f), cap.getCentre(),
                                                 juce::Colour (0xffff3a1a).withAlpha (0.0f), cap.getCentre().translated (cap.getWidth() * 0.6f, 0.0f), true));
        g.fillEllipse (cap.expanded (6.0f));
    }
    if (down)
    {
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.fillRoundedRectangle (cap, cap.getHeight() * 0.3f);
    }
}

void SkinDragButton::paint (juce::Graphics& g)
{
    drawPressOverlay (g, getLocalBounds().toFloat().reduced (1.0f), 3.0f, isMouseOver(), dragging);
}

void SkinPlayButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat();
    const auto c = r.getCentre();
    const auto face = juce::Rectangle<float> (r.getWidth() * 0.6f, r.getWidth() * 0.6f).withCentre (c);

    if (previewing || hostPlaying)
    {
        // cover the orange triangle with the black face and draw the state
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff201e1c), face.getX(), face.getY(),
                                                 juce::Colour (0xff050404), face.getX(), face.getBottom(), false));
        g.fillEllipse (face.reduced (1.0f));
        const auto s = face.getWidth() * 0.34f;
        const auto col = hostPlaying ? metal::colours::ember.withAlpha (0.45f) : metal::colours::ember;
        g.setGradientFill (juce::ColourGradient (col.withAlpha (col.getFloatAlpha() * 0.5f), c, col.withAlpha (0.0f), c.translated (s * 1.3f, 0.0f), true));
        g.fillEllipse (face);
        g.setColour (col);
        if (previewing)
            g.fillRoundedRectangle (juce::Rectangle<float> (s, s).withCentre (c), 1.5f);
        else
        {
            g.setFont (metal::plateFont (7.0f, 0.1f));
            g.drawText ("SYNC", face, juce::Justification::centred, false);
        }
    }
    else if (highlighted)
    {
        g.setGradientFill (juce::ColourGradient (metal::colours::ember.withAlpha (0.25f), c, metal::colours::ember.withAlpha (0.0f),
                                                 c.translated (face.getWidth() * 0.5f, 0.0f), true));
        g.fillEllipse (face);
    }
    if (down)
    {
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.fillEllipse (face.expanded (face.getWidth() * 0.18f));
    }
}

//==============================================================================
void SkinBpm::paint (juce::Graphics& g)
{
    auto glass = getLocalBounds().toFloat();
    const auto manual = proc.getTargetBpm() > 0.0;
    const auto bpm = juce::String (juce::roundToInt (proc.getEffectiveBpm())) + " BPM";
    auto main = glass.removeFromTop (glass.getHeight() * 0.64f);
    const auto font = juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), main.getHeight() * 0.8f, juce::Font::plain))
                          .withHorizontalScale (0.9f);
    g.setFont (font);
    g.setColour (metal::colours::ember.withAlpha (0.35f));
    g.drawText (bpm, main.translated (0.0f, 0.4f).expanded (1.0f, 0.0f), juce::Justification::centredBottom, false);
    g.setColour (juce::Colour (0xffff6a2a));
    g.drawText (bpm, main, juce::Justification::centredBottom, false);

    g.setColour (manual ? metal::colours::emberHot : juce::Colour (0xffff6a2a).withAlpha (0.9f));
    g.setFont (metal::plateFont (glass.getHeight() * 0.75f, 0.1f));
    g.drawText (manual ? "SET" : "AUTO", glass, juce::Justification::centredTop, false);

    const auto beat = proc.getPlayheadBeat();
    const auto on = proc.isHostPlaying() && beat >= 0.0 && beat - std::floor (beat) < 0.2;
    if (on)
        metal::drawLed (g, { (float) getWidth() - 5.0f, glass.getCentreY() - 1.0f }, 1.6f, 1.0f);
}

void SkinBars::paint (juce::Graphics& g)
{
    const auto& img = images();
    for (int i = 0; i < 4; ++i)
    {
        const auto r = design (kLamps[i] - kLampR, kLampY - kLampR, kLamps[i] + kLampR, kLampY + kLampR)
                           .translated ((float) -getX(), (float) -getY());
        if (i == index)
            drawSprite (g, img.lampOn, r);
    }
}

juce::Rectangle<float> SkinHistory::dot (int i) const
{
    const auto x = kDotX0 + kDotStep * (float) i;
    return design (x - kDotR, kDotY - kDotR, x + kDotR, kDotY + kDotR).translated ((float) -getX(), (float) -getY());
}

void SkinHistory::paint (juce::Graphics& g)
{
    const auto count = (int) proc.getKillHistory().size();
    for (int i = 0; i < count; ++i)
    {
        const auto r = dot (i);
        if (i == proc.getKillHistoryPosition())
            metal::drawLed (g, r.getCentre(), r.getWidth() * 0.34f, 1.0f);
        else
        {
            // a filled slot: faint ember inside the dark lamp
            g.setColour (metal::colours::ember.withAlpha (i == hover ? 0.55f : 0.22f));
            g.fillEllipse (r.reduced (r.getWidth() * 0.3f));
        }
    }
}

float SkinMood::slotX (int i) const
{
    return kMood[i] * kPx - (float) getX();
}

void SkinMood::paint (juce::Graphics& g)
{
    const auto x = kMood[mood];
    drawSprite (g, images().moodOn, design (x - kMoodR, kMoodY - kMoodR, x + kMoodR, kMoodY + kMoodR).translated ((float) -getX(), (float) -getY()));
}

SkinHat::SkinHat (juce::AudioProcessorValueTreeState& state, std::function<juce::StringArray()> names)
    : HatPicker (state, std::move (names))
{
    prev.setVisible (false);
    next.setVisible (false);
    wave.bare = true;
    addAndMakeVisible (skinPrev);
    addAndMakeVisible (skinNext);
    skinPrev.onClick = [this] { step (-1); };
    skinNext.onClick = [this] { step (1); };
    skinPrev.setTooltip ("Previous hi-hat");
    skinNext.setTooltip ("Next hi-hat");
}

void SkinHat::resized()
{
    const auto o = juce::Point<float> ((float) -getX(), (float) -getY());
    skinPrev.setBounds (design (386, 490, 428, 538).translated (o.x, o.y).toNearestInt());
    skinNext.setBounds (design (645, 490, 688, 538).translated (o.x, o.y).toNearestInt());
    wave.setBounds (design (444, 460, 630, 552).translated (o.x, o.y).toNearestInt());
}

juce::Rectangle<int> SkinHat::namePlate() const
{
    return design (440, 572, 634, 603).translated ((float) -getX(), (float) -getY()).toNearestInt();
}

void SkinHat::paint (juce::Graphics& g)
{
    const auto r = namePlate().toFloat();
    const auto font = metal::plateFont (r.getHeight() * 0.78f, 0.03f);
    const auto name = getNames()[index].toUpperCase();
    g.setFont (font);
    g.setColour (juce::Colours::white.withAlpha (0.3f));
    g.drawText (name, r.translated (0.0f, 0.6f), juce::Justification::centred, true);
    g.setColour (juce::Colour (0xff161615));
    g.drawText (name, r, juce::Justification::centred, true);
}

void SkinToggle::paint (juce::Graphics& g)
{
    drawSprite (g, mode == 0 ? images().toggleMidi : images().toggleHost, getLocalBounds().toFloat());
}

//==============================================================================
juce::Rectangle<float> SkinBlunt::body() const
{
    return local (design (kBluntX0, kBluntY0, kBluntX1, kBluntY1));
}

juce::Rectangle<float> SkinBlunt::track() const
{
    return local (design (kTrackX0, kTrackY - 10.0f, kTrackX1, kTrackY + 10.0f));
}

juce::Point<float> SkinBlunt::getEmberTip() const
{
    return designPoint (kEmberX + 10.0f, kEmberY - 34.0f) - getPosition().toFloat();
}

float SkinBlunt::ringCentreX() const
{
    return (kRingX0 + value * (kRingX1 - kRingX0)) * kPx - (float) getX();
}

float SkinBlunt::valueAt (juce::Point<float> p) const
{
    if (p.y < body().getY())
    {
        const auto t = track();
        return juce::jlimit (0.0f, 1.0f, (p.x - t.getX()) / t.getWidth());
    }
    const auto x0 = kRingX0 * kPx - (float) getX(), x1 = kRingX1 * kPx - (float) getX();
    return juce::jlimit (0.0f, 1.0f, (p.x - x0) / (x1 - x0));
}

void SkinBlunt::paint (juce::Graphics& g)
{
    // track: orange fill + knob
    const auto t = track();
    const auto line = juce::Rectangle<float> (t.getX(), t.getCentreY() - 0.8f, t.getWidth() * value, 1.6f);
    g.setColour (metal::colours::ember.withAlpha (0.9f));
    g.fillRect (line);
    g.setGradientFill (juce::ColourGradient (metal::colours::emberHot, t.getX() + t.getWidth() * value, t.getCentreY(),
                                             metal::colours::ember.withAlpha (0.0f), t.getX() + t.getWidth() * value + 6.0f, t.getCentreY(), true));
    g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre ({ t.getX() + t.getWidth() * value, t.getCentreY() }));
    g.setColour (metal::colours::emberHot);
    g.fillEllipse (juce::Rectangle<float> (3.6f, 3.6f).withCentre ({ t.getX() + t.getWidth() * value, t.getCentreY() }));

    // ember: dim when PUFF is 0, pulsing with the pull
    const auto ember = designPoint (kEmberX, kEmberY) - getPosition().toFloat();
    const auto heat = value > 0.0f ? juce::jlimit (0.0f, 1.0f, 0.3f + 0.7f * juce::jmax (glow, value * 0.5f)) : 0.0f;
    if (heat > 0.0f)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffa040).withAlpha (0.45f * heat), ember,
                                                 juce::Colour (0xffff5010).withAlpha (0.0f), ember.translated (10.0f + 10.0f * heat, 0.0f), true));
        g.fillEllipse (juce::Rectangle<float> (48.0f, 48.0f).withCentre (ember));
    }
    else
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2624).withAlpha (0.85f), ember,
                                                 juce::Colour (0xff2a2624).withAlpha (0.0f), ember.translated (12.0f, 0.0f), true));
        g.fillEllipse (juce::Rectangle<float> (26.0f, 34.0f).withCentre (ember));
    }

    // the steel ring (slider handle)
    const auto ring = juce::Rectangle<float> (kRingW * kPx, (kRingY1 - kRingY0) * kPx)
                          .withCentre ({ ringCentreX(), (kRingY0 + kRingY1) * 0.5f * kPx - (float) getY() });
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (ring.translated (1.5f, 1.8f), 2.0f);
    g.drawImage (images().ring, ring, juce::RectanglePlacement::stretchToFit);
}

} // namespace rk::ui::skin
