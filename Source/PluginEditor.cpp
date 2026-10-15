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

juce::Font Look::getTextButtonFont (juce::TextButton&, int h) { return Fonts::label (juce::jmin (11.0f, h * 0.45f)); }

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

// ============================================================================ knob
Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& l, std::function<juce::String (float)> fmt)
    : label (l), format (std::move (fmt))
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (180);
    addAndMakeVisible (slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
    slider.onValueChange = [this] { repaint(); };
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

    static juce::Colour roleColour (int n, bool black)
    {
        if (KeyMap::isRate (n)) return red;
        if (KeyMap::isPattern (n)) return boneKey;
        if (n == KeyMap::pitchDown || n == KeyMap::pitchUp || n == KeyMap::latch) return gray;
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
        g.drawText (juce::MidiMessage::getMidiNoteName (n, true, true, 3), r.removeFromBottom (13).toNearestInt(), juce::Justification::centred);
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
    PatternPad (juce::MidiKeyboardState& s, int idx) : state (s), index (idx) {}
    std::function<int()> activeNote;
    int note() const { return KeyMap::firstPattern + index; }

    void paint (juce::Graphics& g) override
    {
        const bool on = activeNote && activeNote() == note();
        auto r = getLocalBounds();
        g.setColour (black);
        g.fillRect (r);
        g.setColour (on ? red : lineSoft);
        g.drawRect (r, 1);
        auto dots = r.reduced (6).removeFromTop (10);
        const auto& pat = KeyMap::pattern (index);
        const float w = dots.getWidth() / 16.0f;
        for (int i = 0; i < 16; ++i)
        {
            const int n = pat[(size_t) i];
            g.setColour (n == 0 ? lineSoft : (on ? red : gray));
            const float hgt = n == 0 ? 3.0f : juce::jmin (10.0f, 3.0f + n * 2.0f);
            g.fillRect (dots.getX() + i * w + 0.5f, (float) dots.getBottom() - hgt, w - 1.0f, hgt);
        }
        g.setColour (on ? red : muted);
        g.setFont (Fonts::label (8.0f));
        g.drawText (juce::MidiMessage::getMidiNoteName (note(), true, true, 3) + " " + KeyMap::patternName (index),
                    r.reduced (6, 4).removeFromBottom (11), juce::Justification::centredLeft);
    }
    void mouseDown (const juce::MouseEvent&) override { state.noteOn (1, note(), 0.9f); }
    void mouseUp (const juce::MouseEvent&) override   { state.noteOff (1, note(), 0.0f); }

private:
    juce::MidiKeyboardState& state;
    int index;
};

// ============================================================================ DRAG MIDI
class DragMidi : public juce::Component
{
public:
    explicit DragMidi (HatGoblinProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }

    juce::String keyName() const
    {
        const int k = proc.lastRollKey();
        return KeyMap::isPattern (k) ? KeyMap::patternName (k - KeyMap::firstPattern) : KeyMap::rateLabel (k);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds();
        g.setColour (dragging ? bone : red);
        g.fillRect (r);
        auto t = r.reduced (14, 10);
        g.setColour (ink);
        g.setFont (Fonts::title (28.0f));
        g.drawText ("DRAG MIDI", t.removeFromTop (30), juce::Justification::left);
        g.setFont (Fonts::label (9.0f).withExtraKerningFactor (0.12f));
        g.drawText ("1 BAR - " + keyName() + " - TO YOUR DAW", t.removeFromTop (14), juce::Justification::left);
        juce::Path arrow;
        const float x = (float) r.getRight() - 30.0f, y = (float) r.getCentreY();
        arrow.startNewSubPath (x - 10, y); arrow.lineTo (x + 10, y);
        arrow.startNewSubPath (x + 3, y - 7); arrow.lineTo (x + 10, y); arrow.lineTo (x + 3, y + 7);
        g.strokePath (arrow, juce::PathStrokeType (2.4f));
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 5) return;
        dragging = true;
        repaint();
        auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                        .getChildFile ("HAT GOBLIN " + keyName().replace ("/", "-") + " "
                                       + juce::String (juce::roundToInt (proc.currentBpm.load())) + "bpm.mid");
        auto midi = RollEngine::renderMidi (proc.lastRollKey(), proc.currentParams());
        file.deleteFile();
        if (auto out = file.createOutputStream())
        {
            midi.writeTo (*out);
            out->flush();
        }
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this,
                                                                    [this] { dragging = false; repaint(); });
    }
    void mouseUp (const juce::MouseEvent&) override { dragging = false; repaint(); }

private:
    HatGoblinProcessor& proc;
    bool dragging = false;
};

// ============================================================================ the whole UI (1100 x 680)
class GoblinContent : public juce::Component
{
public:
    explicit GoblinContent (HatGoblinEditor& e)
        : ed (e), proc (e.proc), keyboard (e.proc.keyboardState),
          kTune (proc.apvts, "tune", "Tune", semis()), kDecay (proc.apvts, "decay", "Decay", pct()),
          kVol (proc.apvts, "volume", "Volume", db()),
          kSwing (proc.apvts, "swing", "Swing", pct()), kHum (proc.apvts, "humanize", "Humanize", pct()),
          kDepth (proc.apvts, "velDepth", "Vel depth", pct()), kRnd (proc.apvts, "velRand", "Vel rnd", pct()),
          kProb (proc.apvts, "prob", "Prob", pct()),
          kPitch (proc.apvts, "pitchRamp", "Pitch ramp", semis()), kPanW (proc.apvts, "panWidth", "Pan width", pct()),
          kDepth2 (proc.apvts, "velDepth", "Vel depth", pct()), kSwing2 (proc.apvts, "swing", "Swing", pct()),
          kDrive (proc.apvts, "drive", "Drive", pct()), kCrush (proc.apvts, "crush", "Amount", pct()),
          kHpf (proc.apvts, "hpf", "High pass", hz()), kLpf (proc.apvts, "lpf", "Low pass", hz()),
          kVerb (proc.apvts, "reverb", "Mix", pct()), kOut (proc.apvts, "out", "Master", db()),
          dragMidi (e.proc)
    {
        const char* tabNames[] = { "ROLL", "SHAPE", "FX", "KIT" };
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
            speedBtn[i].onClick = [this, i] { ed.setParam ("speed", (float) i); };
            addAndMakeVisible (speedBtn[i]);
        }
        for (auto* b : { &monoBtn, &latchBtn, &chokeBtn })
        {
            b->setClickingTogglesState (true);
            addAndMakeVisible (b);
        }
        monoBtn.setButtonText ("MONO"); latchBtn.setButtonText ("LATCH"); chokeBtn.setButtonText ("CHOKE");
        monoAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "mono", monoBtn);
        latchAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "latch", latchBtn);
        chokeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "choke", chokeBtn);
        latchBtn.getProperties().set ("accent", "red");

        // --- ROLL page
        hatBox.setJustificationType (juce::Justification::centredLeft);
        auto banks = HatLibrary::bankNames();
        for (int b = 0; b < banks.size(); ++b)
        {
            hatBox.addSectionHeading (banks[b]);
            for (int i = 0; i < HatLibrary::hatsPerBank; ++i)
            {
                const int idx = b * HatLibrary::hatsPerBank + i;
                if (idx < HatLibrary::get().size())
                    hatBox.addItem (HatLibrary::get().hat (idx).name, idx + 1);
            }
        }
        hatBox.onChange = [this]
        {
            if (hatBox.getSelectedId() > 0)
            {
                ed.setParam ("hat", (float) (hatBox.getSelectedId() - 1));
                ed.setParam ("useUser", 0.0f);
            }
        };
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
            pads[(size_t) i] = std::make_unique<PatternPad> (proc.keyboardState, i);
            pads[(size_t) i]->activeNote = [this] { return proc.engine.displayNote.load(); };
        }
        keyboard.activeNote = [this] { return proc.engine.displayNote.load(); };

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

        addAndMakeVisible (keyboard);
        setPage (0);
        refresh();
    }

    void setPage (int p)
    {
        page = p;
        auto vis = [] (juce::Component& c, bool v, juce::Component* parent)
        {
            if (v && c.getParentComponent() == nullptr) parent->addChildComponent (c);
            c.setVisible (v);
        };
        const bool roll = p == 0, shape = p == 1, fx = p == 2, kit = p == 3;
        for (auto* c : std::initializer_list<juce::Component*> { &hatBox, &kTune, &kDecay, &kVol, &loadWav, &useFactory,
                                                                &kSwing, &kHum, &kDepth, &kRnd, &kProb, &dragMidi })
            vis (*c, roll, this);
        for (auto& s : shapeBtn) vis (s, roll || shape, this);
        for (auto& pd : pads) vis (*pd, roll, this);
        for (auto* c : std::initializer_list<juce::Component*> { &kPitch, &kPanW, &kDepth2, &kSwing2 }) vis (*c, shape, this);
        for (auto& b : panBtn) vis (b, shape, this);
        for (auto* c : std::initializer_list<juce::Component*> { &kDrive, &kCrush, &kHpf, &kLpf, &kVerb, &kOut }) vis (*c, fx, this);
        for (auto& b : kitBtns) vis (*b, kit, this);
        for (auto* c : std::initializer_list<juce::Component*> { &audition, &loadWav2, &useFactory2 }) vis (*c, kit, this);
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
        for (int i = 0; i < 4; ++i) shapeBtn[i].setToggleState (vs == i, juce::dontSendNotification);
        const int pm = (int) ed.param ("panMode");
        for (int i = 0; i < 3; ++i) panBtn[i].setToggleState (pm == i, juce::dontSendNotification);
        const bool user = ed.param ("useUser") > 0.5f && proc.userSampleName().isNotEmpty();
        const int hat = (int) ed.param ("hat");
        hatBox.setSelectedId (user ? 0 : hat + 1, juce::dontSendNotification);
        if (user) hatBox.setText ("YOUR WAV: " + proc.userSampleName(), juce::dontSendNotification);
        useFactory.setEnabled (user); useFactory2.setEnabled (user);
        for (size_t i = 0; i < kitBtns.size(); ++i)
            kitBtns[i]->setToggleState (! user && (int) i == hat, juce::dontSendNotification);

        auto* s = proc.currentSample();
        if (s != shownSample) { shownSample = s; buildPeaks(); }
        for (auto& pd : pads) pd->repaint();
        keyboard.repaint();
        dragMidi.repaint();
        repaint (ledArea);
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

        // preset LED (current sound)
        g.setColour (black);
        g.fillRect (presetLed);
        g.setColour (line);
        g.drawRect (presetLed, 1);
        auto* s = proc.currentSample();
        ledText (g, s != nullptr ? s->name : "--", presetLed.toFloat().reduced (34, 0), 22.0f, red, juce::Justification::centred);

        // sync
        const bool playing = proc.hostPlaying.load();
        g.setColour (playing ? red : redDark);
        g.fillRect (syncArea.getX() + 8, syncArea.getCentreY() - 3, 6, 6);
        smallLabel (g, "SYNC " + juce::String (juce::roundToInt (proc.currentBpm.load())), syncArea.withTrimmedLeft (20), bone, juce::Justification::centredLeft, 10.0f);
        g.setColour (line);
        g.drawRect (syncArea, 1);

        if (page == 0) paintRoll (g);
        if (page == 1) paintShape (g);
        if (page == 2) paintFx (g);
        if (page == 3) paintKit (g);

        // keyboard legend
        auto lg = legendArea;
        auto chip = [&] (juce::Colour c, const juce::String& t, int w)
        {
            auto r = lg.removeFromLeft (w);
            g.setColour (c);
            g.fillRect (r.getX(), r.getCentreY() - 4, 9, 9);
            smallLabel (g, t, r.withTrimmedLeft (14), muted, juce::Justification::centredLeft, 8.5f);
        };
        chip (gray, "Pitch / latch C1-E1", 190);
        chip (boneKey, "Patterns C2-B2", 160);
        chip (red, "Rates 1/2-1/256 C3-C4", 220);
        const float po = proc.engine.pitchOffset.load();
        smallLabel (g, "PITCH " + juce::String (po > 0 ? "+" : "") + juce::String ((int) po) + " ST"
                       + (proc.engine.latchToggled.load() ? "   LATCH KEY ON" : ""),
                    lg, proc.engine.latchToggled.load() ? red : muted, juce::Justification::centredRight, 8.5f);
    }

    void paintRoll (juce::Graphics& g)
    {
        // left
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
            const float decay = ed.param ("decay");
            const float cut = waveArea.getX() + waveArea.getWidth() * decay;
            g.setColour (black.withAlpha (0.7f));
            g.fillRect (juce::Rectangle<float> (cut, (float) waveArea.getY(), waveArea.getRight() - cut, (float) waveArea.getHeight()));
            g.setColour (bone);
            g.fillRect (cut, (float) waveArea.getY(), 1.0f, (float) waveArea.getHeight());
        }
        smallLabel (g, "Drop a WAV anywhere on the plugin", dropHint, gray, juce::Justification::centred, 8.5f);

        // centre LED
        g.setColour (black);
        g.fillRect (ledArea);
        g.setColour (line);
        g.drawRect (ledArea, 1);
        auto led = ledArea.reduced (16, 12);
        const int note = proc.engine.displayNote.load();
        smallLabel (g, note >= 0 ? "Now rolling - " + juce::MidiMessage::getMidiNoteName (note, true, true, 3) : "Hold a key - C2..C4",
                    led.removeFromTop (14));
        juce::String big = "--";
        if (KeyMap::isRate (note)) big = KeyMap::rateLabel (note);
        if (KeyMap::isPattern (note)) big = "P" + juce::String (note - KeyMap::firstPattern + 1);
        ledText (g, big, led.removeFromTop (92).toFloat(), 104.0f, note >= 0 ? red : redDark);
        const char* shapeNames[] = { "FLAT", "RAMP UP", "RAMP DOWN", "ACCENT" };
        juce::String sub = (KeyMap::isPattern (note) ? KeyMap::patternName (note - KeyMap::firstPattern) + " - " : juce::String())
                           + "SWING " + juce::String (juce::roundToInt (ed.param ("swing") * 100)) + "% - VEL "
                           + shapeNames[juce::jlimit (0, 3, (int) ed.param ("velShape"))];
        ledText (g, sub, led.removeFromTop (24).toFloat(), 21.0f, redDim);

        // hit history bars
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
        smallLabel (g, "Patterns - C2..B2", patternPanel.reduced (10).removeFromTop (12));
    }

    void paintShape (juce::Graphics& g)
    {
        auto lanes = shapeLanes;
        const float depth = ed.param ("velDepth"), ramp = ed.param ("pitchRamp"), width = ed.param ("panWidth");
        const int vs = (int) ed.param ("velShape"), pm = (int) ed.param ("panMode");
        RollParams p; p.velShape = vs; p.velDepth = depth;
        auto lane = [&] (const juce::String& name, const juce::String& range, std::function<float (float, int)> fn, bool stepsMode)
        {
            auto r = lanes.removeFromTop ((shapeLanes.getHeight() - 12) / 3);
            lanes.removeFromTop (6);
            panelBox (g, r);
            auto head = r.removeFromLeft (120);
            g.setColour (bone);
            g.setFont (Fonts::title (24.0f));
            g.drawText (name, head.reduced (10, 8).removeFromTop (26), juce::Justification::left);
            smallLabel (g, range, head.reduced (10, 8).withTrimmedTop (30).removeFromTop (12), muted, juce::Justification::left, 8.5f);
            auto plot = r.reduced (8);
            g.setColour (black);
            g.fillRect (plot);
            scanlines (g, plot);
            g.setColour (lineSoft);
            for (int i = 1; i < 4; ++i) g.fillRect (plot.getX() + plot.getWidth() * i / 4, plot.getY(), 1, plot.getHeight());
            juce::Path path;
            const int steps = 16;
            for (int i = 0; i < steps; ++i)
            {
                const float v = juce::jlimit (0.0f, 1.0f, fn (i / (float) steps, i));
                const float x0 = plot.getX() + plot.getWidth() * i / (float) steps;
                const float x1 = plot.getX() + plot.getWidth() * (i + 1) / (float) steps;
                const float y = plot.getBottom() - v * plot.getHeight();
                if (stepsMode)
                {
                    g.setColour (red.withAlpha (0.85f));
                    g.fillRect (juce::Rectangle<float> (x0 + 2, y, x1 - x0 - 4, plot.getBottom() - y));
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
        lane ("VELOCITY", "1 beat - per hit", [&] (float t, int i) { return shapeVel (p, t, i % 4 == 0); }, true);
        lane ("PITCH", juce::String (ramp, 1) + " st over 1 beat", [&] (float t, int) { return 0.5f + ramp * t / 24.0f; }, false);
        lane ("PAN", pm == 0 ? "center" : (pm == 1 ? "ping-pong" : "sweep"), [&] (float t, int i)
        {
            if (pm == 1) return 0.5f + ((i & 1) ? 0.5f : -0.5f) * width;
            if (pm == 2) return 0.5f + 0.5f * width * std::sin (juce::MathConstants<float>::twoPi * t);
            return 0.5f;
        }, pm == 1);
        panelBox (g, shapeSide);
        smallLabel (g, "Velocity shape", shapeSide.reduced (12).removeFromTop (12));
        smallLabel (g, "Pan mode", panLabel);
    }

    static float shapeVel (const RollParams& p, float t, bool onBeat)
    {
        float v = 1.0f;
        if (p.velShape == 1) v = 0.3f + 0.7f * t;
        if (p.velShape == 2) v = 1.0f - 0.7f * t;
        if (p.velShape == 3) v = onBeat ? 1.0f : 0.55f;
        return juce::jmap (p.velDepth, 1.0f, v);
    }

    void paintFx (juce::Graphics& g)
    {
        const char* names[] = { "SATURATE", "CRUSH", "HIGH PASS", "LOW PASS", "REVERB", "OUTPUT" };
        for (int i = 0; i < 6; ++i)
        {
            panelBox (g, fxBoxes[(size_t) i]);
            g.setColour (bone);
            g.setFont (Fonts::title (24.0f));
            g.drawText (names[i], fxBoxes[(size_t) i].reduced (14, 10).removeFromTop (28), juce::Justification::left);
            smallLabel (g, juce::String ("0") + juce::String (i + 1), fxBoxes[(size_t) i].reduced (14, 10).removeFromTop (28), gray, juce::Justification::right, 9.0f);
        }
        smallLabel (g, "Chain: saturate > crush > filters > reverb > output - 0 ms latency", fxFoot, muted, juce::Justification::centredLeft, 8.5f);
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
        g.drawFittedText ("Drop your own WAV, AIFF, FLAC or MP3 anywhere on the plugin. The rolls play your sound.",
                          r.removeFromTop (54), juce::Justification::topLeft, 4);
        const auto name = proc.userSampleName();
        smallLabel (g, "Your WAV", r.removeFromTop (14));
        g.setColour (name.isNotEmpty() ? bone : gray);
        g.setFont (Fonts::label (12.0f));
        g.drawText (name.isNotEmpty() ? name : "- none loaded -", r.removeFromTop (20), juce::Justification::left);
    }

    void resized() override
    {
        headerArea = { 0, 0, getWidth(), 54 };
        int x = 280;
        for (auto& t : tabs) { t.setBounds (x, 14, t.getButtonText().length() * 9 + 26, 26); x += t.getWidth() - 1; }
        presetLed = { x + 16, 11, 250, 32 };
        prevHat.setBounds (presetLed.getX(), presetLed.getY(), 30, presetLed.getHeight());
        nextHat.setBounds (presetLed.getRight() - 30, presetLed.getY(), 30, presetLed.getHeight());
        syncArea = { presetLed.getRight() + 12, 14, 80, 26 };
        int sx = syncArea.getRight() + 8;
        for (auto& b : speedBtn) { b.setBounds (sx, 14, 38, 26); sx += 37; }
        sx += 8;
        monoBtn.setBounds (sx, 14, 52, 26); sx += 51;
        latchBtn.setBounds (sx, 14, 58, 26); sx += 57;
        chokeBtn.setBounds (sx, 14, 60, 26);

        // footer: legend + keyboard
        auto foot = juce::Rectangle<int> (12, getHeight() - 122, getWidth() - 24, 116);
        legendArea = foot.removeFromTop (18);
        keyboard.setBounds (foot);
        keyboard.setKeyWidth (foot.getWidth() / 22.0f);

        auto body = juce::Rectangle<int> (12, 64, getWidth() - 24, getHeight() - 64 - 130);

        // ROLL
        {
            auto b = body;
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
            dropHint = sp.removeFromTop (22);

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
            for (auto& s : shapeBtn) { s.setBounds (sb.removeFromLeft (52)); }

            dragMidi.setBounds (right.removeFromBottom (78));
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
            auto b = body;
            shapeSide = b.removeFromRight (250);
            b.removeFromRight (10);
            shapeLanes = b;
            auto s = shapeSide.reduced (12);
            s.removeFromTop (18);
            auto sb = s.removeFromTop (26);
            if (page == 1) for (auto& btn : shapeBtn) btn.setBounds (sb.removeFromLeft (sb.getWidth() / 4 + 1));
            s.removeFromTop (8);
            auto r1 = s.removeFromTop (90);
            kDepth2.setBounds (r1.removeFromLeft (r1.getWidth() / 2));
            kSwing2.setBounds (r1);
            s.removeFromTop (10);
            auto r2 = s.removeFromTop (90);
            kPitch.setBounds (r2.removeFromLeft (r2.getWidth() / 2));
            kPanW.setBounds (r2);
            s.removeFromTop (10);
            panLabel = s.removeFromTop (14);
            auto pr = s.removeFromTop (26);
            for (auto& p : panBtn) p.setBounds (pr.removeFromLeft (pr.getWidth() / 3 + 1));
        }
        // FX
        {
            auto b = body;
            fxFoot = b.removeFromBottom (18);
            const int w = b.getWidth() / 3, h = b.getHeight() / 2;
            Knob* knobs[] = { &kDrive, &kCrush, &kHpf, &kLpf, &kVerb, &kOut };
            for (int i = 0; i < 6; ++i)
            {
                fxBoxes[(size_t) i] = juce::Rectangle<int> (b.getX() + (i % 3) * w, b.getY() + (i / 3) * h, w - 8, h - 8);
                knobs[i]->setBounds (fxBoxes[(size_t) i].withSizeKeepingCentre (120, 140).translated (0, 14));
            }
        }
        // KIT
        {
            auto b = body;
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
    int page = 0;
    juce::TextButton tabs[4], prevHat, nextHat, speedBtn[3], monoBtn, latchBtn, chokeBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monoAtt, latchAtt, chokeAtt;
    GoblinKeyboard keyboard;
    juce::ComboBox hatBox;
    juce::TextButton loadWav, useFactory, shapeBtn[4], panBtn[3], audition, loadWav2, useFactory2;
    Knob kTune, kDecay, kVol, kSwing, kHum, kDepth, kRnd, kProb, kPitch, kPanW, kDepth2, kSwing2,
         kDrive, kCrush, kHpf, kLpf, kVerb, kOut;
    std::array<std::unique_ptr<PatternPad>, 12> pads;
    std::vector<std::unique_ptr<juce::TextButton>> kitBtns;
    DragMidi dragMidi;
    const HatSample* shownSample = nullptr;
    std::vector<float> peaks;

    juce::Rectangle<int> headerArea, presetLed, syncArea, legendArea, soundPanel, waveArea, dropHint, ledArea, knobPanel,
                         shapeLabel, patternPanel, shapeLanes, shapeSide, panLabel, fxFoot, kitSide;
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
