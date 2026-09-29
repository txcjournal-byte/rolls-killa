#pragma once

#include "MetalWidgets.h"

/**
    Rolls Killa Mini skin: the approved design picture (docs/design/mini_metal.webp) is the plugin's face,
    prepared by tools/mini_skin/make_skin.py into Resources/MiniSkin. The widgets below keep the metal
    widgets' behaviour and only draw what moves on top of the picture (lamps, LEDs, digits, the ring,
    the ember glow, press feedback).

    Geometry is given in design pixels (2000 x 733); 1 UI point = 3.333 design pixels.
*/
namespace rk::ui::skin
{

struct Images
{
    juce::Image plate, ring, lampOn, lampOff, dotOn, dotOff, moodOn, moodOff, toggleMidi, toggleHost;
};

/** Loaded once per open window; hold a juce::SharedResourcePointer<SharedImages> while painting. */
class SharedImages
{
public:
    SharedImages();
    const Images& get() const noexcept { return images_; }

private:
    Images images_;
};

/** The images of the open Mini window (valid while a SharedResourcePointer<SharedImages> exists). */
const Images& images();

constexpr float kPx = 600.0f / 2000.0f;

/** Design pixels -> UI points (in the plate's coordinates). */
inline juce::Rectangle<float> design (float x0, float y0, float x1, float y1)
{
    return { x0 * kPx, y0 * kPx, (x1 - x0) * kPx, (y1 - y0) * kPx };
}

inline juce::Point<float> designPoint (float x, float y) { return { x * kPx, y * kPx }; }

/** Darkens/lightens a baked-in button: hover, press, disabled. */
void drawPressOverlay (juce::Graphics&, juce::Rectangle<float>, float corner, bool highlighted, bool down, bool enabled = true);

//==============================================================================
/** Invisible button over a button that is part of the picture. */
class SkinButton : public juce::Button
{
public:
    explicit SkinButton (const juce::String& name, float cornerPx = 4.0f);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    float corner;
};

class SkinKillButton : public KillButton
{
public:
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
};

class SkinDragButton : public DragMidiZone
{
public:
    void paint (juce::Graphics&) override;
};

class SkinPlayButton : public metal::RoundPlayButton
{
public:
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
};

class SkinBpm : public metal::BpmLcd
{
public:
    using BpmLcd::BpmLcd;
    void paint (juce::Graphics&) override;
};

class SkinBars : public metal::BarsLamps
{
public:
    using BarsLamps::BarsLamps;
    void paint (juce::Graphics&) override;
};

class SkinHistory : public metal::HistoryLeds
{
public:
    using HistoryLeds::HistoryLeds;
    void paint (juce::Graphics&) override;

protected:
    juce::Rectangle<float> dot (int i) const override;
};

class SkinMood : public metal::MoodSwitch
{
public:
    void paint (juce::Graphics&) override;

protected:
    float slotX (int i) const override;
};

class SkinHat : public metal::HatPicker
{
public:
    SkinHat (juce::AudioProcessorValueTreeState&, std::function<juce::StringArray()> names);
    void resized() override;
    void paint (juce::Graphics&) override;

protected:
    juce::Rectangle<int> namePlate() const override;
    SkinButton skinPrev { "prev" }, skinNext { "next" };
};

class SkinToggle : public metal::ModeToggle
{
public:
    using ModeToggle::ModeToggle;
    void paint (juce::Graphics&) override;
};

class SkinBlunt : public metal::BluntSlider
{
public:
    using BluntSlider::BluntSlider;
    void paint (juce::Graphics&) override;
    juce::Point<float> getEmberTip() const override;

protected:
    juce::Rectangle<float> body() const override;
    juce::Rectangle<float> track() const override;
    float valueAt (juce::Point<float> p) const override;
    float ringCentreX() const;
    juce::Rectangle<float> local (juce::Rectangle<float> r) const { return r.translated ((float) -getX(), (float) -getY()); }
};

} // namespace rk::ui::skin
