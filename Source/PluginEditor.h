#pragma once

#include "PluginProcessor.h"
#include "ui/KillaLookAndFeel.h"
#include "ui/KitWidgets.h"
#include "ui/PresetBrowser.h"
#include "ui/RollVisualizer.h"

/**
    ROLLS KILLA by TrapVST - "KILL STATION": a drum & roll factory.
    Eight drawers (808, kick, snare, clap, hi-hat, open hat, perc, FX) as pads; every drawer has a sound
    (synthesized - KILL = a new one - or your WAV) and a pattern; the hi-hat drawer is the Rolls Killa roll engine.
    The beat plays with the DAW (or PLAY / SPACE), drags into FL as MIDI, and the kit exports as a folder of WAVs.
*/
class RollsKillaEditor : public juce::AudioProcessorEditor,
                         public juce::DragAndDropContainer,
                         private juce::Timer
{
public:
    explicit RollsKillaEditor (RollsKillaProcessor&);
    ~RollsKillaEditor() override;

    void paint (juce::Graphics&) override {}
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    static constexpr int kBaseWidth = 1040;
    static constexpr int kBaseHeight = 636;

private:
    class Content : public juce::Component
    {
    public:
        explicit Content (RollsKillaEditor& e) : editor (e) { setOpaque (true); }
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        RollsKillaEditor& editor;
    };

    void timerCallback() override;
    void layout();
    void setUiScale (float scale);
    void showScaleMenu();
    void selectDrawer (int type);
    void showRoll (bool roll);
    void refreshAll();
    void updateStatus();
    void loadWavInto (int type);
    void exportKit();
    void exportOneShots (int count);

    RollsKillaProcessor& proc;
    rk::ui::KillaLookAndFeel lookAndFeel;
    Content content { *this };
    juce::Image plate, logo;
    float plateScale = 0.0f;
    float uiScale = 1.0f;

    // header
    rk::ui::metal::BpmLcd bpm { proc };
    rk::ui::station::MetalChoice mood { { "CHILL", "TRAP", "CRAZY" } };
    rk::ui::metal::MetalButton undoButton { "Undo", rk::ui::Icon::undo }, redoButton { "Redo", rk::ui::Icon::redo };
    rk::ui::station::KillBeatButton killBeat;
    rk::ui::station::StationButton playButton { "PLAY" };
    rk::ui::station::ParamChoice playMode;
    rk::ui::station::MetalKnob smoke { "SMOKE" };
    juce::AudioProcessorValueTreeState::SliderAttachment smokeAttachment;

    // body
    juce::OwnedArray<rk::ui::station::Pad> pads;
    rk::ui::station::BeatView beatView { proc };
    rk::ui::RollVisualizer rollView;
    rk::ui::station::StationButton beatTab { "BEAT" }, rollTab { "HI-HAT ROLL" };
    rk::ui::station::SoundPanel soundPanel { proc };
    rk::ui::station::PatternPanel patternPanel { proc };
    rk::ui::station::HatPanel hatPanel { proc };
    rk::ui::station::KitPanel kitPanel { proc };

    rk::ui::PresetBrowser browser;
    juce::TooltipWindow tooltips { this, 600 };
    std::unique_ptr<juce::FileChooser> fileChooser;

    int selected = 0;
    bool rollShown = false;
    int shownKitVersion = -1, shownPatternVersion = -1;
    int frame = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RollsKillaEditor)
};
