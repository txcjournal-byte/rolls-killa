#pragma once

#include "PluginProcessor.h"
#include "ui/KillaLookAndFeel.h"
#include "ui/PresetBar.h"
#include "ui/PresetBrowser.h"
#include "ui/RollVisualizer.h"
#include "ui/Widgets.h"

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

    static constexpr int kBaseWidth = 900;
    static constexpr int kBaseHeight = 560;

private:
    /** All controls live in here at 900x560 and are scaled as a whole. */
    class Content : public juce::Component
    {
    public:
        explicit Content (RollsKillaEditor& e) : editor (e) {}
        void paint (juce::Graphics&) override;
        RollsKillaEditor& editor;
    };

    void timerCallback() override;
    void layout();
    void setUiScale (float scale);
    void chooseCustomSample();
    void exportMidi();
    void updateStatus();

    RollsKillaProcessor& processor;
    rk::ui::KillaLookAndFeel lookAndFeel;
    Content content { *this };
    juce::Image logo;
    float uiScale = 1.0f;

    juce::OwnedArray<juce::TextButton> scaleButtons;
    rk::ui::RollVisualizer visualizer;
    rk::ui::PresetBar presetBar;
    rk::ui::IconButton undoButton { "Undo", rk::ui::Icon::undo }, redoButton { "Redo", rk::ui::Icon::redo };
    rk::ui::IconButton previewButton { "Preview", rk::ui::Icon::play };

    rk::ui::SegmentedChoice rollSpeed, velMode;
    rk::ui::Knob density, groove, pitchRamp, swing, variation;

    rk::ui::HatSelector hatSelector;
    rk::ui::IconButton loadWavButton { "Load WAV", rk::ui::Icon::folder };
    rk::ui::WaveformView waveform;
    rk::ui::Knob tune, decay, volume;
    rk::ui::ToggleSwitch chokeSwitch;
    juce::AudioProcessorValueTreeState::ButtonAttachment chokeAttachment;

    rk::ui::KillButton killButton;
    rk::ui::DragMidiZone dragZone;
    rk::ui::IconButton exportButton { "Export MIDI", rk::ui::Icon::exportFile };

    rk::ui::PresetBrowser browser;
    juce::TooltipWindow tooltips { this, 600 };
    std::unique_ptr<juce::FileChooser> fileChooser;

    int shownPatternVersion = -1;
    int shownHat = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RollsKillaEditor)
};
