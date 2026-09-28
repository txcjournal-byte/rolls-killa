#pragma once

#include "Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>

namespace rk
{
struct HatSample;
}

namespace rk::ui
{

enum class Icon { prev, next, undo, redo, play, stop, star, starFilled, folder, lock, unlock, close, search, exportFile, midiDrag, chevronDown };

void drawIcon (juce::Graphics& g, Icon icon, juce::Rectangle<float> area, juce::Colour colour);

//==============================================================================
/** Small square button with a vector icon. */
class IconButton : public juce::Button
{
public:
    IconButton (const juce::String& name, Icon icon, bool drawFrame = true);
    void setIcon (Icon newIcon) { icon = newIcon; repaint(); }
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

    juce::String label;              // optional text right of the icon
    juce::Colour iconColour = colours::text;

private:
    Icon icon;
    bool frame;
};

//==============================================================================
/** Rotary knob with a title above and a value box below. */
class Knob : public juce::Component, public juce::SettableTooltipClient
{
public:
    Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& title, bool small = false);
    void resized() override;
    void paint (juce::Graphics&) override;

    juce::Slider slider;

private:
    juce::String title;
    bool small;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

//==============================================================================
/** Row of mutually exclusive buttons bound to a choice parameter. */
class SegmentedChoice : public juce::Component, public juce::SettableTooltipClient
{
public:
    SegmentedChoice (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::StringArray& labels);
    void resized() override;

private:
    void update (float value);

    juce::OwnedArray<juce::TextButton> buttons;
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
};

//==============================================================================
/** Pill toggle switch (CHOKE). */
class ToggleSwitch : public juce::ToggleButton
{
public:
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
};

//==============================================================================
/** The big KILL button. */
class KillButton : public juce::Button
{
public:
    KillButton();
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::String seedText;
    std::function<void()> onBackToOriginal;
};

//==============================================================================
/** Drag the current pattern out of the plugin as a .mid file. */
class DragMidiZone : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<juce::File()> createFile;
    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; repaint(); }

private:
    bool dragging = false;
};

//==============================================================================
/** Waveform of the selected hat. Drop a WAV/AIFF on it to load a custom hat. */
class WaveformView : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    void setSample (std::shared_ptr<const HatSample> s);
    void paint (juce::Graphics&) override;
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragOver = false; repaint(); }
    void filesDropped (const juce::StringArray& files, int, int) override;

    std::function<void (const juce::File&)> onFileDropped;

private:
    std::shared_ptr<const HatSample> sample;
    bool dragOver = false;
};

//==============================================================================
/** "< Killa Hat 03 >" selector bound to the hat choice parameter. */
class HatSelector : public juce::Component
{
public:
    HatSelector (juce::AudioProcessorValueTreeState& state, std::function<juce::StringArray()> names);
    void resized() override;
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    int getIndex() const noexcept { return index; }
    std::function<void()> onChange;

private:
    void step (int delta);
    void setIndex (int newIndex);

    IconButton prev { "prev", Icon::prev, false }, next { "next", Icon::next, false };
    std::function<juce::StringArray()> getNames;
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    int index = 0;
};

} // namespace rk::ui
