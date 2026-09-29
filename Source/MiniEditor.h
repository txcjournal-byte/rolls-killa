#pragma once

#include "PluginProcessor.h"
#include "ui/KillaLookAndFeel.h"
#include "ui/RollVisualizer.h"
#include "ui/Widgets.h"

/**
    Rolls Killa Mini: one thin strip - visualizer, KILL (random preset + variation + knob shuffle),
    hi-hat sample, DRAG MIDI and PLAY. Same processor and engine as Rolls Killa.
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
    static constexpr int kBaseHeight = 212;

private:
    class Content : public juce::Component
    {
    public:
        explicit Content (RollsKillaMiniEditor& e) : editor (e) {}
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        RollsKillaMiniEditor& editor;
    };

    /** Big play triangle + "PLAY" (STOP while previewing, SYNC while the host plays). */
    class PlayTile : public juce::Button
    {
    public:
        PlayTile() : juce::Button ("Play") { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
        void paintButton (juce::Graphics&, bool highlighted, bool down) override;
        bool previewing = false, hostPlaying = false;
    };

    /** "135 BPM" - follows the host (AUTO); drag up/down to set a tempo, double-click = AUTO again. */
    class BpmBox : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        explicit BpmBox (RollsKillaProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::UpDownResizeCursor); }
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override { startBpm = proc.getEffectiveBpm(); }
        void mouseDrag (const juce::MouseEvent& e) override;
        void mouseDoubleClick (const juce::MouseEvent&) override { proc.setTargetBpm (0.0); repaint(); }
        RollsKillaProcessor& proc;
        double startBpm = 140.0;
    };

    /** The last KILLs as dots - click one to go back to that roll. */
    class HistoryStrip : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        explicit HistoryStrip (RollsKillaProcessor& p) : proc (p) {}
        void paint (juce::Graphics&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
        void mouseUp (const juce::MouseEvent&) override;
        juce::Rectangle<float> dotBounds (int i, int count) const;
        RollsKillaProcessor& proc;
        int hover = -1;
    };

    void timerCallback() override;
    void setMood (int mood);
    void layout();
    void setUiScale (float scale);
    void updateStatus();
    void showScaleMenu();

    RollsKillaProcessor& proc;
    rk::ui::KillaLookAndFeel lookAndFeel;
    Content content { *this };
    juce::Image logo;
    float uiScale = 1.0f;

    rk::ui::RollVisualizer visualizer;
    rk::ui::SegmentedChoice barsChoice;
    rk::ui::IconButton undoButton { "Undo", rk::ui::Icon::undo }, redoButton { "Redo", rk::ui::Icon::redo };
    rk::ui::KillButton killButton;
    rk::ui::WaveformView waveform;
    rk::ui::HatSelector hatSelector;
    rk::ui::DragMidiZone dragZone;
    PlayTile playTile;
    BpmBox bpmBox { proc };
    HistoryStrip history { proc };
    juce::OwnedArray<juce::TextButton> moodButtons;
    rk::ui::SegmentedChoice playMode;
    juce::TooltipWindow tooltips { this, 600 };

    int shownHat = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RollsKillaMiniEditor)
};
