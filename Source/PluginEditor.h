#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace HG
{
    // palette (matches the HAT GOBLIN design boards)
    const juce::Colour ink { 0xff0a0a0a }, panel { 0xff0e0e0e }, black { 0xff000000 }, line { 0xff2e2e2e },
                       lineSoft { 0xff1e1e1e }, bone { 0xffe8e4d8 }, boneKey { 0xffd9d4c5 }, muted { 0xff8e8a80 },
                       gray { 0xff5e5a52 }, red { 0xffe3170a }, redDark { 0xff5a0a05 }, redDim { 0xff8e1009 };

    struct Fonts
    {
        juce::Typeface::Ptr anton, mono, black, led;
        static const Fonts& get();
        static juce::Font label (float h);   // IBM Plex Mono Bold
        static juce::Font title (float h);   // Anton
        static juce::Font gothic (float h);  // UnifrakturCook
        static juce::Font lcd (float h);     // VT323
    };

    class Look : public juce::LookAndFeel_V4
    {
    public:
        Look();
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
        void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
        juce::Font getTextButtonFont (juce::TextButton&, int h) override;
        void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getPopupMenuFont() override;
        void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    };
}

// A labelled knob bound to one parameter.
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& label, std::function<juce::String (float)> fmt);
    void resized() override;
    void paint (juce::Graphics&) override;
    juce::Slider slider;
private:
    juce::String label;
    std::function<juce::String (float)> format;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class HatGoblinEditor : public juce::AudioProcessorEditor,
                        public juce::FileDragAndDropTarget,
                        private juce::Timer
{
public:
    explicit HatGoblinEditor (HatGoblinProcessor&);
    ~HatGoblinEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragHover = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragHover = false; repaint(); }

    static constexpr int baseW = 1100, baseH = 680;

private:
    void timerCallback() override;
    void showPage (int index);
    void setParam (const juce::String& id, float plainValue);
    float param (const juce::String& id) const;
    void chooseUserFile();

    HatGoblinProcessor& proc;
    HG::Look look;
    juce::Image logo, grain;
    std::unique_ptr<juce::Component> content;   // laid out at 1100 x 680, scaled to the window
    bool dragHover = false;
    std::unique_ptr<juce::FileChooser> chooser;

    friend class GoblinContent;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HatGoblinEditor)
};
