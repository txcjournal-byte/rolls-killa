#pragma once

#include "Widgets.h"

class RollsKillaProcessor;

/**
    Rolls Killa Mini "metal plate" look (docs/design/mini_metal.webp): one piece of scratched steel,
    engraved labels, ember-orange lights and a blunt as the PUFF slider.
*/
namespace rk::ui::metal
{

namespace colours
{
    inline const juce::Colour ember     { 0xffff7a1a };
    inline const juce::Colour emberHot  { 0xffffc36a };
    inline const juce::Colour emberDeep { 0xffb8300a };
    inline const juce::Colour engrave   { 0xff151515 };
    inline const juce::Colour steel     { 0xff8a8a86 };
    inline const juce::Colour steelDark { 0xff3c3c3a };
    inline const juce::Colour glass     { 0xff0a0807 };
}

/** Brushed, scratched steel texture (generated once, 'scale' = pixels per UI point). */
juce::Image renderSteel (int width, int height, float scale);

void drawScrew (juce::Graphics&, juce::Point<float> centre, float radius, float angle);
void drawEngravedText (juce::Graphics&, const juce::String&, juce::Font, juce::Rectangle<float>, juce::Justification, float alpha = 1.0f);
/** Dark recessed window cut into the plate (LCD, visualizer slot, waveform). */
void drawRecess (juce::Graphics&, juce::Rectangle<float>, float corner);
/** Vertical milled groove between sections. */
void drawGroove (juce::Graphics&, float x, float y0, float y1);
/** Steel cap (buttons). */
void drawCap (juce::Graphics&, juce::Rectangle<float>, float corner, bool highlighted, bool down);
void drawLed (juce::Graphics&, juce::Point<float> centre, float radius, float amount);
/** Engraved label font. */
juce::Font plateFont (float height, float kerning = 0.12f);

//==============================================================================
/** Square steel button with an icon (undo, redo, prev, next). */
class MetalButton : public juce::Button
{
public:
    MetalButton (const juce::String& name, Icon icon);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    Icon icon;
};

//==============================================================================
/** BARS 1 2 4 8 - round steel buttons, the selected one lights up. */
class BarsLamps : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit BarsLamps (juce::AudioProcessorValueTreeState&);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> lamp (int i) const;
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    int index = 2;
};

//==============================================================================
/** CHILL / TRAP / CRAZY - a three-position lever switch. */
class MoodSwitch : public juce::Component, public juce::SettableTooltipClient
{
public:
    MoodSwitch();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent& e) override { mouseDown (e); }
    void setMood (int m) { if (m != mood) { mood = m; repaint(); } }
    std::function<void (int)> onChange;

private:
    float slotX (int i) const;
    int mood = 1;
};

//==============================================================================
/** The red KILL push button in a steel bezel (keeps KillButton's right-click menu). */
class MetalKillButton : public KillButton
{
public:
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
};

//==============================================================================
/** Orange waveform in a black window, drop a WAV on it. */
class WaveWindow : public WaveformView, public juce::SettableTooltipClient
{
public:
    void paint (juce::Graphics&) override;
};

/** < waveform > + the hat's name on a small plate (click = list). */
class HatPicker : public juce::Component
{
public:
    HatPicker (juce::AudioProcessorValueTreeState&, std::function<juce::StringArray()> names);
    void resized() override;
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    int getIndex() const noexcept { return index; }

    WaveWindow wave;
    std::function<void()> onChange;

private:
    juce::Rectangle<int> namePlate() const;
    void step (int delta);

    MetalButton prev { "prev", Icon::prev }, next { "next", Icon::next };
    std::function<juce::StringArray()> getNames;
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    int index = 0;
};

//==============================================================================
/** DRAG TO DAW - a steel button you drag out of the plugin. */
class DragDawButton : public DragMidiZone
{
public:
    void paint (juce::Graphics&) override;
};

//==============================================================================
/** Round PLAY button ("PLAY" / "STOP" while previewing / "SYNC" while the host plays). */
class RoundPlayButton : public juce::Button
{
public:
    RoundPlayButton() : juce::Button ("Play") { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    bool previewing = false, hostPlaying = false;
};

/** MIDI | HOST slide switch bound to the play mode parameter. */
class ModeToggle : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ModeToggle (juce::AudioProcessorValueTreeState&);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    int mode = 0;
};

//==============================================================================
/** Orange LCD "140 BPM" + AUTO/SET and a SYNC LED blinking on the beat.
    Drag up/down = own tempo, double-click = AUTO (follow the DAW). */
class BpmLcd : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit BpmLcd (RollsKillaProcessor& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    RollsKillaProcessor& proc;
    double startBpm = 140.0;
};

//==============================================================================
/** The last 8 KILLs as LEDs - click one to go back to that roll. */
class HistoryLeds : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit HistoryLeds (RollsKillaProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> dot (int i) const;
    RollsKillaProcessor& proc;
    int hover = -1;
};

//==============================================================================
/**
    PUFF: a rolled blunt lying on the plate, the steel ring on it is the slider handle.
    0 = unlit, the further right the ring, the harder the ember glows and the more smoke.
    Drag anywhere on the blunt or on the track, double-click = 0.
*/
class BluntSlider : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit BluntSlider (juce::AudioProcessorValueTreeState&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    /** 0..1 from the audio thread's smoke FX (ember pulse). */
    void setGlow (float g);
    float getValue() const noexcept { return value; }
    /** Where the smoke comes out, in this component's coordinates. */
    juce::Point<float> getEmberTip() const;

private:
    juce::Rectangle<float> body() const;
    juce::Rectangle<float> track() const;
    float handleX() const;
    float valueAt (juce::Point<float> p) const;
    void paintBlunt (juce::Graphics&, juce::Rectangle<float> b, float heat);

    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    float value = 0.0f, glow = 0.0f;
    juce::Image bodyCache;
    juce::Path bodyPath;
    float cacheScale = 0.0f;
};

/** Smoke rising from the ember (drawn over the plate, ignores the mouse). */
class SmokeOverlay : public juce::Component
{
public:
    SmokeOverlay();
    void paint (juce::Graphics&) override;
    /** Advance the particles; 'origin' in this component's coordinates. */
    void tick (float dtSeconds, float amount, float glow, juce::Point<float> origin);

private:
    struct Puff { float x, y, vx, vy, age, life, size, phase; };
    std::array<Puff, 48> puffs {};
    int count = 0;
    float spawnAccumulator = 0.0f;
    juce::Point<float> glowAt;
    float glowAmount = 0.0f;
    juce::Random rng { 42 };
};

} // namespace rk::ui::metal
