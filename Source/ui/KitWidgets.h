#pragma once

#include "MetalWidgets.h"
#include "PresetBar.h"
#include "RollVisualizer.h"
#include "../engine/DrumSynth.h"

class RollsKillaProcessor;

/**
    Rolls Killa "KILL STATION" (main window): dark scratched steel, ember-orange and blood-red lights.
    Eight rubber pads (the drawers), the whole beat in lanes, the selected drawer's sound and pattern,
    and the kit (kept sounds, export).
*/
namespace rk::ui::station
{

juce::Colour drawerColour (int type);
/** Light text stamped into dark steel (with a soft black shadow). */
void drawStamped (juce::Graphics&, const juce::String&, juce::Font, juce::Rectangle<float>, juce::Justification, juce::Colour = juce::Colour (0xffd9d4ce));
/** Dark steel plate (the station's body). */
juce::Image renderDarkSteel (int width, int height, float scale);
/** Recessed dark panel with a stamped title. */
void drawBay (juce::Graphics&, juce::Rectangle<float>, const juce::String& title);

//==============================================================================
/** Chunky steel knob with an ember arc and the value while dragging. */
class MetalKnob : public juce::Slider
{
public:
    explicit MetalKnob (const juce::String& label);
    void paint (juce::Graphics&) override;
    juce::String label;
    juce::Colour accent = metal::colours::ember;
};

/** Row of small steel buttons, one lit (styles, bars, roll speed). */
class MetalChoice : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit MetalChoice (const juce::StringArray& labels);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void setLabels (const juce::StringArray& labels);
    void setSelected (int index) { if (index != selected) { selected = index; repaint(); } }
    int getSelected() const noexcept { return selected; }
    std::function<void (int)> onChange;

private:
    juce::StringArray labels;
    int selected = 0;
};

/** MetalChoice bound to a choice / int parameter. */
class ParamChoice : public MetalChoice
{
public:
    ParamChoice (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::StringArray& labels);

private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
};

/** Text button in steel or blood red (KILL ...). */
class StationButton : public juce::Button
{
public:
    StationButton (const juce::String& buttonText, bool red = false);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    bool red;
    bool lit = false;
};

/** The big KILL in the header: click = KILL BEAT, right-click = KILL KIT / KILL EVERYTHING. */
class KillBeatButton : public metal::MetalKillButton
{
public:
    void mouseDown (const juce::MouseEvent&) override;
    std::function<void()> onKillKit, onKillAll;
};

/** Grab it and drag MIDI into the DAW (station style). */
class StationDrag : public DragMidiZone
{
public:
    explicit StationDrag (const juce::String& text) : label (text) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }
    void paint (juce::Graphics&) override;
    juce::String label;
};

//==============================================================================
/** One drawer as a rubber MPC pad: click = select + play, drop a WAV = your sound, right-click = menu. */
class Pad : public juce::Component, public juce::FileDragAndDropTarget, public juce::SettableTooltipClient
{
public:
    Pad (RollsKillaProcessor&, int type);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragOver = false; repaint(); }
    void filesDropped (const juce::StringArray&, int, int) override;

    /** Called by the editor's timer: flashes on hits, refreshes the waveform when the sound changes. */
    void tick();
    bool selected = false;
    std::function<void (int)> onSelect;
    std::function<void()> onChanged;
    std::function<void (int)> onLoadWav;

private:
    RollsKillaProcessor& proc;
    int type;
    StationButton killButton { "KILL", true };
    int lastHits = 0;
    float flash = 0.0f;
    bool dragOver = false;
    const DrumSound* shownSound = nullptr;
    juce::Path wave;
};

//==============================================================================
/** The whole beat: one lane per drawer, the playhead, click a lane = select, click the LED = pattern on/off. */
class BeatView : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit BeatView (RollsKillaProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void tick();
    int selected = 0;
    std::function<void (int)> onSelect;
    std::function<void()> onChanged;

private:
    juce::Rectangle<float> lane (int i) const;
    RollsKillaProcessor& proc;
    double shownPlayhead = -2.0;
    int shownVersion = -1;
};

//==============================================================================
/** SOUND of the selected drawer: knobs, KILL SOUND, LOAD WAV, KEEP. */
class SoundPanel : public juce::Component
{
public:
    explicit SoundPanel (RollsKillaProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void setDrawer (int type);
    void refresh();
    std::function<void()> onChanged;
    std::function<void()> onLoadWav;

private:
    RollsKillaProcessor& proc;
    int type = 0;
    MetalKnob tune { "TUNE" }, decay { "DECAY" }, punch { "PUNCH" }, drive { "DRIVE" }, tone { "TONE" }, body { "BODY" }, volume { "VOL" };
    StationButton killButton { "KILL SOUND", true }, loadButton { "LOAD WAV" }, keepButton { "KEEP" };
};

/** PATTERN of the selected drawer (kit drawers): style, bars, density, on/off, KILL PATTERN, drag MIDI. */
class PatternPanel : public juce::Component
{
public:
    explicit PatternPanel (RollsKillaProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void setDrawer (int type);
    void refresh();
    std::function<void()> onChanged;

private:
    RollsKillaProcessor& proc;
    int type = 0;
    MetalChoice style { { "A", "B", "C", "D" } };
    MetalChoice bars { { "1", "2", "4", "8" } };
    MetalKnob density { "DENSITY" };
    StationButton onButton { "ON" }, killButton { "KILL PATTERN", true };
    StationDrag drag { "DRAG MIDI" };
};

/** PATTERN of the hi-hat drawer = the Rolls Killa roll engine: presets, KILL ROLL, roll knobs. */
class HatPanel : public juce::Component
{
public:
    explicit HatPanel (RollsKillaProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();
    std::function<void (int)> onOpenBrowser;
    std::function<void()> onChanged;

private:
    RollsKillaProcessor& proc;
    PresetBar presetBar;
    ParamChoice speed;
    MetalKnob density { "DENSITY" }, swing { "SWING" }, variation { "VARY" }, pitch { "PITCH" };
    juce::AudioProcessorValueTreeState::SliderAttachment densityA, swingA, variationA, pitchA;
    StationButton killButton { "KILL ROLL", true };
    StationDrag drag { "DRAG MIDI" };
};

//==============================================================================
/** The kit: its name, the kept sounds (click = play, x = remove), EXPORT KIT, ONE SHOT KIT, DRAG BEAT. */
class KitPanel : public juce::Component
{
public:
    explicit KitPanel (RollsKillaProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void refresh();
    void showMessage (const juce::String& text, const juce::File& folder = {});

    std::function<void()> onExportKit;
    std::function<void (int count)> onExportOneShots;
    std::function<void()> onChanged;
    int selectedDrawer = 0;

private:
    juce::Rectangle<float> chipArea() const;
    std::vector<std::pair<int, juce::Rectangle<float>>> chipRects() const;

    RollsKillaProcessor& proc;
    juce::TextEditor nameEditor;
    StationButton exportButton { "EXPORT KIT" }, oneShotButton { "ONE SHOT KIT" }, killKitButton { "KILL KIT", true };
    MetalChoice oneShotCount { { "10", "25", "50", "100" } };
    StationButton revealButton { "SHOW" };
    StationDrag dragBeat { "DRAG BEAT" };
    juce::String message;
    juce::File messageFolder;
    int hoverChip = -1;
};

} // namespace rk::ui::station
