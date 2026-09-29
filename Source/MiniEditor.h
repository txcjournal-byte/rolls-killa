#pragma once

#include "PluginProcessor.h"
#include "ui/KillaLookAndFeel.h"
#include "ui/RollVisualizer.h"
#include "ui/SkinWidgets.h"

/**
    Rolls Killa Mini: the approved metal plate picture (docs/design/mini_metal.webp) is the face -
    BPM (follows the DAW), undo/redo, BARS, the roll window, KILL history lights,
    CHILL/TRAP/CRAZY + KILL, hi-hat, the blunt (PUFF smoke FX), DRAG TO DAW and PLAY.
    The window never takes the keyboard, so the DAW's shortcuts (space = play) keep working.
    Same processor and engine as Rolls Killa.
*/
class RollsKillaMiniEditor : public juce::AudioProcessorEditor,
                             public juce::DragAndDropContainer,
                             private juce::Timer
{
public:
    explicit RollsKillaMiniEditor (RollsKillaProcessor&);
    ~RollsKillaMiniEditor() override;

    void paint (juce::Graphics&) override {}
    void resized() override;

    static constexpr int kBaseWidth = 600;
    static constexpr int kBaseHeight = 220;

private:
    class Content : public juce::Component
    {
    public:
        explicit Content (RollsKillaMiniEditor& e) : editor (e) { setOpaque (true); }
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        RollsKillaMiniEditor& editor;
    };

    void timerCallback() override;
    void layout();
    void setUiScale (float scale);
    void updateStatus();
    void showScaleMenu();

    RollsKillaProcessor& proc;
    rk::ui::KillaLookAndFeel lookAndFeel;
    Content content { *this };
    float uiScale = 1.0f;

    rk::ui::RollVisualizer visualizer;
    rk::ui::skin::SkinBpm bpm { proc };
    rk::ui::skin::SkinButton undoButton { "Undo" }, redoButton { "Redo" };
    rk::ui::skin::SkinBars bars;
    rk::ui::skin::SkinHistory history { proc };
    rk::ui::skin::SkinMood mood;
    rk::ui::skin::SkinKillButton killButton;
    rk::ui::skin::SkinHat hatPicker;
    rk::ui::skin::SkinBlunt blunt;
    rk::ui::skin::SkinDragButton dragButton;
    rk::ui::skin::SkinPlayButton playButton;
    rk::ui::skin::SkinToggle playMode;
    rk::ui::metal::SmokeOverlay smoke;
    juce::TooltipWindow tooltips { this, 600 };

    int shownHat = -1;
    int frame = 0;
    juce::String shownPresetLine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RollsKillaMiniEditor)
};
