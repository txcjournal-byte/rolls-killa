#include "KitWidgets.h"

#include "../PluginProcessor.h"

namespace rk::ui::station
{

using metal::colours::ember;
using metal::colours::emberHot;
using metal::colours::emberDeep;

namespace
{
    const juce::Colour kText { 0xffd9d4ce };
    const juce::Colour kDim { 0xff8c8680 };
    const juce::Colour kBlood { 0xffe0241c };

    juce::Font bold (float h, float kerning = 0.06f) { return metal::plateFont (h, kerning); }
}

juce::Colour drawerColour (int type)
{
    static const juce::Colour c[] { juce::Colour (0xffff5a1f), juce::Colour (0xffff2e3e), juce::Colour (0xffffb02e), juce::Colour (0xffff4fa3),
                                    juce::Colour (0xffffd23f), juce::Colour (0xffffe9a0), juce::Colour (0xff7dff6a), juce::Colour (0xffc45cff) };
    return c[juce::jlimit (0, kNumDrumTypes - 1, type)];
}

void drawStamped (juce::Graphics& g, const juce::String& text, juce::Font font, juce::Rectangle<float> r, juce::Justification just, juce::Colour colour)
{
    g.setFont (font);
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.drawText (text, r.translated (0.0f, 1.0f), just, true);
    g.setColour (colour);
    g.drawText (text, r, just, true);
}

juce::Image renderDarkSteel (int width, int height, float scale)
{
    auto img = metal::renderSteel (width, height, scale);
    juce::Graphics g (img);
    g.addTransform (juce::AffineTransform::scale (scale));
    g.setColour (juce::Colour (0xff0b0a0a).withAlpha (0.74f));
    g.fillAll();
    // blood rust in the corners, a hot spot behind the logo
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb3121f).withAlpha (0.16f), 140.0f, 30.0f,
                                             juce::Colours::transparentBlack, 520.0f, 30.0f, true));
    g.fillRect (0, 0, width, 140);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6a1a08).withAlpha (0.18f), (float) width, (float) height,
                                             juce::Colours::transparentBlack, (float) width - 380.0f, (float) height - 200.0f, true));
    g.fillRect (0, 0, width, height);
    return img;
}

void drawBay (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.5f), 6.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1a1817), r.getX(), r.getY(), juce::Colour (0xff0e0d0d), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (r.expanded (0.5f), 6.5f, 1.0f);
    if (title.isNotEmpty())
    {
        auto t = r.reduced (10.0f, 6.0f).removeFromTop (14.0f);
        drawStamped (g, title, bold (11.0f, 0.22f), t, juce::Justification::centredLeft, kDim);
    }
}

//==============================================================================
MetalKnob::MetalKnob (const juce::String& l) : juce::Slider (RotaryHorizontalVerticalDrag, NoTextBox), label (l)
{
    setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    setPopupDisplayEnabled (true, true, nullptr);
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    setVelocityBasedMode (false);
}

void MetalKnob::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const auto labelArea = b.removeFromBottom (14.0f);
    const auto d = juce::jmin (b.getWidth(), b.getHeight()) - 4.0f;
    const auto c = b.getCentre();
    const auto r = d * 0.5f;
    const auto pos = (float) valueToProportionOfLength (getValue());
    const auto a0 = juce::MathConstants<float>::pi * 1.25f, a1 = a0 + juce::MathConstants<float>::pi * 1.5f * pos;
    const auto aEnd = juce::MathConstants<float>::pi * 2.75f;

    juce::Path track;
    track.addCentredArc (c.x, c.y, r, r, 0.0f, a0, aEnd, true);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path arc;
    arc.addCentredArc (c.x, c.y, r, r, 0.0f, a0, a1, true);
    g.setColour (accent.withAlpha (0.25f));
    g.strokePath (arc, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (arc, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // steel cap with grip ridges
    const auto cr = r * 0.72f;
    const auto cap = juce::Rectangle<float> (cr * 2.0f, cr * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (cap.translated (0.0f, 1.5f).expanded (1.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb8b4ae), cap.getX(), cap.getY(), juce::Colour (0xff2c2a28), cap.getRight(), cap.getBottom(), false));
    g.fillEllipse (cap);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff57534f), c.x, cap.getY(), juce::Colour (0xff23211f), c.x, cap.getBottom(), false));
    g.fillEllipse (cap.reduced (cr * 0.16f));
    for (int i = 0; i < 18; ++i)
    {
        const auto ang = juce::MathConstants<float>::twoPi * (float) i / 18.0f;
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawLine (c.x + std::cos (ang) * cr * 0.86f, c.y + std::sin (ang) * cr * 0.86f, c.x + std::cos (ang) * cr, c.y + std::sin (ang) * cr, 1.0f);
    }
    const auto ang = a1 - juce::MathConstants<float>::halfPi;
    g.setColour (emberHot);
    g.drawLine (c.x + std::cos (ang) * cr * 0.2f, c.y + std::sin (ang) * cr * 0.2f, c.x + std::cos (ang) * cr * 0.8f, c.y + std::sin (ang) * cr * 0.8f, 2.2f);

    drawStamped (g, label, bold (9.5f, 0.1f), labelArea, juce::Justification::centred, isEnabled() ? kText : kDim);
}

//==============================================================================
MetalChoice::MetalChoice (const juce::StringArray& l) : labels (l)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void MetalChoice::setLabels (const juce::StringArray& l)
{
    labels = l;
    repaint();
}

void MetalChoice::paint (juce::Graphics& g)
{
    const auto n = juce::jmax (1, labels.size());
    const auto w = (float) getWidth() / (float) n;
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<float> ((float) i * w, 0.0f, w, (float) getHeight()).reduced (1.5f, 1.0f);
        const auto on = i == selected;
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (r.translated (0.0f, 1.0f), 3.0f);
        g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xff3a2112) : juce::Colour (0xff383533), r.getX(), r.getY(),
                                                 on ? juce::Colour (0xff1c0f08) : juce::Colour (0xff1c1a19), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (on ? ember : juce::Colours::white.withAlpha (0.08f));
        g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, on ? 1.2f : 0.8f);
        if (on)
        {
            g.setColour (ember.withAlpha (0.18f));
            g.fillRoundedRectangle (r.reduced (1.0f), 3.0f);
        }
        drawStamped (g, labels[i], bold (juce::jmin (10.5f, r.getHeight() * 0.62f), 0.04f), r, juce::Justification::centred, on ? emberHot : kText);
    }
}

void MetalChoice::mouseUp (const juce::MouseEvent& e)
{
    if (! getLocalBounds().contains (e.getPosition()) || labels.isEmpty())
        return;
    const auto i = juce::jlimit (0, labels.size() - 1, (int) (e.position.x / ((float) getWidth() / (float) labels.size())));
    setSelected (i);
    if (onChange != nullptr)
        onChange (i);
}

ParamChoice::ParamChoice (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::StringArray& l)
    : MetalChoice (l),
      param (*state.getParameter (paramId)),
      attachment (param, [this] (float v) { setSelected (juce::roundToInt (v)); }, state.undoManager)
{
    onChange = [this] (int i) { attachment.setValueAsCompleteGesture ((float) i); };
    attachment.sendInitialUpdate();
}

//==============================================================================
StationButton::StationButton (const juce::String& buttonText, bool isRed) : juce::Button (buttonText), red (isRed)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void StationButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.2f), 4.0f);
    if (down)
        r = r.translated (0.0f, 0.8f);
    if (red)
    {
        if (highlighted)
            drawGlow (g, r, 4.0f, kBlood, 1.2f, 4);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe8391c).withMultipliedBrightness (highlighted ? 1.15f : 1.0f), r.getX(), r.getY(),
                                                 juce::Colour (0xff6e0f06), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (juce::Colour (0xffff8a60).withAlpha (0.5f));
        g.drawRoundedRectangle (r.reduced (0.6f), 4.0f, 0.8f);
        g.setFont (bold (juce::jmin (12.0f, r.getHeight() * 0.5f), 0.08f));
        g.setColour (juce::Colour (0xffff9a7a).withAlpha (0.4f));
        g.drawText (getButtonText(), r.translated (0.0f, 0.8f), juce::Justification::centred, true);
        g.setColour (juce::Colour (0xff200402));
        g.drawText (getButtonText(), r, juce::Justification::centred, true);
        return;
    }
    const auto top = lit ? juce::Colour (0xff4a2a14) : juce::Colour (0xff4a4744);
    g.setGradientFill (juce::ColourGradient (top.withMultipliedBrightness (highlighted ? 1.2f : 1.0f), r.getX(), r.getY(),
                                             juce::Colour (0xff1c1a19), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (lit ? ember : juce::Colours::white.withAlpha (highlighted ? 0.25f : 0.12f));
    g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, lit ? 1.3f : 0.8f);
    drawStamped (g, getButtonText(), bold (juce::jmin (11.5f, r.getHeight() * 0.5f), 0.08f), r, juce::Justification::centred,
                 ! isEnabled() ? kDim.withAlpha (0.5f) : lit ? emberHot : kText);
}

void KillBeatButton::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
    {
        juce::Button::mouseDown (e);
        return;
    }
    juce::PopupMenu m;
    m.addItem (1, "KILL BEAT - new patterns + a new hi-hat roll");
    m.addItem (2, "KILL KIT - new sounds in every drawer");
    m.addItem (3, "KILL EVERYTHING - new sounds and a new beat");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                     [safe = juce::Component::SafePointer<KillBeatButton> (this)] (int r)
                     {
                         if (safe == nullptr)
                             return;
                         if (r == 1 && safe->onClick) safe->onClick();
                         if (r == 2 && safe->onKillKit) safe->onKillKit();
                         if (r == 3 && safe->onKillAll) safe->onKillAll();
                     });
}

void StationDrag::paint (juce::Graphics& g)
{
    const auto hot = isMouseOver() || dragging;
    auto r = getLocalBounds().toFloat().reduced (1.5f);
    juce::Path border;
    border.addRoundedRectangle (r, 4.0f);
    juce::Path dashed;
    const float dashes[] { 4.0f, 3.0f };
    juce::PathStrokeType (1.2f).createDashedStroke (dashed, border, dashes, 2);
    g.setColour (hot ? ember : juce::Colours::white.withAlpha (0.3f));
    g.fillPath (dashed);
    if (hot)
    {
        g.setColour (ember.withAlpha (0.08f));
        g.fillRoundedRectangle (r, 4.0f);
    }
    auto icon = r.removeFromLeft (r.getHeight()).reduced (r.getHeight() * 0.22f);
    drawIcon (g, Icon::midiDrag, icon, hot ? ember : kDim);
    drawStamped (g, label, bold (juce::jmin (11.0f, r.getHeight() * 0.48f), 0.12f), r.withTrimmedRight (6.0f), juce::Justification::centred,
                 hot ? emberHot : kText);
}

//==============================================================================
Pad::Pad (RollsKillaProcessor& p, int t) : proc (p), type (t)
{
    addAndMakeVisible (killButton);
    killButton.setTooltip ("KILL - a brand new sound in this drawer");
    killButton.onClick = [this]
    {
        proc.killSound (drumTypeFromIndex (type));
        proc.auditionSlot (drumTypeFromIndex (type));
        if (onChanged) onChanged();
    };
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTooltip (juce::String (drumTypeName (drumTypeFromIndex (type))) + " - click = play, drop a WAV = your sound, right-click = more");
}

void Pad::resized()
{
    killButton.setBounds (getWidth() - 46, getHeight() - 26, 38, 17);
}

void Pad::tick()
{
    const auto hits = proc.getHitCount (drumTypeFromIndex (type));
    bool dirty = false;
    if (hits != lastHits)
    {
        lastHits = hits;
        flash = 1.0f;
        dirty = true;
    }
    else if (flash > 0.01f)
    {
        flash *= 0.8f;
        dirty = true;
    }
    const auto snd = proc.getSlotSound (drumTypeFromIndex (type));
    if (snd.get() != shownSound)
    {
        shownSound = snd.get();
        wave.clear();
        if (snd != nullptr && snd->audio.getNumSamples() > 0)
        {
            const auto& a = snd->audio;
            const int cols = 48;
            const auto n = a.getNumSamples();
            std::vector<float> peaks ((size_t) cols, 0.0f);
            for (int c = 0; c < cols; ++c)
                peaks[(size_t) c] = a.getMagnitude (0, c * n / cols, juce::jmax (1, n / cols));
            wave.startNewSubPath (0.0f, 0.0f);
            for (int c = 0; c < cols; ++c)
                wave.lineTo ((float) c, -peaks[(size_t) c]);
            for (int c = cols; c-- > 0;)
                wave.lineTo ((float) c, peaks[(size_t) c]);
            wave.closeSubPath();
        }
        dirty = true;
    }
    if (dirty)
        repaint();
}

void Pad::paint (juce::Graphics& g)
{
    const auto col = drawerColour (type);
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    const auto& slot = proc.getKit().slot (drumTypeFromIndex (type));

    if (selected)
        drawGlow (g, r, 7.0f, col, 1.6f, 5);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 7.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff302d2b), r.getX(), r.getY(), juce::Colour (0xff121110), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 7.0f);
    // rubber texture
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), r.getX(), r.getY(), juce::Colours::transparentWhite, r.getX(), r.getY() + 18.0f, false));
    g.fillRoundedRectangle (r.reduced (1.0f), 6.0f);
    if (flash > 0.01f)
    {
        g.setGradientFill (juce::ColourGradient (col.withAlpha (0.55f * flash), r.getCentre(), col.withAlpha (0.0f), r.getTopLeft(), true));
        g.fillRoundedRectangle (r, 7.0f);
    }
    g.setColour (selected ? col : juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, selected ? 2.0f : 1.0f);

    auto inner = r.reduced (8.0f, 6.0f);
    const auto snd = proc.getSlotSound (drumTypeFromIndex (type));
    drawStamped (g, drumTypeName (drumTypeFromIndex (type)), bold (14.0f, 0.06f), inner.removeFromTop (17.0f).withTrimmedRight (12.0f), juce::Justification::centredLeft,
                 slot.muted ? kDim : col);
    auto nameRow = inner.removeFromBottom (17.0f);
    juce::String sname = snd != nullptr ? snd->name : juce::String();
    if (snd != nullptr && (drumTypeFromIndex (type) == DrumType::b808 || drumTypeFromIndex (type) == DrumType::kick) && snd->rootNote != 60)
        sname << "  " << noteNameOf (snd->rootNote);
    if (slot.file.isNotEmpty())
        sname = "WAV  " + sname;
    drawStamped (g, sname.toUpperCase(), bold (9.0f, 0.04f), nameRow.withTrimmedRight (40.0f), juce::Justification::centredLeft, kDim);

    // pattern LED (top right) + kept count
    const auto led = juce::Point<float> (r.getRight() - 11.0f, r.getY() + 14.0f);
    metal::drawLed (g, led, 3.6f, (slot.patternOn || drumTypeFromIndex (type) == DrumType::hat) && ! slot.muted ? 1.0f : 0.0f);
    const auto kept = proc.getKit().numKept (drumTypeFromIndex (type));
    if (kept > 0)
        drawStamped (g, "KEPT x" + juce::String (kept), bold (8.5f), juce::Rectangle<float> (r.getRight() - 70.0f, r.getY() + 24.0f, 62.0f, 12.0f),
                     juce::Justification::centredRight, emberHot);

    if (! wave.isEmpty())
    {
        auto wr = inner.reduced (2.0f, 4.0f);
        const auto t = juce::AffineTransform::scale (wr.getWidth() / 48.0f, wr.getHeight() * 0.5f).translated (wr.getX(), wr.getCentreY());
        g.setColour (col.withAlpha (slot.muted ? 0.2f : 0.55f + 0.45f * flash));
        g.fillPath (wave, t);
    }
    if (dragOver)
    {
        g.setColour (ember.withAlpha (0.25f));
        g.fillRoundedRectangle (r, 7.0f);
        drawStamped (g, "DROP WAV", bold (13.0f, 0.2f), r, juce::Justification::centred, emberHot);
    }
}

void Pad::mouseDown (const juce::MouseEvent& e)
{
    const auto t = drumTypeFromIndex (type);
    if (! e.mods.isPopupMenu())
    {
        if (onSelect) onSelect (type);
        proc.auditionSlot (t);
        return;
    }
    const auto& slot = proc.getKit().slot (t);
    juce::PopupMenu m;
    m.addSectionHeader (drumTypeName (t));
    m.addItem (1, "KILL - new sound");
    m.addItem (2, "Load your WAV...");
    m.addItem (3, "Back to the synth sound", slot.file.isNotEmpty());
    m.addItem (4, "KEEP in the kit");
    m.addSeparator();
    m.addItem (5, "Mute", true, slot.muted);
    if (t != DrumType::hat)
        m.addItem (6, "Pattern plays", true, slot.patternOn);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                     [safe = juce::Component::SafePointer<Pad> (this), t] (int r)
                     {
                         if (safe == nullptr || r == 0)
                             return;
                         auto& p = safe->proc;
                         switch (r)
                         {
                             case 1: p.killSound (t); p.auditionSlot (t); break;
                             case 2: if (safe->onLoadWav) safe->onLoadWav (safe->type); break;
                             case 3: p.clearSlotFile (t); break;
                             case 4: p.keepSound (t); break;
                             case 5: p.updateKitSlot (t, [] (KitSlot& s) { s.muted = ! s.muted; }, true); break;
                             case 6: p.updateKitSlot (t, [] (KitSlot& s) { s.patternOn = ! s.patternOn; }, true); break;
                             default: break;
                         }
                         if (safe->onChanged) safe->onChanged();
                     });
}

bool Pad::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac"))
            return true;
    return false;
}

void Pad::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    for (const auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac"))
        {
            proc.loadSlotFile (drumTypeFromIndex (type), juce::File (f));
            proc.auditionSlot (drumTypeFromIndex (type));
            if (onSelect) onSelect (type);
            if (onChanged) onChanged();
            break;
        }
    repaint();
}

//==============================================================================
BeatView::BeatView (RollsKillaProcessor& p) : proc (p)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTooltip ("The whole beat. Click a lane = that drawer, click its light = pattern on / off");
}

juce::Rectangle<float> BeatView::lane (int i) const
{
    auto r = getLocalBounds().toFloat().reduced (6.0f, 24.0f).withTrimmedBottom (-18.0f);
    const auto h = r.getHeight() / (float) kNumDrumTypes;
    return { r.getX(), r.getY() + h * (float) i, r.getWidth(), h };
}

void BeatView::tick()
{
    const auto ph = proc.getPlayheadBeat();
    if (std::abs (ph - shownPlayhead) > 1.0e-6 || proc.getPatternVersion() != shownVersion)
    {
        shownPlayhead = ph;
        shownVersion = proc.getPatternVersion();
        repaint();
    }
}

void BeatView::paint (juce::Graphics& g)
{
    const auto len = juce::jmax (1.0, proc.getBeatLength());
    drawBay (g, getLocalBounds().toFloat(), "BEAT  /  " + juce::String (juce::roundToInt (len / 4.0)) + (len > 4.0 ? " BARS" : " BAR"));

    const auto labelW = 74.0f;
    const auto& kit = proc.getKit();
    const auto& hatPattern = proc.getModel().getPattern();
    const auto hatLen = juce::jmax (1.0, hatPattern.lengthBeats());

    for (int i = 0; i < kNumDrumTypes; ++i)
    {
        const auto type = drumTypeFromIndex (i);
        const auto& slot = kit.slot (type);
        auto r = lane (i);
        const auto on = (type == DrumType::hat || slot.patternOn) && ! slot.muted;
        const auto col = drawerColour (i);

        if (i == selected)
        {
            g.setColour (col.withAlpha (0.08f));
            g.fillRect (r);
        }
        auto label = r.removeFromLeft (labelW);
        metal::drawLed (g, { label.getX() + 8.0f, label.getCentreY() }, 3.6f, on ? 1.0f : 0.0f);
        drawStamped (g, drumTypeName (type), bold (10.0f, 0.06f), label.withTrimmedLeft (16.0f), juce::Justification::centredLeft, on ? col : kDim);

        auto grid = r.reduced (0.0f, 2.0f);
        g.setColour (juce::Colour (0xff0a0909));
        g.fillRect (grid);
        for (int beat = 0; beat <= (int) len; ++beat)
        {
            g.setColour (beat % 4 == 0 ? juce::Colour (0xff3a3532) : juce::Colour (0xff201d1b));
            g.drawVerticalLine (juce::roundToInt (grid.getX() + grid.getWidth() * (float) (beat / len)), grid.getY(), grid.getBottom());
        }

        auto drawHit = [&] (double beat, float vel)
        {
            const auto x = grid.getX() + grid.getWidth() * (float) (beat / len);
            const auto h = juce::jmax (2.0f, grid.getHeight() * (0.25f + 0.75f * vel));
            g.fillRect (juce::Rectangle<float> (x, grid.getBottom() - h, juce::jmax (1.2f, grid.getWidth() / (float) (len * 16.0) * 0.6f), h));
        };
        g.setColour (on ? col.withAlpha (0.9f) : col.withAlpha (0.18f));
        if (type == DrumType::hat)
        {
            for (double off = 0.0; off < len - 1.0e-6; off += hatLen)
                for (const auto& n : hatPattern.notes)
                    if (! n.muted)
                        drawHit (n.beat + off, (float) n.vel / 127.0f);
        }
        else
        {
            const auto hits = kit.pattern (type);
            const auto slotLen = slot.patternBars * 4.0;
            for (double off = 0.0; off < len - 1.0e-6; off += slotLen)
                for (const auto& h : hits)
                    drawHit (h.beat + off, h.vel);
        }
    }

    const auto ph = proc.getPlayheadBeat();
    if (ph >= 0.0)
    {
        auto all = lane (0).getUnion (lane (kNumDrumTypes - 1)).withTrimmedLeft (labelW);
        const auto x = all.getX() + all.getWidth() * (float) (std::fmod (ph, len) / len);
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.fillRect (juce::Rectangle<float> (x - 2.0f, all.getY(), 4.0f, all.getHeight()));
        g.setColour (juce::Colour (0xfffff4e8));
        g.fillRect (juce::Rectangle<float> (x - 0.6f, all.getY(), 1.2f, all.getHeight()));
    }
}

void BeatView::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < kNumDrumTypes; ++i)
        if (lane (i).contains (e.position))
        {
            const auto type = drumTypeFromIndex (i);
            if (e.position.x < lane (i).getX() + 18.0f)
            {
                if (type == DrumType::hat)
                    proc.updateKitSlot (type, [] (KitSlot& s) { s.muted = ! s.muted; }, true);
                else
                    proc.updateKitSlot (type, [] (KitSlot& s) { s.patternOn = ! s.patternOn || s.muted; s.muted = false; }, true);
                if (onChanged) onChanged();
            }
            selected = i;
            if (onSelect) onSelect (i);
            repaint();
            return;
        }
}

//==============================================================================
SoundPanel::SoundPanel (RollsKillaProcessor& p) : proc (p)
{
    struct K { MetalKnob* k; double lo, hi, step; };
    for (auto [k, lo, hi, step] : { K { &tune, -12.0, 12.0, 0.1 }, K { &decay, 0.0, 1.0, 0.0 }, K { &punch, 0.0, 1.0, 0.0 },
                                    K { &drive, 0.0, 1.0, 0.0 }, K { &tone, 0.0, 1.0, 0.0 }, K { &body, 0.0, 1.0, 0.0 },
                                    K { &volume, -36.0, 6.0, 0.1 } })
    {
        k->setRange (lo, hi, step);
        addAndMakeVisible (*k);
        k->onDragEnd = [this] { proc.commitUndoStep(); };
    }
    tune.setTextValueSuffix (" st");
    volume.setTextValueSuffix (" dB");
    tune.setDoubleClickReturnValue (true, 0.0);
    volume.setDoubleClickReturnValue (true, -8.0);
    for (auto* k : { &decay, &punch, &tone, &body })
        k->setDoubleClickReturnValue (true, 0.5);
    drive.setDoubleClickReturnValue (true, 0.0);

    auto bind = [this] (MetalKnob& k, std::function<void (KitSlot&, float)> set)
    {
        k.onValueChange = [this, &k, set]
        {
            const auto v = (float) k.getValue();
            proc.updateKitSlot (drumTypeFromIndex (type), [set, v] (KitSlot& s) { set (s, v); }, false);
            if (onChanged) onChanged();
        };
    };
    bind (tune, [] (KitSlot& s, float v) { s.shape.tune = v; });
    bind (decay, [] (KitSlot& s, float v) { s.shape.decay = v; });
    bind (punch, [] (KitSlot& s, float v) { s.shape.punch = v; });
    bind (drive, [] (KitSlot& s, float v) { s.shape.drive = v; });
    bind (tone, [] (KitSlot& s, float v) { s.shape.tone = v; });
    bind (body, [] (KitSlot& s, float v) { s.shape.body = v; });
    bind (volume, [] (KitSlot& s, float v) { s.volumeDb = v; });
    tune.setTooltip ("Tune (semitones) - the 808 key follows");
    decay.setTooltip ("Length of the sound");
    punch.setTooltip ("Transient: soft ... hard hit");
    drive.setTooltip ("Saturation / distortion");
    tone.setTooltip ("Dark ... bright");
    body.setTooltip ("Noise / metal ... body / tone (snare, clap, hats, perc)");
    volume.setTooltip ("Volume of this drawer in the beat");

    for (auto* b : { &killButton, &loadButton, &keepButton })
        addAndMakeVisible (b);
    killButton.setTooltip ("A brand new sound in this drawer (CHILL / TRAP / CRAZY decides how wild)");
    loadButton.setTooltip ("Your own WAV in this drawer (or drop it on the pad)");
    keepButton.setTooltip ("KEEP this sound in the kit - EXPORT KIT writes all kept sounds");
    killButton.onClick = [this] { proc.killSound (drumTypeFromIndex (type)); proc.auditionSlot (drumTypeFromIndex (type)); refresh(); if (onChanged) onChanged(); };
    loadButton.onClick = [this] { if (onLoadWav) onLoadWav(); };
    keepButton.onClick = [this] { proc.keepSound (drumTypeFromIndex (type)); if (onChanged) onChanged(); };
}

void SoundPanel::setDrawer (int t)
{
    type = t;
    refresh();
}

void SoundPanel::refresh()
{
    const auto& s = proc.getKit().slot (drumTypeFromIndex (type));
    for (auto* k : { &tune, &decay, &punch, &drive, &tone, &body, &volume })
        k->accent = drawerColour (type);
    tune.setValue (s.shape.tune, juce::dontSendNotification);
    decay.setValue (s.shape.decay, juce::dontSendNotification);
    punch.setValue (s.shape.punch, juce::dontSendNotification);
    drive.setValue (s.shape.drive, juce::dontSendNotification);
    tone.setValue (s.shape.tone, juce::dontSendNotification);
    body.setValue (s.shape.body, juce::dontSendNotification);
    volume.setValue (s.volumeDb, juce::dontSendNotification);
    repaint();
}

void SoundPanel::resized()
{
    auto r = getLocalBounds().reduced (10, 6);
    r.removeFromTop (18);
    auto buttons = r.removeFromRight (98);
    const auto bh = juce::jmin (26, buttons.getHeight() / 3 - 4);
    killButton.setBounds (buttons.removeFromTop (bh + 6).reduced (0, 2));
    loadButton.setBounds (buttons.removeFromTop (bh + 4).reduced (0, 2));
    keepButton.setBounds (buttons.removeFromTop (bh + 4).reduced (0, 2));
    r.removeFromRight (6);
    const auto w = r.getWidth() / 7;
    for (auto* k : { &tune, &decay, &punch, &drive, &tone, &body, &volume })
        k->setBounds (r.removeFromLeft (w).reduced (2, 0));
}

void SoundPanel::paint (juce::Graphics& g)
{
    drawBay (g, getLocalBounds().toFloat(), {});
    const auto t = drumTypeFromIndex (type);
    const auto snd = proc.getSlotSound (t);
    juce::String title = "SOUND  /  " + juce::String (drumTypeName (t));
    if (snd != nullptr)
        title << "  /  " << snd->name.toUpperCase();
    if (snd != nullptr && (t == DrumType::b808 || t == DrumType::kick) && snd->rootNote != 60)
        title << "  (" << noteNameOf (snd->rootNote) << ")";
    if (proc.getKit().slot (t).file.isNotEmpty())
        title << "  -  YOUR WAV";
    drawStamped (g, title, bold (11.0f, 0.16f), getLocalBounds().toFloat().reduced (10.0f, 6.0f).removeFromTop (14.0f),
                 juce::Justification::centredLeft, drawerColour (type));
}

//==============================================================================
PatternPanel::PatternPanel (RollsKillaProcessor& p) : proc (p)
{
    for (auto* c : std::initializer_list<juce::Component*> { &style, &bars, &density, &onButton, &killButton, &drag })
        addAndMakeVisible (c);
    density.setRange (0.0, 1.0);
    density.setDoubleClickReturnValue (true, 0.5);
    density.setTooltip ("How busy the pattern is (a new pattern when you let go)");
    style.onChange = [this] (int i) { proc.updateKitSlot (drumTypeFromIndex (type), [i] (KitSlot& s) { s.patternStyle = i; s.patternOn = true; s.muted = false; }, true); refresh(); if (onChanged) onChanged(); };
    bars.onChange = [this] (int i) { proc.updateKitSlot (drumTypeFromIndex (type), [i] (KitSlot& s) { s.patternBars = 1 << i; }, true); if (onChanged) onChanged(); };
    density.onValueChange = [this]
    {
        const auto v = (float) density.getValue();
        proc.updateKitSlot (drumTypeFromIndex (type), [v] (KitSlot& s) { s.patternDensity = v; }, false);
        if (onChanged) onChanged();
    };
    density.onDragEnd = [this] { proc.commitUndoStep(); };
    onButton.setTooltip ("This drawer's pattern plays with the beat");
    onButton.onClick = [this] { proc.updateKitSlot (drumTypeFromIndex (type), [] (KitSlot& s) { s.patternOn = ! s.patternOn; s.muted = false; }, true); refresh(); if (onChanged) onChanged(); };
    killButton.setTooltip ("A brand new pattern for this drawer");
    killButton.onClick = [this] { proc.killPattern (drumTypeFromIndex (type)); refresh(); if (onChanged) onChanged(); };
    drag.createFile = [this] { return proc.createSlotMidiFile (drumTypeFromIndex (type)); };
    drag.setTooltip ("Drag this drawer's pattern into FL (Piano roll / Playlist)");
}

void PatternPanel::setDrawer (int t)
{
    type = t;
    style.setLabels (drumPatternStyles (drumTypeFromIndex (type)));
    refresh();
}

void PatternPanel::refresh()
{
    const auto& s = proc.getKit().slot (drumTypeFromIndex (type));
    style.setSelected (s.patternStyle);
    bars.setSelected (s.patternBars >= 8 ? 3 : s.patternBars >= 4 ? 2 : s.patternBars >= 2 ? 1 : 0);
    density.setValue (s.patternDensity, juce::dontSendNotification);
    density.accent = drawerColour (type);
    onButton.lit = s.patternOn && ! s.muted;
    onButton.setButtonText (onButton.lit ? "ON" : "OFF");
    repaint();
}

void PatternPanel::resized()
{
    auto r = getLocalBounds().reduced (10, 6);
    r.removeFromTop (18);
    auto right = r.removeFromRight (128);
    density.setBounds (r.removeFromRight (78).reduced (2, 0));
    r.removeFromRight (6);
    const auto bh = right.getHeight() / 3;
    onButton.setBounds (right.removeFromTop (bh).reduced (0, 2));
    killButton.setBounds (right.removeFromTop (bh).reduced (0, 2));
    drag.setBounds (right.reduced (0, 2));
    r.removeFromRight (8);
    const auto rowH = juce::jmin (26, r.getHeight() / 2 - 6);
    r.removeFromTop (14);
    style.setBounds (r.removeFromTop (rowH));
    r.removeFromTop (18);
    bars.setBounds (r.removeFromTop (rowH).withWidth (juce::jmin (r.getWidth(), 200)));
}

void PatternPanel::paint (juce::Graphics& g)
{
    drawBay (g, getLocalBounds().toFloat(), {});
    const auto area = getLocalBounds().toFloat().reduced (10.0f, 6.0f);
    drawStamped (g, "PATTERN  /  " + juce::String (drumTypeName (drumTypeFromIndex (type))), bold (11.0f, 0.16f), area.withHeight (14.0f),
                 juce::Justification::centredLeft, drawerColour (type));
    drawStamped (g, "STYLE", bold (9.0f, 0.2f), juce::Rectangle<float> ((float) style.getX(), (float) style.getY() - 13.0f, 100.0f, 12.0f), juce::Justification::centredLeft, kDim);
    drawStamped (g, "BARS", bold (9.0f, 0.2f), juce::Rectangle<float> ((float) bars.getX(), (float) bars.getY() - 13.0f, 100.0f, 12.0f), juce::Justification::centredLeft, kDim);
}

//==============================================================================
HatPanel::HatPanel (RollsKillaProcessor& p)
    : proc (p),
      presetBar (p),
      speed (p.getState(), params::rollSpeed, { "SLOW", "ORIG", "FAST" }),
      densityA (p.getState(), params::density, density),
      swingA (p.getState(), params::swing, swing),
      variationA (p.getState(), params::variation, variation),
      pitchA (p.getState(), params::pitchRamp, pitch)
{
    for (auto* c : std::initializer_list<juce::Component*> { &presetBar, &speed, &density, &swing, &variation, &pitch, &killButton, &drag })
        addAndMakeVisible (c);
    presetBar.compact = true;
    presetBar.onOpenBrowser = [this] (int cat) { if (onOpenBrowser) onOpenBrowser (cat); };
    speed.setTooltip ("Roll speed: one step slower / faster (1/24 - 1/32 - 1/48 - 1/64)");
    density.setTooltip ("Fewer (< 100 %) or more (> 100 %) rolls");
    variation.setTooltip ("How much KILL ROLL changes the preset");
    pitch.setTooltip ("Pitch ramp across every roll");
    for (auto* k : { &density, &swing, &variation, &pitch })
        k->accent = drawerColour ((int) DrumType::hat);
    killButton.setTooltip ("KILL ROLL - a new variation of the hi-hat roll (right-click a pad for more)");
    killButton.onClick = [this] { proc.kill(); if (onChanged) onChanged(); };
    drag.createFile = [this] { return proc.createDragMidiFile(); };
    drag.setTooltip ("Drag the hi-hat roll into FL");
}

void HatPanel::refresh()
{
    presetBar.refresh();
}

void HatPanel::resized()
{
    auto r = getLocalBounds().reduced (10, 6);
    r.removeFromTop (18);
    presetBar.setBounds (r.removeFromTop (34));
    r.removeFromTop (6);
    auto right = r.removeFromRight (120);
    killButton.setBounds (right.removeFromTop (right.getHeight() / 2).reduced (0, 2));
    drag.setBounds (right.reduced (0, 2));
    r.removeFromRight (8);
    auto left = r.removeFromLeft (r.getWidth() * 2 / 5);
    speed.setBounds (left.withSizeKeepingCentre (left.getWidth(), 26).translated (0, 6));
    const auto w = r.getWidth() / 4;
    for (auto* k : { &density, &swing, &variation, &pitch })
        k->setBounds (r.removeFromLeft (w).reduced (2, 0));
}

void HatPanel::paint (juce::Graphics& g)
{
    drawBay (g, getLocalBounds().toFloat(), {});
    const auto area = getLocalBounds().toFloat().reduced (10.0f, 6.0f);
    drawStamped (g, "HI-HAT ROLLS  /  96 TRAP PRESETS + KILL", bold (11.0f, 0.16f), area.withHeight (14.0f), juce::Justification::centredLeft,
                 drawerColour ((int) DrumType::hat));
    drawStamped (g, "ROLL SPEED", bold (9.0f, 0.2f), juce::Rectangle<float> ((float) speed.getX(), (float) speed.getY() - 13.0f, 120.0f, 12.0f),
                 juce::Justification::centredLeft, kDim);
}

//==============================================================================
KitPanel::KitPanel (RollsKillaProcessor& p) : proc (p)
{
    addAndMakeVisible (nameEditor);
    nameEditor.setFont (bold (15.0f, 0.12f));
    nameEditor.setJustification (juce::Justification::centredLeft);
    nameEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0b0a0a));
    nameEditor.setColour (juce::TextEditor::textColourId, emberHot);
    nameEditor.setColour (juce::TextEditor::outlineColourId, juce::Colours::white.withAlpha (0.12f));
    nameEditor.setColour (juce::TextEditor::focusedOutlineColourId, ember);
    nameEditor.setInputRestrictions (48);
    nameEditor.setTooltip ("The kit's name (folder and file names)");
    auto commitName = [this] { proc.setKitName (nameEditor.getText()); nameEditor.setText (proc.getKit().name, false); };
    nameEditor.onReturnKey = commitName;
    nameEditor.onFocusLost = commitName;

    for (auto* c : std::initializer_list<juce::Component*> { &exportButton, &oneShotButton, &killKitButton, &oneShotCount, &revealButton, &dragBeat })
        addAndMakeVisible (c);
    oneShotCount.setSelected (1);
    exportButton.setTooltip ("Write the kit: 808s, Kicks, Snares, Claps, Hi-Hats, Open Hats, Percussion, FX + MIDI (24-bit WAV)");
    oneShotButton.setTooltip ("A one-shot kit: this many different sounds of the selected drawer");
    killKitButton.setTooltip ("New sounds in every drawer");
    dragBeat.setTooltip ("Drag the whole beat into FL (one track per drawer)");
    exportButton.onClick = [this] { if (onExportKit) onExportKit(); };
    oneShotButton.onClick = [this]
    {
        static const int counts[] { 10, 25, 50, 100 };
        if (onExportOneShots) onExportOneShots (counts[oneShotCount.getSelected()]);
    };
    killKitButton.onClick = [this] { proc.killKit(); if (onChanged) onChanged(); };
    dragBeat.createFile = [this] { return proc.createBeatMidiFile(); };
    revealButton.setVisible (false);
    revealButton.onClick = [this] { if (messageFolder.exists()) messageFolder.revealToUser(); };
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void KitPanel::refresh()
{
    if (! nameEditor.hasKeyboardFocus (true))
        nameEditor.setText (proc.getKit().name, false);
    oneShotButton.setButtonText ("ONE SHOT KIT  /  " + juce::String (drumTypeName (drumTypeFromIndex (selectedDrawer))));
    repaint();
}

void KitPanel::showMessage (const juce::String& text, const juce::File& folder)
{
    message = text;
    messageFolder = folder;
    revealButton.setVisible (folder.exists());
    repaint();
}

juce::Rectangle<float> KitPanel::chipArea() const
{
    return getLocalBounds().toFloat().reduced (12.0f, 8.0f).withTrimmedTop (50.0f).withTrimmedRight ((float) getWidth() * 0.44f).withTrimmedBottom (2.0f);
}

std::vector<std::pair<int, juce::Rectangle<float>>> KitPanel::chipRects() const
{
    std::vector<std::pair<int, juce::Rectangle<float>>> out;
    const auto area = chipArea();
    const auto font = bold (10.0f, 0.04f);
    float x = area.getX(), y = area.getY();
    const auto& kept = proc.getKit().kept;
    for (int i = 0; i < (int) kept.size(); ++i)
    {
        const auto text = juce::String (drumTypeName (kept[(size_t) i].type)) + "  " + kept[(size_t) i].name.toUpperCase();
        const auto w = juce::GlyphArrangement::getStringWidth (font, text) + 34.0f;
        if (x + w > area.getRight())
        {
            x = area.getX();
            y += 22.0f;
        }
        if (y + 20.0f > area.getBottom())
            break;
        out.push_back ({ i, { x, y, w, 19.0f } });
        x += w + 5.0f;
    }
    return out;
}

void KitPanel::resized()
{
    auto r = getLocalBounds().reduced (12, 8);
    auto top = r.removeFromTop (30);
    nameEditor.setBounds (top.removeFromLeft (juce::jmin (330, top.getWidth() / 2)).withTrimmedLeft (44));
    auto right = getLocalBounds().reduced (12, 8).removeFromRight ((int) (getWidth() * 0.42f));
    auto row1 = right.removeFromTop (34);
    exportButton.setBounds (row1.removeFromLeft (row1.getWidth() / 2).reduced (2));
    killKitButton.setBounds (row1.reduced (2));
    right.removeFromTop (4);
    auto row2 = right.removeFromTop (30);
    oneShotCount.setBounds (row2.removeFromRight (150).reduced (2, 3));
    oneShotButton.setBounds (row2.reduced (2));
    right.removeFromTop (4);
    auto row3 = right.removeFromTop (34);
    dragBeat.setBounds (row3.removeFromLeft (row3.getWidth() / 2).reduced (2));
    revealButton.setBounds (row3.removeFromRight (70).reduced (2));
}

void KitPanel::paint (juce::Graphics& g)
{
    drawBay (g, getLocalBounds().toFloat(), {});
    auto r = getLocalBounds().toFloat().reduced (12.0f, 8.0f);
    drawStamped (g, "KIT", bold (16.0f, 0.2f), r.withHeight (30.0f).withWidth (40.0f), juce::Justification::centredLeft, ember);
    const auto kept = (int) proc.getKit().kept.size();
    drawStamped (g, kept == 0 ? juce::String ("KEEP THE SOUNDS YOU LIKE  -  EXPORT KIT WRITES EVERY DRAWER (KEPT SOUNDS, OR THE CURRENT ONE)")
                              : juce::String (kept) + (kept == 1 ? " SOUND KEPT" : " SOUNDS KEPT"),
                 bold (9.0f, 0.1f), juce::Rectangle<float> (r.getX(), r.getY() + 32.0f, chipArea().getWidth(), 14.0f), juce::Justification::centredLeft, kDim);

    const auto& list = proc.getKit().kept;
    for (const auto& [i, rect] : chipRects())
    {
        const auto& k = list[(size_t) i];
        const auto col = drawerColour ((int) k.type);
        g.setColour (juce::Colour (0xff221f1d));
        g.fillRoundedRectangle (rect, 9.0f);
        g.setColour (i == hoverChip ? col : col.withAlpha (0.4f));
        g.drawRoundedRectangle (rect.reduced (0.5f), 9.0f, 1.0f);
        g.setColour (col);
        g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ rect.getX() + 10.0f, rect.getCentreY() }));
        drawStamped (g, juce::String (drumTypeName (k.type)) + "  " + k.name.toUpperCase(), bold (10.0f, 0.04f),
                     rect.withTrimmedLeft (18.0f).withTrimmedRight (14.0f), juce::Justification::centredLeft, kText);
        drawStamped (g, "x", bold (10.0f), rect.withTrimmedLeft (rect.getWidth() - 14.0f), juce::Justification::centredLeft, i == hoverChip ? kBlood : kDim);
    }

    if (message.isNotEmpty())
    {
        // row 3, right of DRAG BEAT, left of SHOW
        auto m = getLocalBounds().reduced (12, 8).removeFromRight ((int) (getWidth() * 0.42f)).withTrimmedTop (72).withHeight (34).toFloat();
        m = m.withTrimmedLeft (m.getWidth() / 2.0f + 8.0f).withTrimmedRight (72.0f);
        g.setFont (bold (9.0f, 0.04f));
        g.setColour (emberHot);
        g.drawFittedText (message, m.toNearestInt(), juce::Justification::centredLeft, 2, 0.8f);
    }
}

void KitPanel::mouseMove (const juce::MouseEvent& e)
{
    int found = -1;
    for (const auto& [i, rect] : chipRects())
        if (rect.contains (e.position))
            found = i;
    if (found != hoverChip)
    {
        hoverChip = found;
        setMouseCursor (found >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void KitPanel::mouseDown (const juce::MouseEvent& e)
{
    for (const auto& [i, rect] : chipRects())
        if (rect.contains (e.position) && e.position.x > rect.getRight() - 16.0f)
        {
            proc.removeKept (i);
            hoverChip = -1;
            if (onChanged) onChanged();
            repaint();
            return;
        }
}

} // namespace rk::ui::station
