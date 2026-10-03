#include "PluginEditor.h"

using namespace rk;
using namespace rk::ui;
using namespace rk::ui::station;

namespace
{
    // Layout at 100 %
    const juce::Rectangle<float> kLogo      { 22.0f, 8.0f, 330.0f, 44.0f };
    const juce::Rectangle<int>   kBpm       { 372, 14, 88, 46 };
    const juce::Rectangle<int>   kMood      { 474, 30, 186, 26 };
    const juce::Rectangle<int>   kUndo      { 672, 22, 34, 34 };
    const juce::Rectangle<int>   kRedo      { 710, 22, 34, 34 };
    const juce::Rectangle<int>   kKillBeat  { 754, 8, 134, 60 };
    const juce::Rectangle<int>   kPlay      { 898, 12, 70, 28 };
    const juce::Rectangle<int>   kPlayMode  { 898, 44, 70, 20 };
    const juce::Rectangle<int>   kSmoke     { 974, 6, 58, 62 };
    const juce::Rectangle<int>   kPads      { 16, 80, 384, 216 };
    const juce::Rectangle<int>   kBay       { 412, 80, 612, 216 };
    const juce::Rectangle<int>   kSound     { 16, 306, 520, 160 };
    const juce::Rectangle<int>   kPattern   { 544, 306, 480, 160 };
    const juce::Rectangle<int>   kKit       { 16, 476, 1008, 146 };

    constexpr const char* kUiScaleProp = "stationScale";
}

//==============================================================================
void RollsKillaEditor::Content::paint (juce::Graphics& g)
{
    auto& ed = editor;
    const auto scale = juce::jmin (3.0f, (float) g.getInternalContext().getPhysicalPixelScaleFactor());
    if (ed.plate.isNull() || std::abs (ed.plateScale - scale) > 0.01f)
    {
        ed.plateScale = scale;
        ed.plate = renderDarkSteel (kBaseWidth, kBaseHeight, scale);
        ed.logo = {};
    }
    g.drawImage (ed.plate, getLocalBounds().toFloat());

    // screws
    for (auto p : { juce::Point<float> (9.0f, 9.0f), juce::Point<float> ((float) kBaseWidth - 9.0f, 9.0f),
                    juce::Point<float> (9.0f, (float) kBaseHeight - 9.0f), juce::Point<float> ((float) kBaseWidth - 9.0f, (float) kBaseHeight - 9.0f) })
        metal::drawScrew (g, p, 4.5f, p.x * 0.01f);

    // logo: heavy chrome letters with a blood-red glow, scratched
    if (ed.logo.isNull())
    {
        ed.logo = juce::Image (juce::Image::ARGB, juce::roundToInt (kLogo.getRight() * scale) + 20, juce::roundToInt ((kLogo.getBottom() + 8.0f) * scale), true);
        juce::Graphics lg (ed.logo);
        lg.addTransform (juce::AffineTransform::scale (scale));
        juce::GlyphArrangement ga;
        ga.addLineOfText (juce::Font (juce::FontOptions (48.0f, juce::Font::bold)).withHorizontalScale (0.78f).withExtraKerningFactor (-0.01f), "ROLLS KILLA", 0.0f, 0.0f);
        juce::Path path;
        ga.createPath (path);
        path.applyTransform (juce::AffineTransform::shear (-0.16f, 0.0f));
        path.applyTransform (path.getTransformToScaleToFit (kLogo, true, juce::Justification::centredLeft));
        const auto b = path.getBounds();
        for (int i = 4; i > 0; --i)
        {
            lg.setColour (juce::Colour (0xffe0201c).withAlpha (0.09f));
            lg.strokePath (path, juce::PathStrokeType ((float) i * 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        lg.setColour (juce::Colours::black);
        lg.fillPath (path, juce::AffineTransform::translation (0.0f, 2.0f));
        juce::ColourGradient chrome (juce::Colour (0xfff4efe9), 0.0f, b.getY(), juce::Colour (0xff4a4440), 0.0f, b.getBottom(), false);
        chrome.addColour (0.48, juce::Colour (0xffb9b1aa));
        chrome.addColour (0.52, juce::Colour (0xff6c6560));
        lg.setGradientFill (chrome);
        lg.fillPath (path);
        {
            juce::Graphics::ScopedSaveState save (lg);
            lg.reduceClipRegion (path);
            juce::Random rng (13);
            for (int i = 0; i < 260; ++i)
            {
                const auto x = b.getX() + rng.nextFloat() * b.getWidth(), y = b.getY() + rng.nextFloat() * b.getHeight();
                lg.setColour (juce::Colour (0xff1a1514).withAlpha (0.15f + rng.nextFloat() * 0.35f));
                lg.drawLine (x, y, x + rng.nextFloat() * 9.0f - 4.0f, y + rng.nextFloat() * 3.0f, 0.6f);
            }
        }
        lg.setColour (juce::Colours::black.withAlpha (0.8f));
        lg.strokePath (path, juce::PathStrokeType (1.0f));
    }
    g.drawImage (ed.logo, juce::Rectangle<float> (0.0f, 0.0f, (float) ed.logo.getWidth() / scale, (float) ed.logo.getHeight() / scale));

    drawStamped (g, "DRUM  &  ROLL  FACTORY", metal::plateFont (10.5f, 0.45f), { 24.0f, 54.0f, 240.0f, 14.0f }, juce::Justification::centredLeft,
                 juce::Colour (0xffbdb6af));
    drawStamped (g, "by TrapVST", metal::plateFont (9.5f, 0.12f), { 250.0f, 54.0f, 102.0f, 14.0f }, juce::Justification::centredRight,
                 juce::Colour (0xff8c8680));
    drawStamped (g, "KILL MOOD", metal::plateFont (8.5f, 0.25f), { (float) kMood.getX(), 14.0f, (float) kMood.getWidth(), 12.0f },
                 juce::Justification::centred, juce::Colour (0xff8c8680));
}

void RollsKillaEditor::Content::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu() && e.position.y < (float) kPads.getY())
        editor.showScaleMenu();
}

//==============================================================================
RollsKillaEditor::RollsKillaEditor (RollsKillaProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      playMode (p.getState(), params::playMode, { "MIDI", "HOST" }),
      smokeAttachment (p.getState(), params::puff, smoke),
      rollView (p),
      browser (p)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (content);

    // header
    for (auto* c : std::initializer_list<juce::Component*> { &bpm, &mood, &undoButton, &redoButton, &killBeat, &playButton, &playMode, &smoke })
        content.addAndMakeVisible (c);
    bpm.setTooltip ("Tempo: AUTO follows your DAW. Drag up/down for your own, double-click = AUTO");
    mood.setTooltip ("How wild KILL goes: CHILL / TRAP / CRAZY (sounds, rolls and patterns)");
    mood.setSelected (proc.getMood());
    mood.onChange = [this] (int m) { proc.setMood (m); };
    undoButton.setTooltip ("Undo (Ctrl+Z)");
    redoButton.setTooltip ("Redo (Ctrl+Y)");
    undoButton.onClick = [this] { proc.undo(); refreshAll(); };
    redoButton.onClick = [this] { proc.redo(); refreshAll(); };
    killBeat.setTooltip ("KILL BEAT - new patterns for every playing drawer + a new hi-hat roll (right-click: new kit sounds)");
    killBeat.onClick = [this] { proc.killBeat(); refreshAll(); };
    killBeat.onKillKit = [this] { proc.killKit(); refreshAll(); };
    killBeat.onKillAll = [this] { proc.killKit(); proc.killBeat(); refreshAll(); };
    playButton.setTooltip ("Play the beat while the DAW is stopped (or SPACE in this window)");
    playButton.onClick = [this] { proc.setPreviewEnabled (! proc.isPreviewEnabled()); updateStatus(); };
    playMode.setTooltip ("MIDI: plays only while a note is held on this channel. HOST: plays whenever the DAW plays");
    smoke.setTooltip ("SMOKE (PUFF): crackle, a breath in the beat and a hazy filter on the whole beat");

    // pads
    for (int t = 0; t < kNumDrumTypes; ++t)
    {
        auto* pad = pads.add (new Pad (proc, t));
        pad->onSelect = [this] (int type) { selectDrawer (type); };
        pad->onChanged = [this] { refreshAll(); };
        pad->onLoadWav = [this] (int type) { loadWavInto (type); };
        content.addAndMakeVisible (pad);
    }

    content.addAndMakeVisible (beatView);
    content.addChildComponent (rollView);
    beatView.onSelect = [this] (int type) { selectDrawer (type); };
    beatView.onChanged = [this] { refreshAll(); };
    for (auto* b : { &beatTab, &rollTab })
        content.addAndMakeVisible (b);
    beatTab.onClick = [this] { showRoll (false); };
    rollTab.onClick = [this] { showRoll (true); };
    beatTab.setTooltip ("The whole beat");
    rollTab.setTooltip ("Edit the hi-hat roll: click a note = mute, drag = velocity, double-click a roll = speed, right-click = delete");

    content.addAndMakeVisible (soundPanel);
    content.addAndMakeVisible (patternPanel);
    content.addChildComponent (hatPanel);
    content.addAndMakeVisible (kitPanel);
    soundPanel.onChanged = [this] { refreshAll(); };
    soundPanel.onLoadWav = [this] { loadWavInto (selected); };
    patternPanel.onChanged = [this] { beatView.repaint(); refreshAll(); };
    hatPanel.onChanged = [this] { refreshAll(); };
    hatPanel.onOpenBrowser = [this] (int category) { browser.open (category); };
    kitPanel.onChanged = [this] { refreshAll(); };
    kitPanel.onExportKit = [this] { exportKit(); };
    kitPanel.onExportOneShots = [this] (int count) { exportOneShots (count); };

    content.addChildComponent (browser);
    browser.onPresetLoaded = [this] { refreshAll(); };

    // only the window and text fields take the keyboard; SPACE never re-presses the last clicked button
    std::function<void (juce::Component&)> noButtonFocus = [&noButtonFocus] (juce::Component& c)
    {
        if (dynamic_cast<juce::TextEditor*> (&c) == nullptr)
            c.setWantsKeyboardFocus (false);
        for (auto* child : c.getChildren())
            noButtonFocus (*child);
    };
    noButtonFocus (content);
    setWantsKeyboardFocus (true);

    uiScale = (float) (double) proc.getState().state.getProperty (kUiScaleProp, 1.0);
    layout();
    setUiScale (uiScale);
    selectDrawer (0);
    refreshAll();
    for (auto* pad : pads)
        pad->tick();
    startTimerHz (30);
}

RollsKillaEditor::~RollsKillaEditor()
{
    stopTimer();
    juce::PopupMenu::dismissAllActiveMenus();
    setLookAndFeel (nullptr);
}

void RollsKillaEditor::layout()
{
    bpm.setBounds (kBpm);
    mood.setBounds (kMood);
    undoButton.setBounds (kUndo);
    redoButton.setBounds (kRedo);
    killBeat.setBounds (kKillBeat);
    playButton.setBounds (kPlay);
    playMode.setBounds (kPlayMode);
    smoke.setBounds (kSmoke);

    const auto padW = kPads.getWidth() / 4, padH = kPads.getHeight() / 2;
    for (int i = 0; i < pads.size(); ++i)
        pads[i]->setBounds (kPads.getX() + (i % 4) * padW, kPads.getY() + (i / 4) * padH, padW, padH);

    beatView.setBounds (kBay);
    rollView.setBounds (kBay.withTrimmedTop (26));
    rollTab.setBounds (kBay.getRight() - 110, kBay.getY() + 4, 104, 20);
    beatTab.setBounds (kBay.getRight() - 172, kBay.getY() + 4, 60, 20);

    soundPanel.setBounds (kSound);
    patternPanel.setBounds (kPattern);
    hatPanel.setBounds (kPattern);
    kitPanel.setBounds (kKit);
    browser.setBounds (0, 0, kBaseWidth, kBaseHeight);
}

void RollsKillaEditor::setUiScale (float scale)
{
    uiScale = juce::jlimit (0.75f, 1.5f, scale);
    proc.getState().state.setProperty (kUiScaleProp, uiScale, nullptr);
    content.setTransform (juce::AffineTransform::scale (uiScale));
    setSize (juce::roundToInt (kBaseWidth * uiScale), juce::roundToInt (kBaseHeight * uiScale));
}

void RollsKillaEditor::showScaleMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader ("UI SCALE");
    for (auto pct : { 75, 90, 100, 125, 150 })
        menu.addItem (pct, juce::String (pct) + " %", true, juce::roundToInt (uiScale * 100.0f) == pct);
    menu.showMenuAsync (juce::PopupMenu::Options(), [safe = juce::Component::SafePointer<RollsKillaEditor> (this)] (int result)
                        {
                            if (safe != nullptr && result > 0)
                                safe->setUiScale ((float) result / 100.0f);
                        });
}

void RollsKillaEditor::resized()
{
    content.setBounds (0, 0, kBaseWidth, kBaseHeight);
}

void RollsKillaEditor::selectDrawer (int type)
{
    selected = juce::jlimit (0, kNumDrumTypes - 1, type);
    for (int i = 0; i < pads.size(); ++i)
    {
        pads[i]->selected = i == selected;
        pads[i]->repaint();
    }
    beatView.selected = selected;
    beatView.repaint();
    soundPanel.setDrawer (selected);
    const auto hat = drumTypeFromIndex (selected) == DrumType::hat;
    patternPanel.setVisible (! hat);
    hatPanel.setVisible (hat);
    if (hat)
        hatPanel.refresh();
    else
        patternPanel.setDrawer (selected);
    kitPanel.selectedDrawer = selected;
    kitPanel.refresh();
    showRoll (hat);
}

void RollsKillaEditor::showRoll (bool roll)
{
    rollShown = roll;
    rollView.setVisible (roll);
    beatView.setVisible (true);   // the bay frame + title stay
    beatTab.lit = ! roll;
    rollTab.lit = roll;
    beatTab.repaint();
    rollTab.repaint();
}

void RollsKillaEditor::refreshAll()
{
    shownKitVersion = proc.getKitVersion();
    soundPanel.refresh();
    if (patternPanel.isVisible())
        patternPanel.refresh();
    if (hatPanel.isVisible())
        hatPanel.refresh();
    kitPanel.refresh();
    mood.setSelected (proc.getMood());
    for (auto* pad : pads)
        pad->repaint();
    beatView.repaint();
    updateStatus();
}

void RollsKillaEditor::updateStatus()
{
    const auto hostPlaying = proc.isHostPlaying();
    const auto previewing = proc.isPreviewEnabled() && ! hostPlaying;
    playButton.setButtonText (hostPlaying ? "SYNC" : previewing ? "STOP" : "PLAY");
    playButton.lit = previewing || hostPlaying;
    playButton.setEnabled (! hostPlaying);
    playButton.repaint();
    undoButton.setEnabled (proc.canUndo());
    redoButton.setEnabled (proc.canRedo());
    bpm.repaint();
}

void RollsKillaEditor::timerCallback()
{
    for (auto* pad : pads)
        pad->tick();
    beatView.tick();

    if (proc.getKitVersion() != shownKitVersion || proc.getPatternVersion() != shownPatternVersion)
    {
        shownPatternVersion = proc.getPatternVersion();
        refreshAll();
    }
    if (++frame % 3 == 0)
        updateStatus();
}

bool RollsKillaEditor::keyPressed (const juce::KeyPress& key)
{
    if (key.getKeyCode() == juce::KeyPress::spaceKey && ! key.getModifiers().isAnyModifierKeyDown())
    {
        proc.setPreviewEnabled (! proc.isPreviewEnabled());
        updateStatus();
        return true;
    }
    if (key.getModifiers().isCommandDown() && (key.getKeyCode() == 'Z' || key.getKeyCode() == 'Y'))
    {
        const auto redo = key.getKeyCode() == 'Y' || key.getModifiers().isShiftDown();
        redo ? proc.redo() : proc.undo();
        refreshAll();
        return true;
    }
    return false;
}

//==============================================================================
void RollsKillaEditor::loadWavInto (int type)
{
    fileChooser = std::make_unique<juce::FileChooser> ("Your " + juce::String (drumTypeName (drumTypeFromIndex (type))) + " (WAV / AIFF)",
                                                       juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.flac");
    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this, type] (const juce::FileChooser& fc)
                              {
                                  const auto f = fc.getResult();
                                  if (! f.existsAsFile())
                                      return;
                                  const auto error = proc.loadSlotFile (drumTypeFromIndex (type), f);
                                  if (error.isNotEmpty())
                                      kitPanel.showMessage (error.toUpperCase());
                                  else
                                      proc.auditionSlot (drumTypeFromIndex (type));
                                  refreshAll();
                              });
}

void RollsKillaEditor::exportKit()
{
    auto start = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Rolls Killa Kits");
    start.createDirectory();
    fileChooser = std::make_unique<juce::FileChooser> ("Where should the kit go?", start);
    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                              [this] (const juce::FileChooser& fc)
                              {
                                  const auto dir = fc.getResult();
                                  if (! dir.isDirectory())
                                      return;
                                  const auto r = proc.exportKitTo (dir);
                                  kitPanel.showMessage (r.ok ? juce::String (r.wavs) + " SOUNDS + " + juce::String (r.midis) + " MIDI  ->  " + r.folder.getFileName().toUpperCase()
                                                             : r.error.toUpperCase(),
                                                        r.folder);
                              });
}

void RollsKillaEditor::exportOneShots (int count)
{
    auto start = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Rolls Killa Kits");
    start.createDirectory();
    fileChooser = std::make_unique<juce::FileChooser> ("Where should the one-shot kit go?", start);
    const auto type = selected;
    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                              [this, count, type] (const juce::FileChooser& fc)
                              {
                                  const auto dir = fc.getResult();
                                  if (! dir.isDirectory())
                                      return;
                                  const auto r = proc.exportOneShotsTo (dir, drumTypeFromIndex (type), count);
                                  kitPanel.showMessage (r.ok ? juce::String (r.wavs) + " " + juce::String (drumTypeFolder (drumTypeFromIndex (type))).toUpperCase()
                                                                   + "  ->  " + r.folder.getFileName().toUpperCase()
                                                             : r.error.toUpperCase(),
                                                        r.folder);
                              });
}
