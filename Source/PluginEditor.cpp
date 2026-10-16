#include "PluginEditor.h"
#include <BinaryData.h>

using namespace HG;

// ============================================================================ fonts / look
const Fonts& Fonts::get()
{
    static Fonts f = []
    {
        Fonts x;
        x.anton = juce::Typeface::createSystemTypefaceFor (BinaryData::AntonRegular_ttf, (size_t) BinaryData::AntonRegular_ttfSize);
        x.mono  = juce::Typeface::createSystemTypefaceFor (BinaryData::IBMPlexMonoBold_ttf, (size_t) BinaryData::IBMPlexMonoBold_ttfSize);
        x.black = juce::Typeface::createSystemTypefaceFor (BinaryData::UnifrakturCookBold_ttf, (size_t) BinaryData::UnifrakturCookBold_ttfSize);
        x.led   = juce::Typeface::createSystemTypefaceFor (BinaryData::VT323Regular_ttf, (size_t) BinaryData::VT323Regular_ttfSize);
        return x;
    }();
    return f;
}
juce::Font Fonts::label (float h)  { return juce::Font (juce::FontOptions (get().mono).withHeight (h)); }
juce::Font Fonts::title (float h)  { return juce::Font (juce::FontOptions (get().anton).withHeight (h)); }
juce::Font Fonts::gothic (float h) { return juce::Font (juce::FontOptions (get().black).withHeight (h)); }
juce::Font Fonts::lcd (float h)    { return juce::Font (juce::FontOptions (get().led).withHeight (h)); }

Look::Look()
{
    setColour (juce::ComboBox::backgroundColourId, black);
    setColour (juce::ComboBox::textColourId, bone);
    setColour (juce::ComboBox::outlineColourId, line);
    setColour (juce::ComboBox::arrowColourId, muted);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, bone);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, red);
    setColour (juce::PopupMenu::highlightedTextColourId, ink);
    setColour (juce::PopupMenu::headerTextColourId, red);
    setColour (juce::TextButton::buttonColourId, ink);
    setColour (juce::TextButton::buttonOnColourId, bone);
    setColour (juce::TextButton::textColourOffId, bone);
    setColour (juce::TextButton::textColourOnId, ink);
}

void Look::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    juce::Path track, value;
    track.addCentredArc (c.x, c.y, r - 1, r - 1, 0.0f, start, end, true);
    g.setColour (line);
    g.strokePath (track, juce::PathStrokeType (2.0f));
    const float ang = start + pos * (end - start);
    value.addCentredArc (c.x, c.y, r - 1, r - 1, 0.0f, start, ang, true);
    g.setColour (red);
    g.strokePath (value, juce::PathStrokeType (2.0f));
    const float ir = r * 0.74f;
    g.setColour (juce::Colour (0xff161616));
    g.fillEllipse (c.x - ir, c.y - ir, ir * 2, ir * 2);
    g.setColour (juce::Colour (0xff333333));
    g.drawEllipse (c.x - ir, c.y - ir, ir * 2, ir * 2, 1.0f);
    juce::Path ptr;
    ptr.startNewSubPath (c.x, c.y);
    ptr.lineTo (c.x + std::sin (ang) * ir * 0.9f, c.y - std::cos (ang) * ir * 0.9f);
    g.setColour (bone);
    g.strokePath (ptr, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void Look::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat();
    const bool on = b.getToggleState();
    const auto accent = b.getProperties()["accent"].toString() == "red";
    juce::Colour fill = on ? (accent ? red : bone) : ink;
    if (! on && (over || down)) fill = juce::Colour (0xff1a1a1a);
    g.setColour (fill);
    g.fillRect (r);
    g.setColour (on ? fill : line);
    g.drawRect (r, 1.0f);
}

juce::Font Look::getTextButtonFont (juce::TextButton& b, int h)
{
    if ((bool) b.getProperties()["big"]) return Fonts::title (h * 0.62f);
    return Fonts::label (juce::jmin (11.0f, h * 0.45f));
}

void Look::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (b.getToggleState() ? ink : (b.isEnabled() ? bone : muted));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (3, 0), juce::Justification::centred, 1);
}

void Look::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    g.setColour (black);
    g.fillRect (0, 0, w, h);
    g.setColour (line);
    g.drawRect (0, 0, w, h, 1);
    juce::Path tri;
    const float cx = (float) w - 12.0f, cy = h * 0.5f;
    tri.addTriangle (cx - 4, cy - 2, cx + 4, cy - 2, cx, cy + 3);
    g.setColour (muted);
    g.fillPath (tri);
}
juce::Font Look::getComboBoxFont (juce::ComboBox&) { return Fonts::label (12.0f); }
juce::Font Look::getPopupMenuFont() { return Fonts::label (13.0f); }
void Look::positionComboBoxText (juce::ComboBox& box, juce::Label& l)
{
    l.setBounds (6, 0, box.getWidth() - 24, box.getHeight());
    l.setFont (getComboBoxFont (box));
}

// ============================================================================ helpers
static void panelBox (juce::Graphics& g, juce::Rectangle<int> r, juce::Colour border = line)
{
    g.setColour (panel);
    g.fillRect (r);
    g.setColour (border);
    g.drawRect (r, 1);
}

static void smallLabel (juce::Graphics& g, const juce::String& t, juce::Rectangle<int> r, juce::Colour c = muted,
                        juce::Justification j = juce::Justification::centredLeft, float h = 10.0f)
{
    g.setColour (c);
    g.setFont (Fonts::label (h).withExtraKerningFactor (0.18f));
    g.drawText (t.toUpperCase(), r, j, false);
}

static void ledText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, float h, juce::Colour c,
                     juce::Justification j = juce::Justification::centredLeft)
{
    juce::GlyphArrangement ga;
    ga.addFittedText (Fonts::lcd (h), t, r.getX(), r.getY(), r.getWidth(), r.getHeight(), j, 1);
    juce::Path p;
    ga.createPath (p);
    juce::DropShadow (c.withAlpha (0.55f), (int) juce::jmax (4.0f, h * 0.12f), {}).drawForPath (g, p);
    g.setColour (c);
    g.fillPath (p);
}

static void scanlines (juce::Graphics& g, juce::Rectangle<int> r)
{
    g.setColour (juce::Colours::white.withAlpha (0.035f));
    for (int y = r.getY(); y < r.getBottom(); y += 3)
        g.fillRect (r.getX(), y, r.getWidth(), 1);
}

// Never take the keyboard from the host: after a click on the plugin, FL Studio's
// space bar, typing keyboard and shortcuts must keep working.
static void neverTakeKeyboard (juce::Component& c)
{
    c.setWantsKeyboardFocus (false);
    c.setMouseClickGrabsKeyboardFocus (false);
    for (auto* ch : c.getChildren()) neverTakeKeyboard (*ch);
}
static void childrenNeverTakeKeyboard (juce::Component& c)
{
    for (auto* ch : c.getChildren()) neverTakeKeyboard (*ch);
}

// ============================================================================ knob
Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& l, std::function<juce::String (float)> fmt)
    : state (s), paramId (id), label (l), format (std::move (fmt))
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (180);
    addAndMakeVisible (slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
    if (auto* p = s.getParameter (id))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    slider.onValueChange = [this] { repaint(); };
    slider.onMenu = [this] { showMenu(); };
    slider.setTooltip ("Double-click or right-click to reset");
}

void Knob::resetToDefault()
{
    if (auto* p = state.getParameter (paramId))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->getDefaultValue());
        p->endChangeGesture();
    }
}

void Knob::showMenu()
{
    auto* p = state.getParameter (paramId);
    if (p == nullptr) return;
    juce::PopupMenu m;
    m.addSectionHeader (label.toUpperCase() + "  " + format ((float) slider.getValue()));
    m.addItem ("Reset to default (" + format (p->convertFrom0to1 (p->getDefaultValue())) + ")", [this] { resetToDefault(); });
    m.addItem ("Set to minimum", [p] { p->beginChangeGesture(); p->setValueNotifyingHost (0.0f); p->endChangeGesture(); });
    m.addItem ("Set to maximum", [p] { p->beginChangeGesture(); p->setValueNotifyingHost (1.0f); p->endChangeGesture(); });

    // the host's own items (automation, link to controller ...) when it offers them
    if (auto* ed = findParentComponentOfClass<juce::AudioProcessorEditor>())
        if (auto* ctx = ed->getHostContext())
            if (auto hostMenu = ctx->getContextMenuForParameter (p))
            {
                auto hm = hostMenu->getEquivalentPopupMenu();
                if (hm.getNumItems() > 0)
                {
                    m.addSeparator();
                    m.addSubMenu ("Host", hm);
                }
            }
    m.addSeparator();
    m.addItem ("Tip: double-click a knob to reset it", false, false, nullptr);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&slider));
}

void Knob::resized()
{
    auto r = getLocalBounds();
    const int d = juce::jmin (r.getWidth(), r.getHeight() - 26);
    slider.setBounds (r.removeFromTop (d).withSizeKeepingCentre (d, d));
}

void Knob::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    r.removeFromTop (juce::jmin (r.getWidth(), r.getHeight() - 26));
    smallLabel (g, label, r.removeFromTop (13), muted, juce::Justification::centred, 8.5f);
    g.setColour (bone);
    g.setFont (Fonts::label (9.5f));
    g.drawText (format ((float) slider.getValue()), r.removeFromTop (12), juce::Justification::centred);
}

static std::function<juce::String (float)> pct()   { return [] (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; }; }
static std::function<juce::String (float)> semis() { return [] (float v) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " st"; }; }
static std::function<juce::String (float)> db()    { return [] (float v) { return juce::String (v, 1) + " dB"; }; }
static std::function<juce::String (float)> hz()    { return [] (float v) { return v >= 1000 ? juce::String (v / 1000.0f, 1) + "k" : juce::String (juce::roundToInt (v)); }; }

// ============================================================================ keyboard
class GoblinKeyboard : public juce::MidiKeyboardComponent
{
public:
    explicit GoblinKeyboard (juce::MidiKeyboardState& s) : MidiKeyboardComponent (s, horizontalKeyboard)
    {
        setAvailableRange (36, 72);
        setScrollButtonsVisible (false);
        setWantsKeyboardFocus (false);
        setMouseClickGrabsKeyboardFocus (false);
        setVelocity (0.9f, false);
        setColour (shadowColourId, juce::Colours::transparentBlack);
        setColour (keySeparatorLineColourId, ink);
        setColour (upDownButtonBackgroundColourId, ink);
    }
    std::function<int()> activeNote;
    std::function<bool()> flNames;

    static juce::Colour roleColour (int n, bool black)
    {
        if (KeyMap::isRate (n)) return red;
        if (KeyMap::isPattern (n)) return boneKey;
        if (n == KeyMap::pitchDown || n == KeyMap::pitchUp || n == KeyMap::latch) return gray;
        if (n == KeyMap::hitNote) return redDim;
        return black ? line : juce::Colour (0xff1a1a1a);
    }

    void drawWhiteNote (int n, juce::Graphics& g, juce::Rectangle<float> a, bool down, bool, juce::Colour, juce::Colour) override
    {
        const bool live = down || (activeNote && activeNote() == n);
        auto r = a.reduced (1.0f, 0.0f);
        g.setColour (live ? juce::Colours::white : roleColour (n, false));
        g.fillRect (r);
        if (live)
        {
            g.setColour (red);
            g.drawRect (r, 2.0f);
        }
        const bool darkText = KeyMap::isRate (n) || KeyMap::isPattern (n) || live;
        g.setColour (darkText ? ink : bone);
        g.setFont (Fonts::label (9.0f));
        g.drawText (KeyMap::keyLabel (n), r.withTrimmedBottom (14).removeFromBottom (14).toNearestInt(), juce::Justification::centred);
        g.setColour (darkText ? ink.withAlpha (0.5f) : gray);
        g.setFont (Fonts::label (7.0f));
        g.drawText (KeyMap::noteName (n, flNames && flNames()), r.removeFromBottom (13).toNearestInt(), juce::Justification::centred);
    }

    void drawBlackNote (int n, juce::Graphics& g, juce::Rectangle<float> a, bool down, bool, juce::Colour) override
    {
        const bool live = down || (activeNote && activeNote() == n);
        auto c = roleColour (n, true);
        g.setColour (live ? juce::Colours::white : black);
        g.fillRect (a);
        g.setColour (line);
        g.drawRect (a, 1.0f);
        g.setColour (c);
        g.fillRect (a.removeFromTop (5.0f));
        g.setFont (Fonts::label (7.0f));
        g.setColour (live ? ink : c);
        g.drawText (KeyMap::keyLabel (n), a.removeFromBottom (14).toNearestInt(), juce::Justification::centred);
    }
};

// ============================================================================ pads (patterns)
class PatternPad : public juce::Component
{
public:
    PatternPad (HatGoblinProcessor& p, int idx) : proc (p), index (idx) {}
    std::function<int()> activeNote;
    std::function<void (int)> onSelect;          // EDIT page: picks the slot
    std::function<bool()> selected;
    int note() const { return KeyMap::firstPattern + index; }

    void paint (juce::Graphics& g) override
    {
        const bool on = activeNote && activeNote() == note();
        const bool sel = selected && selected();
        auto r = getLocalBounds();
        g.setColour (black);
        g.fillRect (r);
        g.setColour (on ? red : (sel ? bone : lineSoft));
        g.drawRect (r, sel ? 2 : 1);
        auto dots = r.reduced (6).removeFromTop (12);
        const float w = dots.getWidth() / 16.0f;
        for (int i = 0; i < 16; ++i)
        {
            const int n = proc.engine.step (index, i);
            g.setColour (n == 0 ? lineSoft : (on ? red : (n > 1 ? redDim : gray)));
            const float hgt = n == 0 ? 2.0f : juce::jmin (12.0f, 2.0f + n * 2.0f);
            g.fillRect (dots.getX() + i * w + 0.5f, (float) dots.getBottom() - hgt, w - 1.0f, hgt);
        }
        g.setColour (on ? red : muted);
        g.setFont (Fonts::label (8.0f));
        g.drawText (KeyMap::noteName (note(), proc.flNoteNames()) + " " + proc.engine.patternName (index),
                    r.reduced (6, 4).removeFromBottom (11), juce::Justification::centredLeft);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (onSelect) onSelect (index);
        if (! e.mods.isPopupMenu()) { proc.keyboardState.noteOn (1, note(), 0.9f); down = true; }
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (down) proc.keyboardState.noteOff (1, note(), 0.0f);
        down = false;
    }

private:
    HatGoblinProcessor& proc;
    int index;
    bool down = false;
};

// ============================================================================ drag tiles (MIDI / audio / hat)
class DragTile : public juce::Component
{
public:
    DragTile (juce::String t, std::function<juce::String()> subText, std::function<juce::File()> makeFile, bool isBig)
        : title (std::move (t)), sub (std::move (subText)), make (std::move (makeFile)), big (isBig)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds();
        if (big || dragging)
        {
            g.setColour (dragging ? bone : red);
            g.fillRect (r);
        }
        else
        {
            g.setColour (black);
            g.fillRect (r);
            g.setColour (red);
            g.drawRect (r, 1);
        }
        const auto fg = (big || dragging) ? ink : bone;
        auto t = r.reduced (big ? 14 : 8, big ? 8 : 5);
        g.setColour (fg);
        g.setFont (Fonts::title (big ? 26.0f : 15.0f));
        g.drawText (title, t.removeFromTop (big ? 28 : 17), juce::Justification::left);
        g.setColour ((big || dragging) ? ink : muted);
        g.setFont (Fonts::label (big ? 9.0f : 7.5f).withExtraKerningFactor (0.1f));
        g.drawText (sub ? sub() : juce::String(), t.removeFromTop (12), juce::Justification::left);
        if (big)
        {
            juce::Path arrow;
            const float x = (float) r.getRight() - 28.0f, y = (float) r.getCentreY();
            arrow.startNewSubPath (x - 10, y); arrow.lineTo (x + 10, y);
            arrow.startNewSubPath (x + 3, y - 7); arrow.lineTo (x + 10, y); arrow.lineTo (x + 3, y + 7);
            g.setColour (fg);
            g.strokePath (arrow, juce::PathStrokeType (2.4f));
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 5) return;
        dragging = true;
        repaint();
        auto file = make();
        if (! file.existsAsFile()) { dragging = false; repaint(); return; }
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this,
                                                                    [this] { dragging = false; repaint(); });
    }
    void mouseUp (const juce::MouseEvent&) override { dragging = false; repaint(); }

private:
    juce::String title;
    std::function<juce::String()> sub;
    std::function<juce::File()> make;
    bool big = false, dragging = false;
};

// ============================================================================ groove grid (main page)
// One bar = 4 beats. Each beat: 4 slots (1/16), 3 (1/8T) or 6 (1/16T). Each slot: 0..6 hits.
// Click a cell = that many hits (clicking the top cell again clears), drag paints, right-click clears,
// click a beat's header to switch 1/16 -> 1/8T -> 1/16T.
class GrooveGrid : public juce::Component
{
public:
    explicit GrooveGrid (HatGoblinProcessor& p) : proc (p) {}
    int shownBar = 0;
    std::function<void()> onEdited;

    static constexpr int levels = KeyMap::maxHitsPerStep, headH = 26, axisW = 46;

    juce::Rectangle<float> cells() const { return getLocalBounds().toFloat().withTrimmedLeft ((float) axisW).withTrimmedTop ((float) headH); }
    juce::Rectangle<float> beatArea (int i) const
    {
        auto a = cells();
        const float w = a.getWidth() / 4.0f;
        return { a.getX() + i * w, a.getY(), w, a.getHeight() };
    }

    void paint (juce::Graphics& g) override
    {
        auto a = cells();
        g.setColour (black);
        g.fillRect (a);
        const float ch = a.getHeight() / (float) levels;
        static const char* rowNames[] = { "HAT", "X2", "X3", "X4", "X5", "X6" };
        for (int l = 0; l < levels; ++l)
            smallLabel (g, rowNames[l], juce::Rectangle<float> (0, a.getBottom() - (l + 1) * ch, (float) axisW - 6, ch).toNearestInt(),
                        l == 0 ? bone : muted, juce::Justification::centredRight, 8.0f);
        smallLabel (g, "ROLL", juce::Rectangle<int> (0, (int) a.getY() - 2, axisW - 6, 14), red, juce::Justification::centredRight, 8.0f);

        const int nowBeat = proc.engine.grooveNowBeat.load(), nowSlot = proc.engine.grooveNowSlot.load();
        for (int i = 0; i < 4; ++i)
        {
            const int beat = shownBar * 4 + i;
            const int d = proc.engine.beatDivision (beat);
            auto ba = beatArea (i);
            // header: beat number + division
            auto head = juce::Rectangle<float> (ba.getX() + 2, 0, ba.getWidth() - 4, (float) headH - 4);
            g.setColour (d == 4 ? juce::Colour (0xff161616) : redDark);
            g.fillRect (head);
            g.setColour (d == 4 ? line : red);
            g.drawRect (head, 1.0f);
            smallLabel (g, "BEAT " + juce::String (beat + 1), head.reduced (6, 0).toNearestInt(), bone, juce::Justification::centredLeft, 8.5f);
            smallLabel (g, d == 4 ? "1/16" : (d == 3 ? "1/8T" : "1/16T"), head.reduced (6, 0).toNearestInt(),
                        d == 4 ? muted : red, juce::Justification::centredRight, 8.5f);

            if (i % 2 == 0) { g.setColour (juce::Colour (0xff111111)); g.fillRect (ba); }
            const float sw = ba.getWidth() / (float) d;
            for (int k = 0; k < d; ++k)
            {
                const int v = proc.engine.slotHits (beat, k);
                const bool now = nowBeat == beat && nowSlot == k;
                const float x = ba.getX() + k * sw;
                if (now)
                {
                    g.setColour (bone.withAlpha (0.10f));
                    g.fillRect (x, a.getY(), sw, a.getHeight());
                }
                for (int l = 0; l < levels; ++l)
                {
                    auto cell = juce::Rectangle<float> (x + 2, a.getBottom() - (l + 1) * ch + 2, sw - 4, ch - 4);
                    const bool filled = l < v;
                    juce::Colour col = juce::Colour (0xff1b1b1b);
                    if (filled) col = l == v - 1 ? (now ? bone : red) : redDim;
                    g.setColour (col);
                    g.fillRect (cell);
                }
            }
            g.setColour (line);
            g.drawRect (ba, 1.0f);
        }
    }

    bool hitTestBeatHeader (juce::Point<float> p, int& beatOut) const
    {
        if (p.y > headH) return false;
        for (int i = 0; i < 4; ++i)
            if (p.x >= beatArea (i).getX() && p.x < beatArea (i).getRight()) { beatOut = shownBar * 4 + i; return true; }
        return false;
    }

    bool cellAt (juce::Point<float> p, int& beat, int& slot, int& level) const
    {
        auto a = cells();
        if (p.x < a.getX() || p.x >= a.getRight()) return false;
        const int i = juce::jlimit (0, 3, (int) ((p.x - a.getX()) / (a.getWidth() / 4.0f)));
        beat = shownBar * 4 + i;
        const int d = proc.engine.beatDivision (beat);
        auto ba = beatArea (i);
        slot = juce::jlimit (0, d - 1, (int) ((p.x - ba.getX()) / (ba.getWidth() / d)));
        level = juce::jlimit (0, levels, (int) std::ceil ((a.getBottom() - p.y) / (a.getHeight() / levels)));
        return true;
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        changed = false;
        int beat = 0;
        if (hitTestBeatHeader (e.position, beat))
        {
            const int d = proc.engine.beatDivision (beat);
            const int nd = d == 4 ? 3 : (d == 3 ? 6 : 4);
            // keep the feel: map the old slots onto the new ones
            std::array<int, 6> old {};
            for (int k = 0; k < 6; ++k) old[(size_t) k] = proc.engine.slotHits (beat, k);
            proc.engine.setBeatDivision (beat, nd);
            for (int k = 0; k < 6; ++k)
                proc.engine.setSlotHits (beat, k, k < nd ? juce::jmax (k == 0 ? 1 : 0, old[(size_t) juce::jmin (d - 1, k * d / nd)]) : 0);
            if (nd != 4) for (int k = 0; k < nd; ++k) if (proc.engine.slotHits (beat, k) == 0) proc.engine.setSlotHits (beat, k, 1);
            changed = true;
            finish();
            return;
        }
        int slot = 0, level = 0;
        if (! cellAt (e.position, beat, slot, level)) return;
        if (e.mods.isPopupMenu()) { proc.engine.setSlotHits (beat, slot, 0); changed = true; finish(); return; }
        paintValue = proc.engine.slotHits (beat, slot) == level ? 0 : level;
        proc.engine.setSlotHits (beat, slot, paintValue);
        changed = true;
        repaint();
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() || e.mouseDownPosition.y <= headH) return;
        int beat = 0, slot = 0, level = 0;
        if (cellAt (e.position, beat, slot, level) && proc.engine.slotHits (beat, slot) != paintValue)
        {
            proc.engine.setSlotHits (beat, slot, paintValue);
            changed = true;
            repaint();
        }
    }
    void mouseUp (const juce::MouseEvent&) override { finish(); }

private:
    void finish()
    {
        if (changed && onEdited) onEdited();
        changed = false;
        repaint();
    }
    HatGoblinProcessor& proc;
    int paintValue = 1;
    bool changed = false;
};

// ============================================================================ the goblin
class GoblinFace : public juce::Component
{
public:
    GoblinFace()
    {
        grin  = juce::Drawable::createFromImageData (BinaryData::grin_svg, (size_t) BinaryData::grin_svgSize);
        laugh = juce::Drawable::createFromImageData (BinaryData::laugh_svg, (size_t) BinaryData::laugh_svgSize);
        setInterceptsMouseClicks (false, false);
    }
    void cackle() { laughUntil = juce::Time::getMillisecondCounter() + 900; repaint(); }
    bool laughing() const { return juce::Time::getMillisecondCounter() < laughUntil; }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const bool l = laughing();
        auto* d = l ? laugh.get() : grin.get();
        if (d != nullptr)
        {
            const float wob = l ? std::sin ((float) juce::Time::getMillisecondCounter() * 0.04f) * 2.5f : 0.0f;
            d->drawWithin (g, r.reduced (6).translated (0, wob), juce::RectanglePlacement::centred, 1.0f);
        }
        if (l)
        {
            g.setColour (red);
            g.setFont (Fonts::title (26.0f));
            g.drawText ("HA", juce::Rectangle<float> (r.getRight() - 58, r.getY() + 4, 50, 30), juce::Justification::centred);
            g.setColour (bone);
            g.setFont (Fonts::title (20.0f));
            g.drawText ("HA", juce::Rectangle<float> (r.getX() + 8, r.getY() + 10, 40, 26), juce::Justification::centred);
        }
    }

private:
    std::unique_ptr<juce::Drawable> grin, laugh;
    juce::uint32 laughUntil = 0;
};

// ============================================================================ help overlay
class HelpOverlay : public juce::Component
{
public:
    explicit HelpOverlay (HatGoblinProcessor& p) : proc (p)
    {
        close.setButtonText ("GOT IT - LET'S ROLL");
        close.getProperties().set ("accent", "red");
        close.setToggleState (true, juce::dontSendNotification);
        close.onClick = [this] { proc.helpSeen = true; setVisible (false); };
        addAndMakeVisible (close);
    }
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black.withAlpha (0.9f));
        auto box = getLocalBounds().reduced (120, 40);
        panelBox (g, box, red);
        auto r = box.reduced (30, 22);
        g.setColour (red);
        g.setFont (Fonts::gothic (40.0f));
        g.drawText ("How the goblin rolls", r.removeFromTop (48), juce::Justification::left);
        struct Step { const char* head; const char* body; };
        static const Step steps[] = {
            { "1  PRESS PLAY", "Hit PLAY or the SPACE BAR (click on the goblin first). It plays at your DAW's tempo. When your DAW plays, the goblin is quiet, unless WITH DAW is on." },
            { "2  GENERATE", "Pick a style (or MIX), set COMPLEXITY, smash GENERATE. Don't like it? Smash it again. The arrows bring back the ones before. Or pick a PRESET." },
            { "3  TWEAK", "Click the grid: HAT = one hit, X2..X6 = a roll inside that slot. Click a beat's header for 1/8T or 1/16T triplets. FEEL knobs: swing, humanize, probability, pitch ramp." },
            { "4  YOUR SOUND", "120 factory hats: arrows at the top or the KIT page. Or drop your own WAV anywhere on the goblin." },
            { "5  TAKE IT", "DRAG AUDIO: the loop as a WAV, straight into your playlist. DRAG MIDI: into the piano roll of any hat channel (notes on C5). DRAG HAT: the sample itself. What you drag is exactly what you hear." },
        };
        for (auto& st : steps)
        {
            g.setColour (bone);
            g.setFont (Fonts::title (20.0f));
            g.drawText (st.head, r.removeFromTop (25), juce::Justification::left);
            g.setColour (muted);
            g.setFont (Fonts::label (11.0f));
            g.drawFittedText (st.body, r.removeFromTop (46), juce::Justification::topLeft, 3);
            r.removeFromTop (6);
        }
    }
    void resized() override
    {
        auto box = getLocalBounds().reduced (120, 40);
        close.setBounds (box.getRight() - 30 - 230, box.getBottom() - 22 - 36, 230, 36);
    }
    void mouseDown (const juce::MouseEvent&) override {}

private:
    HatGoblinProcessor& proc;
    juce::TextButton close;
};

// ============================================================================ the whole UI (1100 x 680)
class GoblinContent : public juce::Component
{
public:
    enum Page { groovePage = 0, shapePage, fxPage, kitPage, keysPage };

    explicit GoblinContent (HatGoblinEditor& e)
        : ed (e), proc (e.proc), keyboard (e.proc.keyboardState),
          kTune (proc.apvts, "tune", "Tune", semis()), kDecay (proc.apvts, "decay", "Decay", pct()),
          kVol (proc.apvts, "volume", "Volume", db()),
          kSwing (proc.apvts, "swing", "Swing", pct()), kHum (proc.apvts, "humanize", "Humanize", pct()),
          kDepth (proc.apvts, "velDepth", "Dynamics", pct()), kRnd (proc.apvts, "velRand", "Vel rnd", pct()),
          kProb (proc.apvts, "prob", "Prob", pct()),
          kPitch (proc.apvts, "pitchRamp", "Pitch ramp", semis()), kPanW (proc.apvts, "panWidth", "Pan width", pct()),
          kDepth2 (proc.apvts, "velDepth", "Dynamics", pct()), kSwing2 (proc.apvts, "swing", "Swing", pct()),
          kDrive (proc.apvts, "drive", "Drive", pct()), kCrush (proc.apvts, "crush", "Amount", pct()),
          kHpf (proc.apvts, "hpf", "High pass", hz()), kLpf (proc.apvts, "lpf", "Low pass", hz()),
          kVerb (proc.apvts, "reverb", "Mix", pct()), kOut (proc.apvts, "out", "Master", db()),
          kSwingG (proc.apvts, "swing", "Swing", pct()), kHumG (proc.apvts, "humanize", "Humanize", pct()),
          kProbG (proc.apvts, "prob", "Prob", pct()), kDepthG (proc.apvts, "velDepth", "Dynamics", pct()),
          kPitchG (proc.apvts, "pitchRamp", "Pitch ramp", semis()), kTuneG (proc.apvts, "tune", "Tune", semis()),
          kComplex (proc.apvts, "complexity", "Complexity", pct()),
          grid (e.proc), help (e.proc),
          dragAudio ("DRAG AUDIO", [this] { return "WAV - " + lengthText() + " - INTO YOUR PLAYLIST"; },
                     [this] { return proc.writeAudio (-1); }, true),
          dragMidi ("DRAG MIDI", [] { return "TO A PIANO ROLL"; }, [this] { return proc.writeMidi (-1); }, false),
          dragHat ("DRAG HAT", [] { return "THE SAMPLE"; }, [this] { return proc.writeHatWav(); }, false),
          keysMidi ("DRAG MIDI", [this] { return "1 BAR - " + keyRollName() + " - TO YOUR DAW"; },
                    [this] { return proc.writeMidi (proc.lastRollKey()); }, true),
          keysAudio ("DRAG AUDIO", [] { return "WAV OF THIS ROLL"; }, [this] { return proc.writeAudio (proc.lastRollKey()); }, false)
    {
        const char* tabNames[] = { "GROOVE", "SHAPE", "FX", "KIT" };
        for (int i = 0; i < 4; ++i)
        {
            tabs[i].setButtonText (tabNames[i]);
            tabs[i].setClickingTogglesState (true);
            tabs[i].setRadioGroupId (101);
            tabs[i].onClick = [this, i] { if (tabs[i].getToggleState()) setPage (i); };
            addAndMakeVisible (tabs[i]);
        }
        tabs[0].setToggleState (true, juce::dontSendNotification);

        prevHat.setButtonText ("<"); nextHat.setButtonText (">");
        prevHat.onClick = [this] { stepHat (-1); };
        nextHat.onClick = [this] { stepHat (1); };
        addAndMakeVisible (prevHat); addAndMakeVisible (nextHat);

        const char* speeds[] = { "1/2x", "1x", "2x" };
        for (int i = 0; i < 3; ++i)
        {
            speedBtn[i].setButtonText (speeds[i]);
            speedBtn[i].setTooltip (i == 0 ? "Half time" : (i == 2 ? "Double time" : "Normal"));
            speedBtn[i].onClick = [this, i] { ed.setParam ("speed", (float) i); };
            addAndMakeVisible (speedBtn[i]);
        }
        for (auto* b : { &monoBtn, &latchBtn, &chokeBtn }) b->setClickingTogglesState (true);
        addAndMakeVisible (chokeBtn);
        monoBtn.setButtonText ("MONO"); latchBtn.setButtonText ("LATCH"); chokeBtn.setButtonText ("CHOKE");
        chokeBtn.setTooltip ("Each hit cuts the one before: tight, clean rolls");
        monoAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "mono", monoBtn);
        latchAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "latch", latchBtn);
        chokeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "choke", chokeBtn);
        latchBtn.getProperties().set ("accent", "red");
        helpBtn.setButtonText ("?");
        helpBtn.setTooltip ("How it works");
        helpBtn.onClick = [this] { help.setVisible (true); help.toFront (false); };
        addAndMakeVisible (helpBtn);

        // --- GROOVE page
        playBtn.onClick = [this] { togglePlay(); };
        withDawBtn.setButtonText ("WITH DAW");
        withDawBtn.setClickingTogglesState (true);
        withDawBtn.setTooltip ("Also play while your DAW plays (off = the goblin is quiet when the DAW plays)");
        withDawAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "withDaw", withDawBtn);
        playBtn.getProperties().set ("accent", "red");
        playBtn.getProperties().set ("big", true);
        backBtn.setButtonText ("<"); fwdBtn.setButtonText (">");
        backBtn.setTooltip ("Previous groove"); fwdBtn.setTooltip ("Next groove");
        backBtn.onClick = [this] { proc.goBack(); grid.repaint(); };
        fwdBtn.onClick = [this] { proc.goForward(); grid.repaint(); };
        const char* lens[] = { "1 BAR", "2 BARS", "4 BARS" };
        for (int i = 0; i < 3; ++i)
        {
            lenBtn[i].setButtonText (lens[i]);
            lenBtn[i].onClick = [this, i] { setLength (i); };
        }
        for (int i = 0; i < 4; ++i)
        {
            barBtn[i].setButtonText ("BAR " + juce::String (i + 1));
            barBtn[i].onClick = [this, i] { grid.shownBar = i; followUntil = juce::Time::getMillisecondCounter() + 4000; grid.repaint(); };
        }
        grid.onEdited = [this] { proc.grooveEdited(); };

        generateBtn.setButtonText ("GENERATE");
        generateBtn.getProperties().set ("accent", "red");
        generateBtn.getProperties().set ("big", true);
        generateBtn.setToggleState (true, juce::dontSendNotification);
        generateBtn.setClickingTogglesState (false);
        generateBtn.onClick = [this] { proc.generate(); goblin.cackle(); grid.shownBar = 0; grid.repaint(); };
        for (int i = 0; i < GrooveGen::numStyles; ++i)
        {
            auto b = std::make_unique<juce::TextButton> (GrooveGen::styleName (i));
            b->getProperties().set ("accent", "red");
            b->onClick = [this, i] { ed.setParam ("style", (float) i); };
            styleBtns.push_back (std::move (b));
        }
        varyBtn.setButtonText ("VARY");
        varyBtn.setTooltip ("Same pattern, new humanize / probability dice");
        varyBtn.onClick = [this] { proc.vary(); goblin.cackle(); };
        presetBox.setTextWhenNothingSelected ("PRESETS");
        for (int i = 0; i < GrooveGen::numPresets(); ++i)
        {
            if (i % 6 == 0) presetBox.addSectionHeading (GrooveGen::styleName (1 + i / 6));
            presetBox.addItem (GrooveGen::presetName (i), i + 1);
        }
        presetBox.onChange = [this]
        {
            if (presetBox.getSelectedId() > 0) { proc.loadPreset (presetBox.getSelectedId() - 1); grid.shownBar = 0; grid.repaint(); }
        };
        hatBox2.setJustificationType (juce::Justification::centredLeft);
        loadWavG.setButtonText ("LOAD YOUR HAT");
        loadWavG.onClick = [this] { ed.chooseUserFile(); };
        const char* rollShapes[] = { "FLAT", "SWELL", "FADE", "ACCENT" };
        for (int i = 0; i < 4; ++i)
        {
            rollShapeBtn[i].setButtonText (rollShapes[i]);
            rollShapeBtn[i].getProperties().set ("accent", "red");
            rollShapeBtn[i].onClick = [this, i] { ed.setParam ("velShape", (float) i); };
        }

        // --- KEYS page (old-school: hold keys to roll)
        hatBox.setJustificationType (juce::Justification::centredLeft);
        auto banks = HatLibrary::bankNames();
        for (auto* box : { &hatBox, &hatBox2 })
        {
            for (int b = 0; b < banks.size(); ++b)
            {
                box->addSectionHeading (banks[b]);
                for (int i = 0; i < HatLibrary::hatsPerBank; ++i)
                {
                    const int idx = b * HatLibrary::hatsPerBank + i;
                    if (idx < HatLibrary::get().size())
                        box->addItem (HatLibrary::get().hat (idx).name, idx + 1);
                }
            }
            box->onChange = [this, box]
            {
                if (box->getSelectedId() > 0)
                {
                    ed.setParam ("hat", (float) (box->getSelectedId() - 1));
                    ed.setParam ("useUser", 0.0f);
                }
            };
        }
        loadWav.setButtonText ("LOAD WAV");
        loadWav.onClick = [this] { ed.chooseUserFile(); };
        useFactory.setButtonText ("FACTORY");
        useFactory.onClick = [this] { proc.clearUserSample(); };
        const char* shapes[] = { "FLAT", "UP", "DOWN", "ACC" };
        for (int i = 0; i < 4; ++i)
        {
            shapeBtn[i].setButtonText (shapes[i]);
            shapeBtn[i].getProperties().set ("accent", "red");
            shapeBtn[i].onClick = [this, i] { ed.setParam ("velShape", (float) i); };
        }
        for (int i = 0; i < 12; ++i)
        {
            pads[(size_t) i] = std::make_unique<PatternPad> (proc, i);
            pads[(size_t) i]->activeNote = [this] { return proc.engine.displayNote.load(); };
        }
        keyboard.activeNote = [this] { return proc.engine.displayNote.load(); };
        keyboard.flNames = [this] { return proc.flNoteNames(); };

        // --- SHAPE page
        const char* pans[] = { "CENTER", "PING-PONG", "SWEEP" };
        for (int i = 0; i < 3; ++i)
        {
            panBtn[i].setButtonText (pans[i]);
            panBtn[i].onClick = [this, i] { ed.setParam ("panMode", (float) i); };
        }

        // --- KIT page
        for (int i = 0; i < HatLibrary::get().size(); ++i)
        {
            auto b = std::make_unique<juce::TextButton> (HatLibrary::get().hat (i).name.fromFirstOccurrenceOf (" ", false, false));
            b->getProperties().set ("accent", "red");
            b->onClick = [this, i]
            {
                ed.setParam ("hat", (float) i);
                ed.setParam ("useUser", 0.0f);
                proc.engine.auditionRequest.store (true);
            };
            kitBtns.push_back (std::move (b));
        }
        audition.setButtonText ("PLAY SOUND");
        audition.onClick = [this] { proc.engine.auditionRequest.store (true); };
        loadWav2.setButtonText ("LOAD WAV");
        loadWav2.onClick = [this] { ed.chooseUserFile(); };
        useFactory2.setButtonText ("BACK TO FACTORY");
        useFactory2.onClick = [this] { proc.clearUserSample(); };

        addChildComponent (help);
        setPage (groovePage);
        refresh();
        if (! proc.helpSeen) { help.setVisible (true); }

        // space bar = PLAY / STOP while you work in the plugin
        setWantsKeyboardFocus (true);
        setMouseClickGrabsKeyboardFocus (true);
        addMouseListener (this, true);
    }

    void togglePlay()
    {
        if (proc.hostPlaying.load()) return;          // the DAW is playing: the goblin waits
        proc.engine.previewPlaying.store (! proc.engine.previewPlaying.load());
    }
    void mouseDown (const juce::MouseEvent&) override { if (isShowing()) grabKeyboardFocus(); }
    bool keyPressed (const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::spaceKey) { togglePlay(); return true; }
        return false;
    }

    juce::String lengthText() const
    {
        const int b = proc.currentParams().bars;
        return juce::String (b) + (b == 1 ? " BAR" : " BARS");
    }
    juce::String keyRollName() const
    {
        const int k = proc.lastRollKey();
        return KeyMap::isPattern (k) ? proc.engine.patternName (k - KeyMap::firstPattern) : KeyMap::rateLabel (k);
    }

    void setLength (int i)
    {
        const int oldBeats = proc.currentParams().bars * 4;
        ed.setParam ("bars", (float) i);
        const int newBeats = (i == 2 ? 4 : (i == 1 ? 2 : 1)) * 4;
        for (int b = oldBeats; b < newBeats; ++b)          // new bars start as copies of what is there
        {
            const int src = b % oldBeats;
            proc.engine.setBeatDivision (b, proc.engine.beatDivision (src));
            for (int k = 0; k < 6; ++k) proc.engine.setSlotHits (b, k, proc.engine.slotHits (src, k));
        }
        if (grid.shownBar * 4 >= newBeats) grid.shownBar = 0;
        resized();
        repaint();
    }

    void setPage (int p)
    {
        page = p;
        auto vis = [] (juce::Component& c, bool v, juce::Component* parent)
        {
            if (v && c.getParentComponent() == nullptr) parent->addChildComponent (c);
            c.setVisible (v);
        };
        const bool gr = p == groovePage, shape = p == shapePage, fx = p == fxPage, kit = p == kitPage, keys = p == keysPage;
        for (auto* c : std::initializer_list<juce::Component*> { &playBtn, &withDawBtn, &backBtn, &fwdBtn, &grid, &generateBtn, &goblin, &varyBtn,
                                                                &presetBox, &kComplex, &dragAudio, &dragMidi, &dragHat,
                                                                &kSwingG, &kHumG, &kProbG, &kDepthG, &kPitchG, &kTuneG, &hatBox2, &loadWavG })
            vis (*c, gr, this);
        for (auto& b : lenBtn) vis (b, gr, this);
        for (auto& b : barBtn) vis (b, gr, this);
        for (auto& b : styleBtns) vis (*b, gr, this);
        for (auto& b : rollShapeBtn) vis (b, gr, this);
        for (auto* c : std::initializer_list<juce::Component*> { &hatBox, &kTune, &kDecay, &kVol, &loadWav, &useFactory,
                                                                &kSwing, &kHum, &kDepth, &kRnd, &kProb, &keysMidi, &keysAudio,
                                                                &keyboard, &monoBtn, &latchBtn })
            vis (*c, keys, this);
        for (auto& s : shapeBtn) vis (s, keys || shape, this);
        for (auto& pd : pads) vis (*pd, keys, this);
        for (auto* c : std::initializer_list<juce::Component*> { &kPitch, &kPanW, &kDepth2, &kSwing2 }) vis (*c, shape, this);
        for (auto& b : panBtn) vis (b, shape, this);
        for (auto* c : std::initializer_list<juce::Component*> { &kDrive, &kCrush, &kHpf, &kLpf, &kVerb, &kOut }) vis (*c, fx, this);
        for (auto& b : kitBtns) vis (*b, kit, this);
        for (auto* c : std::initializer_list<juce::Component*> { &audition, &loadWav2, &useFactory2 }) vis (*c, kit, this);
        help.toFront (false);
        childrenNeverTakeKeyboard (*this);
        resized();
        repaint();
    }

    void stepHat (int d)
    {
        const int n = HatLibrary::get().size();
        const int cur = (int) ed.param ("hat");
        ed.setParam ("hat", (float) ((cur + d + n) % n));
        ed.setParam ("useUser", 0.0f);
        proc.engine.auditionRequest.store (true);
    }

    // called by the editor's timer
    void refresh()
    {
        const int sp = (int) ed.param ("speed");
        for (int i = 0; i < 3; ++i) speedBtn[i].setToggleState (sp == i, juce::dontSendNotification);
        const int vs = (int) ed.param ("velShape");
        for (int i = 0; i < 4; ++i)
        {
            shapeBtn[i].setToggleState (vs == i, juce::dontSendNotification);
            rollShapeBtn[i].setToggleState (vs == i, juce::dontSendNotification);
        }
        const int pm = (int) ed.param ("panMode");
        for (int i = 0; i < 3; ++i) panBtn[i].setToggleState (pm == i, juce::dontSendNotification);
        const bool user = ed.param ("useUser") > 0.5f && proc.userSampleName().isNotEmpty();
        const int hat = (int) ed.param ("hat");
        for (auto* box : { &hatBox, &hatBox2 })
        {
            box->setSelectedId (user ? 0 : hat + 1, juce::dontSendNotification);
            if (user) box->setText ("YOUR WAV: " + proc.userSampleName(), juce::dontSendNotification);
        }
        useFactory.setEnabled (user); useFactory2.setEnabled (user);
        for (size_t i = 0; i < kitBtns.size(); ++i)
            kitBtns[i]->setToggleState (! user && (int) i == hat, juce::dontSendNotification);

        // groove page
        const int st = (int) ed.param ("style");
        for (size_t i = 0; i < styleBtns.size(); ++i) styleBtns[i]->setToggleState ((int) i == st, juce::dontSendNotification);
        const int bars = (int) ed.param ("bars");
        for (int i = 0; i < 3; ++i) lenBtn[i].setToggleState (bars == i, juce::dontSendNotification);
        const int nb = proc.currentParams().bars;
        if (nb != shownBars) { shownBars = nb; resized(); }
        const int now = proc.engine.grooveNowBeat.load();
        if (now >= 0 && juce::Time::getMillisecondCounter() > followUntil && ! grid.isMouseButtonDown())
            grid.shownBar = juce::jlimit (0, nb - 1, now / 4);
        for (int i = 0; i < 4; ++i) barBtn[i].setToggleState (grid.shownBar == i, juce::dontSendNotification);
        const bool running = proc.hostPlaying.load() || proc.engine.previewPlaying.load();
        const bool preview = proc.engine.previewPlaying.load();
        juce::ignoreUnused (running);
        playBtn.setButtonText (preview ? "STOP" : "PLAY");
        playBtn.setToggleState (preview || proc.engine.grooveNowBeat.load() >= 0, juce::dontSendNotification);
        backBtn.setEnabled (proc.canGoBack());
        fwdBtn.setEnabled (proc.canGoForward());
        presetBox.setSelectedId (proc.presetIndex >= 0 ? proc.presetIndex + 1 : 0, juce::dontSendNotification);
        if (proc.presetIndex < 0) presetBox.setText ("PRESETS", juce::dontSendNotification);

        auto* s = proc.currentSample();
        if (s != shownSample) { shownSample = s; buildPeaks(); }
        if (page == keysPage)
        {
            for (auto& pd : pads) pd->repaint();
            keyboard.repaint();
            keysMidi.repaint();
            repaint (ledArea);
        }
        if (page == groovePage)
        {
            grid.repaint();
            dragAudio.repaint();
            if (goblin.laughing() || goblinWasLaughing) goblin.repaint();
            goblinWasLaughing = goblin.laughing();
            repaint (statusArea);
            repaint (nameArea);
        }
        if (page == shapePage) repaint (shapeLanes);
        repaint (headerArea);
    }

    void buildPeaks()
    {
        peaks.clear();
        if (shownSample == nullptr) return;
        const auto& a = shownSample->audio;
        const int n = a.getNumSamples(), cols = 110;
        for (int c = 0; c < cols; ++c)
        {
            const int s0 = n * c / cols, s1 = juce::jmax (s0 + 1, n * (c + 1) / cols);
            peaks.push_back (a.getMagnitude (0, s0, juce::jmin (s1, n) - s0));
        }
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (ink);

        // header
        g.setColour (juce::Colour (0xff070707));
        g.fillRect (headerArea);
        g.setColour (line);
        g.drawHorizontalLine (headerArea.getBottom() - 1, 0.0f, (float) getWidth());
        if (ed.logo.isValid())
        {
            const float h = 38.0f, w = h * ed.logo.getWidth() / (float) ed.logo.getHeight();
            g.drawImage (ed.logo, juce::Rectangle<float> (14.0f, 8.0f, w, h), juce::RectanglePlacement::centred);
        }
        g.setColour (line);
        g.fillRect (138, 14, 1, 26);
        g.setColour (red);
        g.setFont (Fonts::gothic (24.0f));
        g.drawText ("Hat Goblin", 146, 10, 130, 34, juce::Justification::centredLeft);

        // sound LED
        g.setColour (black);
        g.fillRect (presetLed);
        g.setColour (line);
        g.drawRect (presetLed, 1);
        auto* s = proc.currentSample();
        ledText (g, s != nullptr ? s->name : "--", presetLed.toFloat().reduced (32, 0), 20.0f, red, juce::Justification::centred);

        // sync
        const bool playing = proc.hostPlaying.load();
        g.setColour (playing ? red : redDark);
        g.fillRect (syncArea.getX() + 8, syncArea.getCentreY() - 3, 6, 6);
        smallLabel (g, juce::String (juce::roundToInt (proc.currentBpm.load())) + " BPM", syncArea.withTrimmedLeft (20), bone, juce::Justification::centredLeft, 10.0f);
        g.setColour (line);
        g.drawRect (syncArea, 1);

        if (page == groovePage) paintGroove (g);
        if (page == keysPage) paintKeys (g);
        if (page == shapePage) paintShape (g);
        if (page == fxPage) paintFx (g);
        if (page == kitPage) paintKit (g);
    }

    juce::String statusText (juce::Colour& c) const
    {
        const auto p = proc.currentParams();
        c = red;
        const auto bpm = juce::String (juce::roundToInt (proc.currentBpm.load())) + " BPM";
        if (proc.engine.previewPlaying.load()) return "PLAYING - " + bpm;
        if (proc.hostPlaying.load())
        {
            if (! p.withDaw) { c = muted; return "DAW PLAYING - GOBLIN WAITS"; }
            if (proc.engine.pausedByMidi.load()) { c = bone; return "NOTES FROM YOUR DAW PLAY"; }
            return "PLAYING WITH YOUR DAW";
        }
        c = redDim;
        return "PRESS PLAY OR SPACE";
    }

    void paintGroove (juce::Graphics& g)
    {
        // status LED
        g.setColour (black);
        g.fillRect (statusArea);
        g.setColour (line);
        g.drawRect (statusArea, 1);
        juce::Colour c;
        const auto txt = statusText (c);
        ledText (g, txt, statusArea.toFloat().reduced (12, 4), 22.0f, c);
        scanlines (g, statusArea);

        // groove name
        g.setColour (black);
        g.fillRect (nameArea);
        g.setColour (line);
        g.drawRect (nameArea, 1);
        smallLabel (g, "Groove", nameArea.reduced (8, 4).removeFromTop (10), muted, juce::Justification::centredLeft, 7.5f);
        g.setColour (bone);
        g.setFont (Fonts::title (19.0f));
        g.drawFittedText (proc.grooveName, nameArea.reduced (8, 2).withTrimmedTop (12), juce::Justification::centredLeft, 1);

        smallLabel (g, "Click = hits  -  drag = paint  -  right-click = clear  -  beat header = triplets", gridHint, gray, juce::Justification::centredRight, 8.0f);

        panelBox (g, feelPanel);
        smallLabel (g, "Feel", feelPanel.reduced (12, 8).removeFromTop (12));
        smallLabel (g, "Roll velocity", rollShapeLabel, muted, juce::Justification::centredLeft, 8.5f);
        smallLabel (g, "Hat", hatLabel, muted, juce::Justification::centredLeft, 8.5f);

        panelBox (g, genPanel, goblin.laughing() ? red : line);
        smallLabel (g, "Style", styleLabel, muted, juce::Justification::centredLeft, 8.5f);
        panelBox (g, outPanel);
        smallLabel (g, "Take it to your DAW", outPanel.reduced (12, 8).removeFromTop (12));
        g.setColour (gray);
        g.setFont (Fonts::label (8.5f));
        g.drawFittedText ("MIDI notes sit on C5 (FL): they play any sampler, or this plugin, at the right pitch.", midiNote, juce::Justification::topLeft, 2);
    }

    void paintKeys (juce::Graphics& g)
    {
        panelBox (g, soundPanel);
        smallLabel (g, "Main hat", soundPanel.reduced (10).removeFromTop (12));
        const bool user = ed.param ("useUser") > 0.5f && proc.userSampleName().isNotEmpty();
        smallLabel (g, user ? "YOUR WAV" : "FACTORY", soundPanel.reduced (10).removeFromTop (12), red, juce::Justification::centredRight);
        g.setColour (black);
        g.fillRect (waveArea);
        g.setColour (lineSoft);
        g.drawRect (waveArea, 1);
        if (! peaks.empty())
        {
            const float w = waveArea.getWidth() / (float) peaks.size(), mid = (float) waveArea.getCentreY();
            g.setColour (red);
            for (size_t i = 0; i < peaks.size(); ++i)
            {
                const float h = juce::jmax (1.0f, peaks[i] * waveArea.getHeight() * 0.46f);
                g.fillRect (waveArea.getX() + i * w, mid - h, juce::jmax (1.0f, w - 0.5f), h * 2);
            }
        }
        g.setColour (gray);
        g.setFont (Fonts::label (8.5f));
        g.drawFittedText ("Latch: tap a key and it keeps rolling, tap it again to stop.", latchHint, juce::Justification::topLeft, 3);

        // LED: what the last key does
        g.setColour (black);
        g.fillRect (ledArea);
        g.setColour (line);
        g.drawRect (ledArea, 1);
        auto led = ledArea.reduced (16, 12);
        const int in = proc.engine.lastIncomingNote.load();
        const bool fl = proc.flNoteNames();
        smallLabel (g, in >= 0 ? "Last key in: " + KeyMap::noteName (in, fl) + "  ->  " + KeyMap::describe (in)
                               : "Hold a key: rates " + KeyMap::noteName (60, fl) + "-" + KeyMap::noteName (72, fl)
                                 + ", patterns " + KeyMap::noteName (48, fl) + "-" + KeyMap::noteName (59, fl),
                    led.removeFromTop (14), in >= 0 ? bone : muted);
        const int note = proc.engine.displayNote.load();
        juce::String big = "--";
        if (KeyMap::isRate (note)) big = KeyMap::rateLabel (note);
        if (KeyMap::isPattern (note)) big = "P" + juce::String (note - KeyMap::firstPattern + 1);
        ledText (g, big, led.removeFromTop (92).toFloat(), 104.0f, note >= 0 ? red : redDark);
        const float po = proc.engine.pitchOffset.load();
        ledText (g, "PITCH " + juce::String (po > 0 ? "+" : "") + juce::String ((int) po) + " ST"
                    + (proc.engine.latchToggled.load() || ed.param ("latch") > 0.5f ? "  -  LATCH ON" : ""),
                 led.removeFromTop (24).toFloat(), 21.0f, redDim);
        auto bars = led.reduced (0, 6);
        const int n = RollEngine::historySize;
        const float bw = bars.getWidth() / (float) n;
        const int w0 = proc.engine.historyWrite.load();
        for (int i = 0; i < n; ++i)
        {
            const float v = proc.engine.history[(size_t) ((w0 + i) % n)].load();
            const float h = juce::jmax (3.0f, v * bars.getHeight());
            const auto col = v <= 0.0f ? line : (i == n - 1 ? red : redDim);
            for (float y = (float) bars.getBottom() - 4.0f; y > bars.getBottom() - h; y -= 6.0f)
            {
                g.setColour (col);
                g.fillRect (bars.getX() + i * bw + 1.0f, y, bw - 2.0f, 4.0f);
            }
        }
        scanlines (g, ledArea);
        smallLabel (g, "Velocity shape", shapeLabel, muted, juce::Justification::centredRight, 8.5f);
        panelBox (g, knobPanel);
        panelBox (g, patternPanel);
        smallLabel (g, "Patterns " + KeyMap::noteName (48, fl) + "-" + KeyMap::noteName (59, fl), patternPanel.reduced (10).removeFromTop (12));

        // legend
        auto lg = legendArea;
        auto chip = [&] (juce::Colour c, const juce::String& t, int w)
        {
            auto r = lg.removeFromLeft (w);
            g.setColour (c);
            g.fillRect (r.getX(), r.getCentreY() - 4, 9, 9);
            smallLabel (g, t, r.withTrimmedLeft (14), muted, juce::Justification::centredLeft, 8.5f);
        };
        chip (gray, "Pitch -/+ " + KeyMap::noteName (36, fl) + " " + KeyMap::noteName (38, fl) + ", latch " + KeyMap::noteName (40, fl), 250);
        chip (boneKey, "Patterns " + KeyMap::noteName (48, fl) + "-" + KeyMap::noteName (59, fl), 170);
        chip (red, "Rates " + KeyMap::noteName (60, fl) + "-" + KeyMap::noteName (72, fl), 150);
        chip (redDim, "One hit " + KeyMap::noteName (KeyMap::hitNote, fl), 140);
    }

    void paintShape (juce::Graphics& g)
    {
        auto lanes = shapeLanes;
        const float depth = ed.param ("velDepth"), ramp = ed.param ("pitchRamp"), width = ed.param ("panWidth");
        const int vs = (int) ed.param ("velShape"), pm = (int) ed.param ("panMode");
        auto lane = [&] (const juce::String& name, const juce::String& range, std::function<float (float, int)> fn, bool stepsMode)
        {
            auto r = lanes.removeFromTop ((shapeLanes.getHeight() - 12) / 3);
            lanes.removeFromTop (6);
            panelBox (g, r);
            auto head = r.removeFromLeft (130);
            g.setColour (bone);
            g.setFont (Fonts::title (24.0f));
            g.drawText (name, head.reduced (10, 8).removeFromTop (26), juce::Justification::left);
            smallLabel (g, range, head.reduced (10, 8).withTrimmedTop (30).removeFromTop (12), muted, juce::Justification::left, 8.5f);
            auto plot = r.reduced (8);
            g.setColour (black);
            g.fillRect (plot);
            scanlines (g, plot);
            juce::Path path;
            const int steps = 12;
            for (int i = 0; i < steps; ++i)
            {
                const float v = juce::jlimit (0.0f, 1.0f, fn (i / (float) (steps - 1), i));
                const float x0 = plot.getX() + plot.getWidth() * i / (float) steps;
                const float x1 = plot.getX() + plot.getWidth() * (i + 1) / (float) steps;
                const float y = plot.getBottom() - v * plot.getHeight();
                if (stepsMode)
                {
                    g.setColour (red.withAlpha (0.85f));
                    g.fillRect (juce::Rectangle<float> (x0 + 3, y, x1 - x0 - 6, plot.getBottom() - y));
                }
                else
                {
                    if (i == 0) path.startNewSubPath (x0, y); else path.lineTo (x0, y);
                    if (i == steps - 1) path.lineTo (x1, y);
                }
            }
            if (! stepsMode)
            {
                g.setColour (bone);
                g.strokePath (path, juce::PathStrokeType (2.0f));
            }
        };
        lane ("ROLL VELOCITY", "across one roll", [&] (float t, int i)
        {
            float v = 0.8f;
            if (vs == 1) v = 0.42f + 0.58f * t;
            if (vs == 2) v = 1.0f - 0.55f * t;
            if (vs == 3) v = (i % 3 == 0) ? 0.95f : 0.55f;
            return 1.0f - depth * (1.0f - v);
        }, true);
        lane ("PITCH", juce::String (ramp, 1) + " st across a roll", [&] (float t, int) { return 0.5f + ramp * t / 24.0f; }, false);
        lane ("PAN", pm == 0 ? "center" : (pm == 1 ? "ping-pong" : "sweep"), [&] (float t, int i)
        {
            if (pm == 1) return 0.5f + ((i & 1) ? 0.5f : -0.5f) * width;
            if (pm == 2) return 0.5f + 0.5f * width * std::sin (juce::MathConstants<float>::twoPi * t);
            return 0.5f;
        }, pm == 1);
        panelBox (g, shapeSide);
        smallLabel (g, "Roll velocity", shapeSide.reduced (12).removeFromTop (12));
        smallLabel (g, "Pan mode", panLabel);
    }

    void paintFx (juce::Graphics& g)
    {
        const char* names[] = { "SATURATE", "CRUSH", "HIGH PASS", "LOW PASS", "ROOM", "OUTPUT" };
        for (int i = 0; i < 6; ++i)
        {
            panelBox (g, fxBoxes[(size_t) i]);
            g.setColour (bone);
            g.setFont (Fonts::title (24.0f));
            g.drawText (names[i], fxBoxes[(size_t) i].reduced (14, 10).removeFromTop (28), juce::Justification::left);
            smallLabel (g, juce::String ("0") + juce::String (i + 1), fxBoxes[(size_t) i].reduced (14, 10).removeFromTop (28), gray, juce::Justification::right, 9.0f);
        }
        smallLabel (g, "Chain: saturate > crush > filters > room > output - 0 ms latency - double-click a knob to reset", fxFoot, muted, juce::Justification::centredLeft, 8.5f);
    }

    void paintKit (juce::Graphics& g)
    {
        auto banks = HatLibrary::bankNames();
        for (int b = 0; b < banks.size(); ++b)
        {
            panelBox (g, kitCols[(size_t) b]);
            g.setColour (bone);
            g.setFont (Fonts::title (20.0f));
            g.drawText (banks[b], kitCols[(size_t) b].reduced (8, 6).removeFromTop (22), juce::Justification::left);
        }
        panelBox (g, kitSide, ed.dragHover ? red : line);
        auto r = kitSide.reduced (14);
        g.setColour (red);
        g.setFont (Fonts::title (26.0f));
        g.drawText ("FEED THE GOBLIN", r.removeFromTop (30), juce::Justification::left);
        g.setColour (muted);
        g.setFont (Fonts::label (10.0f));
        g.drawFittedText ("Drop your own WAV, AIFF, FLAC or MP3 anywhere on the plugin. Everything plays your sound.",
                          r.removeFromTop (54), juce::Justification::topLeft, 4);
        const auto name = proc.userSampleName();
        smallLabel (g, "Your WAV", r.removeFromTop (14));
        g.setColour (name.isNotEmpty() ? bone : gray);
        g.setFont (Fonts::label (12.0f));
        g.drawText (name.isNotEmpty() ? name : "- none loaded -", r.removeFromTop (20), juce::Justification::left);
    }

    void resized() override
    {
        help.setBounds (getLocalBounds());
        headerArea = { 0, 0, getWidth(), 54 };
        int x = 280;
        for (int i = 0; i < 4; ++i) { auto& t = tabs[i]; t.setBounds (x, 14, t.getButtonText().length() * 9 + 26, 26); x += t.getWidth() - 1; }
        presetLed = { x + 10, 11, 168, 32 };
        prevHat.setBounds (presetLed.getX(), presetLed.getY(), 30, presetLed.getHeight());
        nextHat.setBounds (presetLed.getRight() - 30, presetLed.getY(), 30, presetLed.getHeight());
        syncArea = { presetLed.getRight() + 8, 14, 74, 26 };
        int sx = syncArea.getRight() + 8;
        for (auto& b : speedBtn) { b.setBounds (sx, 14, 36, 26); sx += 35; }
        sx += 8;
        chokeBtn.setBounds (sx, 14, 56, 26); sx += 64;
        helpBtn.setBounds (getWidth() - 12 - 30, 14, 30, 26);

        const auto bodyTall = juce::Rectangle<int> (12, 64, getWidth() - 24, getHeight() - 64 - 12);

        if (page == groovePage)
        {
            auto b = bodyTall;
            auto right = b.removeFromRight (262);
            b.removeFromRight (10);

            // right: goblin + generate + style + presets, then drag tiles
            outPanel = right.removeFromBottom (196);
            right.removeFromBottom (8);
            genPanel = right;
            auto gp = genPanel.reduced (10);
            goblin.setBounds (gp.removeFromTop (136));
            generateBtn.setBounds (gp.removeFromTop (50));
            gp.removeFromTop (8);
            styleLabel = gp.removeFromTop (14);
            for (size_t i = 0; i < styleBtns.size(); ++i)
            {
                const int col = (int) i % 3, row = (int) i / 3;
                const int w = gp.getWidth() / 3;
                styleBtns[i]->setBounds (gp.getX() + col * w, gp.getY() + row * 25, w - 2, 23);
            }
            gp.removeFromTop (3 * 25 + 6);
            auto row = gp.removeFromTop (92);
            kComplex.setBounds (row.removeFromLeft (92));
            row.removeFromLeft (6);
            presetBox.setBounds (row.removeFromTop (28));
            row.removeFromTop (6);
            varyBtn.setBounds (row.removeFromTop (28));

            auto op = outPanel.reduced (10);
            op.removeFromTop (18);
            dragAudio.setBounds (op.removeFromTop (70));
            op.removeFromTop (6);
            auto r2 = op.removeFromTop (44);
            dragMidi.setBounds (r2.removeFromLeft (r2.getWidth() / 2 - 3));
            r2.removeFromLeft (6);
            dragHat.setBounds (r2);
            op.removeFromTop (8);
            midiNote = op.removeFromTop (30);

            // left: transport strip
            auto strip = b.removeFromTop (54);
            playBtn.setBounds (strip.removeFromLeft (104));
            strip.removeFromLeft (8);
            withDawBtn.setBounds (strip.removeFromLeft (86).withSizeKeepingCentre (86, 30));
            strip.removeFromLeft (8);
            auto lens = strip.removeFromRight (3 * 58);
            for (int i = 0; i < 3; ++i) lenBtn[i].setBounds (lens.getX() + i * 58, lens.getY() + 12, 56, 30);
            strip.removeFromRight (8);
            fwdBtn.setBounds (strip.removeFromRight (28).withSizeKeepingCentre (28, 54));
            nameArea = strip.removeFromRight (196);
            backBtn.setBounds (strip.removeFromRight (28).withSizeKeepingCentre (28, 54));
            strip.removeFromRight (8);
            statusArea = strip;
            b.removeFromTop (10);

            // bars + grid
            feelPanel = b.removeFromBottom (186);
            b.removeFromBottom (8);
            auto barRow = b.removeFromTop (24);
            gridHint = barRow;
            for (int i = 0; i < 4; ++i)
            {
                barBtn[i].setBounds (barRow.getX() + 46 + i * 62, barRow.getY(), 60, 22);
                barBtn[i].setVisible (page == groovePage && i < shownBars && shownBars > 1);
            }
            b.removeFromTop (4);
            grid.setBounds (b);

            // feel panel
            auto fp = feelPanel.reduced (12, 8);
            fp.removeFromTop (14);
            auto knobs = fp.removeFromTop (92);
            const int kw = knobs.getWidth() / 6;
            for (auto* k : { &kSwingG, &kHumG, &kProbG, &kDepthG, &kPitchG, &kTuneG }) k->setBounds (knobs.removeFromLeft (kw));
            fp.removeFromTop (12);
            auto bottom = fp.removeFromTop (28);
            rollShapeLabel = bottom.removeFromLeft (100);
            for (auto& rb : rollShapeBtn) rb.setBounds (bottom.removeFromLeft (64));
            bottom.removeFromLeft (20);
            hatLabel = bottom.removeFromLeft (34);
            loadWavG.setBounds (bottom.removeFromRight (120));
            bottom.removeFromRight (6);
            hatBox2.setBounds (bottom);
        }

        // KEYS page: the old roll page + keyboard
        {
            auto foot = juce::Rectangle<int> (12, getHeight() - 122, getWidth() - 24, 116);
            legendArea = foot.removeFromTop (18);
            namesBtn.setBounds (legendArea.getRight() - 210, legendArea.getY() - 2, 210, 20);
            legendArea.setRight (legendArea.getRight() - 220);
            keyboard.setBounds (foot);
            keyboard.setKeyWidth (foot.getWidth() / 22.0f);

            auto b = juce::Rectangle<int> (12, 64, getWidth() - 24, getHeight() - 64 - 130);
            soundPanel = b.removeFromLeft (246);
            auto sp = soundPanel.reduced (10);
            sp.removeFromTop (18);
            hatBox.setBounds (sp.removeFromTop (26));
            sp.removeFromTop (8);
            waveArea = sp.removeFromTop (60);
            sp.removeFromTop (8);
            auto row = sp.removeFromTop (78);
            kTune.setBounds (row.removeFromLeft (row.getWidth() / 3));
            kDecay.setBounds (row.removeFromLeft (row.getWidth() / 2));
            kVol.setBounds (row);
            sp.removeFromTop (8);
            auto br = sp.removeFromTop (26);
            loadWav.setBounds (br.removeFromLeft (br.getWidth() / 2 - 2));
            br.removeFromLeft (4);
            useFactory.setBounds (br);
            sp.removeFromTop (14);
            auto ml = sp.removeFromTop (26);
            monoBtn.setBounds (ml.removeFromLeft (ml.getWidth() / 2 - 2));
            ml.removeFromLeft (4);
            latchBtn.setBounds (ml);
            sp.removeFromTop (6);
            latchHint = sp.removeFromTop (30);

            b.removeFromLeft (10);
            auto right = b.removeFromRight (252);
            b.removeFromRight (10);
            auto centre = b;
            knobPanel = centre.removeFromBottom (90);
            centre.removeFromBottom (8);
            ledArea = centre;
            auto kp = knobPanel.reduced (6, 6);
            const int kw = kp.getWidth() / 5;
            for (auto* k : { &kSwing, &kHum, &kDepth, &kRnd, &kProb }) k->setBounds (kp.removeFromLeft (kw));
            auto sb = juce::Rectangle<int> (ledArea.getRight() - 16 - 4 * 52, ledArea.getY() + 30, 4 * 52, 24);
            shapeLabel = sb.translated (0, -16).withHeight (14);
            if (page == keysPage) for (auto& s : shapeBtn) s.setBounds (sb.removeFromLeft (52));

            auto drag = right.removeFromBottom (112);
            keysMidi.setBounds (drag.removeFromTop (70));
            drag.removeFromTop (6);
            keysAudio.setBounds (drag);
            right.removeFromBottom (8);
            patternPanel = right;
            auto pg = right.reduced (10);
            pg.removeFromTop (20);
            const int pw = pg.getWidth() / 3, ph = juce::jmin (52, pg.getHeight() / 4);
            for (int i = 0; i < 12; ++i)
                pads[(size_t) i]->setBounds (pg.getX() + (i % 3) * pw + 2, pg.getY() + (i / 3) * ph + 2, pw - 4, ph - 4);
        }
        // SHAPE
        {
            auto b = bodyTall;
            shapeSide = b.removeFromRight (250);
            b.removeFromRight (10);
            shapeLanes = b;
            auto s = shapeSide.reduced (12);
            s.removeFromTop (18);
            auto sb = s.removeFromTop (26);
            if (page == shapePage) for (auto& btn : shapeBtn) btn.setBounds (sb.removeFromLeft (sb.getWidth() / 4 + 1));
            s.removeFromTop (8);
            auto r1 = s.removeFromTop (100);
            kDepth2.setBounds (r1.removeFromLeft (r1.getWidth() / 2));
            kSwing2.setBounds (r1);
            s.removeFromTop (10);
            auto r2 = s.removeFromTop (100);
            kPitch.setBounds (r2.removeFromLeft (r2.getWidth() / 2));
            kPanW.setBounds (r2);
            s.removeFromTop (10);
            panLabel = s.removeFromTop (14);
            auto pr = s.removeFromTop (26);
            for (auto& p : panBtn) p.setBounds (pr.removeFromLeft (pr.getWidth() / 3 + 1));
        }
        // FX
        {
            auto b = bodyTall;
            fxFoot = b.removeFromBottom (18);
            const int w = b.getWidth() / 3, h = b.getHeight() / 2;
            Knob* knobs[] = { &kDrive, &kCrush, &kHpf, &kLpf, &kVerb, &kOut };
            for (int i = 0; i < 6; ++i)
            {
                fxBoxes[(size_t) i] = juce::Rectangle<int> (b.getX() + (i % 3) * w, b.getY() + (i / 3) * h, w - 8, h - 8);
                knobs[i]->setBounds (fxBoxes[(size_t) i].withSizeKeepingCentre (130, 150).translated (0, 14));
            }
        }
        // KIT
        {
            auto b = bodyTall;
            kitSide = b.removeFromRight (240);
            b.removeFromRight (10);
            const int cw = b.getWidth() / 6;
            for (int c = 0; c < 6; ++c)
            {
                kitCols[(size_t) c] = juce::Rectangle<int> (b.getX() + c * cw, b.getY(), cw - 6, b.getHeight());
                auto col = kitCols[(size_t) c].reduced (6);
                col.removeFromTop (26);
                const int bh = col.getHeight() / 10;
                for (int i = 0; i < HatLibrary::hatsPerBank; ++i)
                {
                    const size_t idx = (size_t) (c * HatLibrary::hatsPerBank + i);
                    if (idx < kitBtns.size())
                        kitBtns[idx]->setBounds (col.getX() + (i % 2) * (col.getWidth() / 2), col.getY() + (i / 2) * bh,
                                                 col.getWidth() / 2 - 2, bh - 2);
                }
            }
            auto s = kitSide.reduced (14);
            s.removeFromTop (30 + 54 + 14 + 20 + 12);
            audition.setBounds (s.removeFromTop (28)); s.removeFromTop (6);
            loadWav2.setBounds (s.removeFromTop (28)); s.removeFromTop (6);
            useFactory2.setBounds (s.removeFromTop (28));
        }
    }

private:
    HatGoblinEditor& ed;
    HatGoblinProcessor& proc;
    int page = 0, shownBars = 1;
    juce::uint32 followUntil = 0;
    bool goblinWasLaughing = false;
    juce::TextButton tabs[5], prevHat, nextHat, speedBtn[3], monoBtn, latchBtn, chokeBtn, helpBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monoAtt, latchAtt, chokeAtt, withDawAtt;
    GoblinKeyboard keyboard;
    juce::ComboBox hatBox, hatBox2, presetBox, midiForBox;
    juce::TextButton loadWav, useFactory, shapeBtn[4], panBtn[3], audition, loadWav2, useFactory2, namesBtn;
    juce::TextButton playBtn, withDawBtn, backBtn, fwdBtn, lenBtn[3], barBtn[4], generateBtn, varyBtn, loadWavG, rollShapeBtn[4];
    std::vector<std::unique_ptr<juce::TextButton>> styleBtns;
    Knob kTune, kDecay, kVol, kSwing, kHum, kDepth, kRnd, kProb, kPitch, kPanW, kDepth2, kSwing2,
         kDrive, kCrush, kHpf, kLpf, kVerb, kOut,
         kSwingG, kHumG, kProbG, kDepthG, kPitchG, kTuneG, kComplex;
    std::array<std::unique_ptr<PatternPad>, 12> pads;
    std::vector<std::unique_ptr<juce::TextButton>> kitBtns;
    GrooveGrid grid;
    GoblinFace goblin;
    HelpOverlay help;
    DragTile dragAudio, dragMidi, dragHat, keysMidi, keysAudio;
    const HatSample* shownSample = nullptr;
    std::vector<float> peaks;

    juce::Rectangle<int> headerArea, presetLed, syncArea, legendArea, soundPanel, waveArea, latchHint, ledArea, knobPanel,
                         shapeLabel, patternPanel, shapeLanes, shapeSide, panLabel, fxFoot, kitSide,
                         statusArea, nameArea, gridHint, midiNote, feelPanel, rollShapeLabel, hatLabel, genPanel, styleLabel, outPanel;
    std::array<juce::Rectangle<int>, 6> fxBoxes, kitCols;
};

// ============================================================================ editor
HatGoblinEditor::HatGoblinEditor (HatGoblinProcessor& p)
    : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&look);
    logo = juce::ImageCache::getFromMemory (BinaryData::trapwin_logo_png, BinaryData::trapwin_logo_pngSize);

    grain = juce::Image (juce::Image::ARGB, 256, 256, true);
    juce::Random rng (2026);
    for (int y = 0; y < 256; ++y)
        for (int x = 0; x < 256; ++x)
        {
            const auto v = (juce::uint8) rng.nextInt (256);
            grain.setPixelAt (x, y, juce::Colour (v, v, v, (juce::uint8) 255));
        }

    content = std::make_unique<GoblinContent> (*this);
    content->setBounds (0, 0, baseW, baseH);
    addAndMakeVisible (*content);
    neverTakeKeyboard (*this);
    content->setWantsKeyboardFocus (true);      // only for the space bar (PLAY / STOP); all other keys go to the host

    setResizable (true, true);
    setResizeLimits (baseW * 6 / 10, baseH * 6 / 10, baseW * 2, baseH * 2);
    getConstrainer()->setFixedAspectRatio ((double) baseW / baseH);
    setSize (baseW, baseH);
    startTimerHz (30);
}

HatGoblinEditor::~HatGoblinEditor()
{
    stopTimer();
    content.reset();
    setLookAndFeel (nullptr);
}

void HatGoblinEditor::paint (juce::Graphics& g) { g.fillAll (HG::ink); }

void HatGoblinEditor::paintOverChildren (juce::Graphics& g)
{
    g.setOpacity (0.045f);
    g.setTiledImageFill (grain, 0, 0, 0.045f);
    g.fillRect (getLocalBounds());
    if (dragHover)
    {
        g.setColour (HG::red);
        g.drawRect (getLocalBounds(), 3);
    }
}

void HatGoblinEditor::resized()
{
    const float s = getWidth() / (float) baseW;
    content->setTransform (juce::AffineTransform::scale (s));
}

void HatGoblinEditor::timerCallback()
{
    static_cast<GoblinContent*> (content.get())->refresh();
}

void HatGoblinEditor::setParam (const juce::String& id, float plain)
{
    if (auto* p = proc.apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (plain));
        p->endChangeGesture();
    }
}

float HatGoblinEditor::param (const juce::String& id) const
{
    return proc.apvts.getRawParameterValue (id)->load();
}

bool HatGoblinEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg"))
            return true;
    return false;
}

void HatGoblinEditor::filesDropped (const juce::StringArray& files, int, int)
{
    dragHover = false;
    for (auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg") && proc.loadUserSample (juce::File (f)))
        {
            proc.engine.auditionRequest.store (true);
            break;
        }
    repaint();
}

void HatGoblinEditor::chooseUserFile()
{
    chooser = std::make_unique<juce::FileChooser> ("Feed the goblin a hi-hat", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f.existsAsFile() && proc.loadUserSample (f))
                                  proc.engine.auditionRequest.store (true);
                          });
}
