#pragma once

#include "PluginProcessor.h"
#include "ui/KillaLookAndFeel.h"
#include "ui/MetalWidgets.h"
#include "ui/RollVisualizer.h"

/**
    Rolls Killa Mini: one small plate of scratched steel (docs/design/mini_metal.webp) -
    BPM (follows the DAW), undo/redo, BARS, the roll window, KILL history lights,
    CHILL/TRAP/CRAZY + KILL, hi-hat, the blunt (PUFF smoke FX), DRAG TO DAW and PLAY.
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
    bool keyPressed (const juce::KeyPress&) override;

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
    juce::Image steel;
    float steelScale = 0.0f;
    float uiScale = 1.0f;

    rk::ui::RollVisualizer visualizer;
    rk::ui::metal::BpmLcd bpm { proc };
    rk::ui::metal::MetalButton undoButton { "Undo", rk::ui::Icon::undo }, redoButton { "Redo", rk::ui::Icon::redo };
    rk::ui::metal::BarsLamps bars;
    rk::ui::metal::HistoryLeds history { proc };
    rk::ui::metal::MoodSwitch mood;
    rk::ui::metal::MetalKillButton killButton;
    rk::ui::metal::HatPicker hatPicker;
    rk::ui::metal::BluntSlider blunt;
    rk::ui::metal::DragDawButton dragButton;
    rk::ui::metal::RoundPlayButton playButton;
    rk::ui::metal::ModeToggle playMode;
    rk::ui::metal::SmokeOverlay smoke;
    juce::TooltipWindow tooltips { this, 600 };

    int shownHat = -1;
    int frame = 0;
    juce::String shownPresetLine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RollsKillaMiniEditor)
};
