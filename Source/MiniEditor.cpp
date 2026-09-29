#include "MiniEditor.h"

#include "engine/CategoryProfile.h"

using namespace rk;
using namespace rk::ui;
using namespace rk::ui::metal;

namespace
{
    // Layout at 100 % (matches docs/design/mini_metal.webp)
    const juce::Rectangle<float> kPlate      { 1.0f, 1.0f, 598.0f, 218.0f };
    const juce::Rectangle<float> kLogo       { 26.0f, 11.0f, 212.0f, 37.0f };
    const juce::Rectangle<int>   kBpm        { 318, 17, 78, 40 };
    const juce::Rectangle<int>   kUndo       { 404, 23, 29, 29 };
    const juce::Rectangle<int>   kRedo       { 437, 23, 29, 29 };
    const juce::Rectangle<int>   kBars       { 480, 14, 106, 44 };
    const juce::Rectangle<float> kSlot       { 16.0f, 64.0f, 568.0f, 42.0f };
    const juce::Rectangle<float> kPresetLine { 22.0f, 108.0f, 230.0f, 11.0f };
    const juce::Rectangle<int>   kHistory    { 256, 107, 104, 12 };
    const juce::Rectangle<int>   kMood       { 14, 118, 100, 22 };
    const juce::Rectangle<int>   kKill       { 22, 142, 84, 44 };
    const juce::Rectangle<int>   kHat        { 124, 132, 90, 54 };
    const juce::Rectangle<int>   kBlunt      { 224, 118, 254, 74 };
    const juce::Rectangle<int>   kDrag       { 492, 138, 40, 40 };
    const juce::Rectangle<int>   kPlay       { 545, 115, 48, 52 };
    const juce::Rectangle<int>   kMode       { 543, 170, 52, 24 };
    const juce::Rectangle<int>   kSmoke      { 404, 96, 120, 96 };
    const float kGrooves[] { 118.0f, 219.0f, 484.0f, 540.0f };

    constexpr const char* kMiniScaleProp = "miniUiScale";
}

//==============================================================================
void RollsKillaMiniEditor::Content::paint (juce::Graphics& g)
{
    auto& ed = editor;
    g.fillAll (juce::Colour (0xff070707));

    // the plate
    const auto scale = (float) g.getInternalContext().getPhysicalPixelScaleFactor();
    if (ed.steel.isNull() || std::abs (ed.steelScale - scale) > 0.01f)
    {
        ed.steelScale = scale;
        ed.steel = renderSteel (kBaseWidth, kBaseHeight, juce::jmin (scale, 3.0f));
    }
    {
        juce::Graphics::ScopedSaveState save (g);
        juce::Path plate;
        plate.addRoundedRectangle (kPlate, 7.0f);
        g.reduceClipRegion (plate);
        g.drawImage (ed.steel, juce::Rectangle<float> (0.0f, 0.0f, (float) kBaseWidth, (float) kBaseHeight));
    }
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawRoundedRectangle (kPlate.reduced (1.2f), 6.0f, 0.8f);
    g.setColour (juce::Colours::black.withAlpha (0.9f));
    g.drawRoundedRectangle (kPlate, 7.0f, 1.2f);

    drawScrew (g, { 12.0f, 12.0f }, 5.5f, 0.4f);
    drawScrew (g, { 588.0f, 12.0f }, 5.5f, 1.1f);
    drawScrew (g, { 12.0f, 208.0f }, 5.5f, 0.9f);
    drawScrew (g, { 588.0f, 208.0f }, 5.5f, 0.2f);

    // stamped logo
    juce::GlyphArrangement ga;
    ga.addLineOfText (juce::Font (juce::FontOptions (40.0f, juce::Font::bold)).withHorizontalScale (0.78f).withExtraKerningFactor (-0.02f),
                      "ROLLS KILLA", 0.0f, 0.0f);
    juce::Path logo;
    ga.createPath (logo);
    logo.applyTransform (juce::AffineTransform::shear (-0.16f, 0.0f));
    logo.applyTransform (logo.getTransformToScaleToFit (kLogo, true, juce::Justification::centredLeft));
    const auto logoBounds = logo.getBounds();
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.fillPath (logo, juce::AffineTransform::translation (0.0f, 1.1f));
    g.strokePath (logo, juce::PathStrokeType (1.4f), juce::AffineTransform::translation (0.0f, 1.1f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2e2e2c), 0.0f, logoBounds.getY(), juce::Colour (0xff0e0e0e), 0.0f, logoBounds.getBottom(), false));
    g.fillPath (logo);
    g.strokePath (logo, juce::PathStrokeType (1.4f));
    {
        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (logo);
        g.setOpacity (0.22f);
        g.drawImage (ed.steel, juce::Rectangle<float> (0.0f, 0.0f, (float) kBaseWidth, (float) kBaseHeight));
    }

    // MINI badge
    const auto badge = juce::Rectangle<float> (logoBounds.getRight() + 5.0f, logoBounds.getBottom() - 15.0f, 36.0f, 13.0f);
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.fillRoundedRectangle (badge.translated (0.0f, 0.8f), 2.0f);
    g.setColour (juce::Colour (0xff161615));
    g.fillRoundedRectangle (badge, 2.0f);
    {
        juce::GlyphArrangement mg;
        mg.addFittedText (plateFont (10.0f, 0.08f), "MINI", badge.getX(), badge.getY(), badge.getWidth(), badge.getHeight(),
                          juce::Justification::centred, 1);
        juce::Path mini;
        mg.createPath (mini);
        mini.applyTransform (juce::AffineTransform::shear (-0.18f, 0.0f).translated (badge.getCentreY() * 0.18f, 0.0f));
        g.setColour (juce::Colour (0xffb8b8b2));
        g.fillPath (mini);
    }

    drawEngravedText (g, "TRAP HI-HAT ROLLS GENERATOR", plateFont (7.6f, 0.62f), { 28.0f, 49.0f, 280.0f, 11.0f },
                      juce::Justification::centredLeft);

    drawGroove (g, 308.0f, 15.0f, 58.0f);
    drawGroove (g, 474.0f, 15.0f, 58.0f);

    // roll window
    drawRecess (g, kSlot, 4.0f);
    drawEngravedText (g, ed.shownPresetLine, plateFont (7.0f, 0.12f), kPresetLine, juce::Justification::centredLeft, 0.85f);

    // bottom row
    for (auto x : kGrooves)
        drawGroove (g, x, 121.0f, 206.0f);
    drawEngravedText (g, "MILLIONS OF COMBINATIONS", plateFont (6.4f, 0.04f), { 12.0f, 188.0f, 104.0f, 10.0f }, juce::Justification::centred);
    drawEngravedText (g, "HI-HAT SAMPLE", plateFont (8.0f, 0.06f), { 122.0f, 118.0f, 94.0f, 12.0f }, juce::Justification::centred);
    drawEngravedText (g, "DRAG TO DAW", plateFont (7.4f, 0.02f), { 484.0f, 120.0f, 56.0f, 12.0f }, juce::Justification::centred);

    const auto smokeY = 199.0f;
    drawEngravedText (g, "SMOKE  FX", plateFont (8.0f, 0.3f), { 310.0f, smokeY - 5.0f, 80.0f, 10.0f }, juce::Justification::centred);
    for (auto seg : { juce::Range<float> (262.0f, 308.0f), juce::Range<float> (392.0f, 438.0f) })
    {
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRect (juce::Rectangle<float> (seg.getStart(), smokeY - 0.6f, seg.getLength(), 1.0f));
        g.setColour (juce::Colours::white.withAlpha (0.2f));
        g.fillRect (juce::Rectangle<float> (seg.getStart(), smokeY + 0.5f, seg.getLength(), 0.7f));
    }
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
    for (auto* c : std::initializer_list<juce::Component*> { &visualizer, &bpm, &undoButton, &redoButton, &bars, &history, &mood,
                                                              &killButton, &hatPicker, &blunt, &dragButton, &playButton, &playMode, &smoke })
        content.addAndMakeVisible (c);

    bpm.setTooltip ("Tempo. AUTO = follows your DAW, KILL picks rolls that fit it. Drag up/down to set your own, double-click = AUTO.");
    history.setTooltip ("The last KILLs - click a light to go back to that roll");
    bars.setTooltip ("Pattern length in bars");

    mood.setMood (proc.getMood());
    mood.setTooltip ("What KILL picks: CHILL = simple rolls, TRAP = classic, CRAZY = the wildest");
    mood.onChange = [this] (int m) { proc.setMood (m); };

    undoButton.setTooltip ("Undo - e.g. back to the previous KILL (Ctrl+Z)");
    redoButton.setTooltip ("Redo (Ctrl+Shift+Z)");
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
    setWantsKeyboardFocus (true);
    layout();
    setUiScale (uiScale);
    updateStatus();

    // the blunt is already burning when the window opens
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
    visualizer.setBounds (kSlot.reduced (3.0f).toNearestInt());
    bpm.setBounds (kBpm);
    undoButton.setBounds (kUndo);
    redoButton.setBounds (kRedo);
    bars.setBounds (kBars);
    history.setBounds (kHistory);
    mood.setBounds (kMood);
    killButton.setBounds (kKill);
    hatPicker.setBounds (kHat);
    blunt.setBounds (kBlunt);
    dragButton.setBounds (kDrag);
    playButton.setBounds (kPlay);
    playMode.setBounds (kMode);
    smoke.setBounds (kSmoke);
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

bool RollsKillaMiniEditor::keyPressed (const juce::KeyPress& key)
{
    const auto cmd = key.getModifiers().isCommandDown();
    if (cmd && key.getKeyCode() == 'Z')
    {
        key.getModifiers().isShiftDown() ? proc.redo() : proc.undo();
        updateStatus();
        return true;
    }
    return false;
}
