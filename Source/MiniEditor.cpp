#include "MiniEditor.h"

#include "engine/CategoryProfile.h"

using namespace rk;
using namespace rk::ui;
using skin::design;

namespace
{
    // Layout: the skin picture is 2000 x 733 design pixels = 600 x 220 points (see ui/SkinWidgets.h)
    const juce::Rectangle<float> kSlot       = design (62, 222, 1942, 342);
    const juce::Rectangle<float> kPresetLine = design (74, 348, 820, 372);
    const juce::Rectangle<float> kBpm        = design (1090, 90, 1296, 176);
    const juce::Rectangle<float> kUndo       = design (1340, 95, 1426, 181);
    const juce::Rectangle<float> kRedo       = design (1452, 95, 1538, 181);
    const juce::Rectangle<float> kBars       = design (1592, 118, 1912, 190);
    const juce::Rectangle<float> kHistory    = design (846, 360, 1178, 396);
    const juce::Rectangle<float> kMood       = design (76, 404, 350, 448);
    const juce::Rectangle<float> kKill       = design (95, 455, 312, 602);
    const juce::Rectangle<float> kHat        = design (380, 450, 695, 608);
    const juce::Rectangle<float> kBlunt      = design (740, 405, 1585, 590);
    const juce::Rectangle<float> kDrag       = design (1626, 468, 1760, 584);
    const juce::Rectangle<float> kPlay       = design (1818, 442, 1936, 560);
    const juce::Rectangle<float> kMode       = design (1816, 594, 1922, 626);
    const juce::Rectangle<float> kSmoke      = design (1380, 230, 1720, 560);

    constexpr const char* kMiniScaleProp = "miniUiScale";
}

//==============================================================================
void RollsKillaMiniEditor::Content::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff070707));
    g.drawImage (skin::images().plate, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);

    // what plays now, stamped small under the roll window
    const auto& line = editor.shownPresetLine;
    g.setFont (metal::plateFont (7.0f, 0.12f));
    g.setColour (juce::Colours::white.withAlpha (0.28f));
    g.drawText (line, kPresetLine.translated (0.0f, 0.6f), juce::Justification::centredLeft, true);
    g.setColour (juce::Colour (0xff141414).withAlpha (0.9f));
    g.drawText (line, kPresetLine, juce::Justification::centredLeft, true);
}

void RollsKillaMiniEditor::Content::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu() && e.position.y < kSlot.getY())
        editor.showScaleMenu();
}

//==============================================================================
RollsKillaMiniEditor::RollsKillaMiniEditor (RollsKillaProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      visualizer (p, true),
      bars (p.getState()),
      hatPicker (p.getState(), [&p] { return p.getSampler().getSampleNames(); }),
      blunt (p.getState()),
      playMode (p.getState())
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (content);

    visualizer.setEmberStyle (true);
    smoke.drawGlow = false;   // the ember glow belongs to the blunt
    for (auto* c : std::initializer_list<juce::Component*> { &visualizer, &bpm, &undoButton, &redoButton, &bars, &history, &mood,
                                                              &killButton, &hatPicker, &blunt, &dragButton, &playButton, &playMode, &smoke })
        content.addAndMakeVisible (c);

    bpm.setTooltip ("Tempo. AUTO = follows your DAW, KILL picks rolls that fit it. Drag up/down to set your own, double-click = AUTO.");
    history.setTooltip ("The last KILLs - click a light to go back to that roll");
    bars.setTooltip ("Pattern length in bars");

    mood.setMood (proc.getMood());
    mood.setTooltip ("What KILL picks: CHILL = simple rolls, TRAP = classic, CRAZY = the wildest");
    mood.onChange = [this] (int m) { proc.setMood (m); };

    undoButton.setTooltip ("Undo - e.g. back to the previous KILL");
    redoButton.setTooltip ("Redo");
    undoButton.onClick = [this] { proc.undo(); updateStatus(); };
    redoButton.onClick = [this] { proc.redo(); updateStatus(); };

    killButton.setTooltip ("KILL - a new roll: random preset + new variation (right-click: same preset / back to original)");
    killButton.onClick = [this] { proc.killEverything(); updateStatus(); };
    killButton.onSamePresetVariation = [this] { proc.kill(); updateStatus(); };
    killButton.onBackToOriginal = [this] { proc.resetVariation(); updateStatus(); };

    hatPicker.wave.onFileDropped = [this] (const juce::File& f) { proc.loadCustomSample (f); shownHat = -1; };
    hatPicker.onChange = [this] { shownHat = -1; };
    shownHat = hatPicker.getIndex();
    hatPicker.wave.setSample (proc.getSampler().getSample (shownHat));

    blunt.setTooltip ("PUFF - smoke FX on the hats: crackle, a breath in the beat and a hazy filter. Double-click = off.");

    dragButton.createFile = [this] { return proc.createDragMidiFile(); };
    dragButton.setTooltip ("Drag the roll into your DAW (FL Studio: drop on the Playlist or a Piano roll)");

    playButton.onClick = [this] { proc.setPreviewEnabled (! proc.isPreviewEnabled()); updateStatus(); };
    playButton.setTooltip ("Play the roll while the DAW is stopped");
    playMode.setTooltip ("MIDI: plays only while a note is held on this channel. HOST: plays whenever the DAW plays.");

    uiScale = (float) (double) proc.getState().state.getProperty (kMiniScaleProp, 1.25);  // 125 % default: small but readable

    // never take the keyboard (also not by clicking a button): space, Ctrl+Z etc. stay with the DAW
    std::function<void (juce::Component&)> noKeyboard = [&noKeyboard] (juce::Component& c)
    {
        c.setWantsKeyboardFocus (false);
        c.setMouseClickGrabsKeyboardFocus (false);
        for (auto* child : c.getChildren())
            noKeyboard (*child);
    };
    noKeyboard (*this);

    layout();
    setUiScale (uiScale);
    updateStatus();

    // the blunt is already smoking when the window opens
    for (int i = 0; i < 75; ++i)
        smoke.tick (1.0f / 30.0f, blunt.getValue(), 0.5f,
                    blunt.getEmberTip() + blunt.getPosition().toFloat() - smoke.getPosition().toFloat());
    startTimerHz (30);
}

RollsKillaMiniEditor::~RollsKillaMiniEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void RollsKillaMiniEditor::layout()
{
    visualizer.setBounds (kSlot.toNearestInt());
    bpm.setBounds (kBpm.toNearestInt());
    undoButton.setBounds (kUndo.toNearestInt());
    redoButton.setBounds (kRedo.toNearestInt());
    bars.setBounds (kBars.toNearestInt());
    history.setBounds (kHistory.toNearestInt());
    mood.setBounds (kMood.toNearestInt());
    killButton.setBounds (kKill.toNearestInt());
    hatPicker.setBounds (kHat.toNearestInt());
    blunt.setBounds (kBlunt.toNearestInt());
    dragButton.setBounds (kDrag.toNearestInt());
    playButton.setBounds (kPlay.toNearestInt());
    playMode.setBounds (kMode.toNearestInt());
    smoke.setBounds (kSmoke.toNearestInt());
}

void RollsKillaMiniEditor::setUiScale (float scale)
{
    uiScale = juce::jlimit (1.0f, 2.0f, scale);
    proc.getState().state.setProperty (kMiniScaleProp, uiScale, nullptr);
    content.setTransform (juce::AffineTransform::scale (uiScale));
    setSize (juce::roundToInt (kBaseWidth * uiScale), juce::roundToInt (kBaseHeight * uiScale));
}

void RollsKillaMiniEditor::showScaleMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader ("UI SCALE");
    const int pcts[] { 100, 125, 150, 200 };
    for (auto pct : pcts)
        menu.addItem (pct, juce::String (pct) + " %", true, juce::roundToInt (uiScale * 100.0f) == pct);
    menu.showMenuAsync (juce::PopupMenu::Options(), [safe = juce::Component::SafePointer<RollsKillaMiniEditor> (this)] (int result)
                        {
                            if (safe != nullptr && result > 0)
                                safe->setUiScale ((float) result / 100.0f);
                        });
}

void RollsKillaMiniEditor::resized()
{
    content.setBounds (0, 0, kBaseWidth, kBaseHeight);
}

void RollsKillaMiniEditor::updateStatus()
{
    const auto hostPlaying = proc.isHostPlaying();
    const auto previewing = proc.isPreviewEnabled() && ! hostPlaying;
    if (playButton.previewing != previewing || playButton.hostPlaying != hostPlaying)
    {
        playButton.previewing = previewing;
        playButton.hostPlaying = hostPlaying;
        playButton.repaint();
    }
    playButton.setEnabled (! hostPlaying);
    undoButton.setEnabled (proc.canUndo());
    redoButton.setEnabled (proc.canRedo());
    mood.setMood (proc.getMood());
    bpm.repaint();
    history.repaint();

    const auto seed = proc.getSeed();
    killButton.seedText = seed != 0 ? "#" + juce::String ((int) seed) : juce::String();

    const auto& preset = proc.getModel().getPreset();
    const auto line = juce::String (getCategoryProfile (preset.category).name) + "  /  " + preset.name.toUpperCase();
    if (line != shownPresetLine)
    {
        shownPresetLine = line;
        content.repaint (kPresetLine.expanded (2.0f).toNearestInt());
    }
}

void RollsKillaMiniEditor::timerCallback()
{
    const auto hat = hatPicker.getIndex();
    if (hat != shownHat)
    {
        shownHat = hat;
        hatPicker.wave.setSample (proc.getSampler().getSample (hat));
    }

    const auto glow = proc.getSmokeGlow();
    blunt.setGlow (glow);
    smoke.tick (1.0f / 30.0f, blunt.getValue(), glow,
                blunt.getEmberTip() + blunt.getPosition().toFloat() - smoke.getPosition().toFloat());

    if (++frame % 2 == 0)
        updateStatus();
}
