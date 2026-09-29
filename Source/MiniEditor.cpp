#include "MiniEditor.h"
#include "ui/Sigils.h"

using namespace rk;
using namespace rk::ui;

namespace
{
    // Layout at 100 % (matches docs/design/mini_window.webp)
    const juce::Rectangle<int> kHeader     { 6, 6, 588, 30 };
    const juce::Rectangle<int> kVisualizer { 6, 40, 588, 78 };
    const juce::Rectangle<int> kHistory    { 6, 119, 588, 12 };
    const juce::Rectangle<int> kMood       { 6, 134, 210, 16 };
    const juce::Rectangle<int> kKill       { 6, 151, 210, 55 };
    const juce::Rectangle<int> kHat        { 222, 134, 150, 72 };
    const juce::Rectangle<int> kDrag       { 378, 134, 122, 72 };
    const juce::Rectangle<int> kPlay       { 506, 134, 88, 72 };

    constexpr const char* kMiniScaleProp = "miniUiScale";
}

//==============================================================================
void RollsKillaMiniEditor::Content::paint (juce::Graphics& g)
{
    auto& ed = editor;
    g.fillAll (colours::background);

    juce::Random rng (11);
    for (int i = 0; i < 500; ++i)
    {
        g.setColour (juce::Colours::white.withAlpha (rng.nextFloat() * 0.025f));
        g.fillRect (rng.nextInt (getWidth()), rng.nextInt (getHeight()), 1, 1);
    }

    // header
    drawPanel (g, kHeader.toFloat(), 5.0f);
    const auto scale = (float) g.getInternalContext().getPhysicalPixelScaleFactor();
    if (ed.logo.isNull() || std::abs ((float) ed.logo.getHeight() - 28.0f * scale) > 1.0f)
        ed.logo = renderLogo (28, scale);
    const auto logoW = (float) ed.logo.getWidth() / scale;
    g.drawImage (ed.logo, juce::Rectangle<float> (12.0f, 7.0f, logoW, 28.0f));

    // "MINI" tag in the logo's red, slanted like the logo
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)).withExtraKerningFactor (0.08f), "MINI", 0.0f, 0.0f);
        juce::Path mini;
        ga.createPath (mini);
        mini.applyTransform (juce::AffineTransform::shear (-0.2f, 0.0f).translated (12.0f + logoW + 3.0f, 26.0f));
        g.setColour (colours::accent.withAlpha (0.25f));
        g.strokePath (mini, juce::PathStrokeType (2.0f));
        g.setColour (colours::accent);
        g.fillPath (mini);
    }

    const auto x0 = 12.0f + logoW + 38.0f;
    g.setColour (colours::outlineLight);
    g.drawVerticalLine ((int) x0, 13.0f, 29.0f);
    g.setColour (colours::text.withAlpha (0.85f));
    g.setFont (labelFont (8.5f).withExtraKerningFactor (0.4f));
    g.drawText ("HI-HAT ROLLS", juce::Rectangle<float> (x0 + 8.0f, 6.0f, 300.0f - x0 - 8.0f, 30.0f), juce::Justification::centredLeft, true);

    // what plays now (display only - KILL is the way to change it)
    const auto& preset = ed.proc.getModel().getPreset();
    auto now = juce::Rectangle<float> (364.0f, 6.0f, 88.0f, 30.0f);
    g.setColour (colours::textDim);
    g.setFont (labelFont (7.5f));
    g.drawText (juce::String (getCategoryProfile (preset.category).name), now.removeFromTop (17.0f), juce::Justification::bottomLeft, true);
    g.setColour (colours::text.withAlpha (0.8f));
    g.setFont (uiFont (9.5f, false));
    g.drawText (preset.name, now, juce::Justification::topLeft, true);

    g.setColour (colours::textDim);
    g.setFont (labelFont (9.0f));
    g.drawText ("BARS", juce::Rectangle<float> (505.0f, 6.0f, 30.0f, 30.0f), juce::Justification::centredLeft, false);

    // bottom tiles
    drawPanel (g, kHat.toFloat(), 5.0f);
    g.setColour (colours::text.withAlpha (0.85f));
    g.setFont (labelFont (8.5f));
    g.drawText ("HI-HAT SAMPLE", kHat.withHeight (18).translated (0, 2), juce::Justification::centred, false);
    drawPanel (g, kPlay.toFloat(), 5.0f);
}

void RollsKillaMiniEditor::Content::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu() && kHeader.contains (e.getPosition()))
        editor.showScaleMenu();
}

//==============================================================================
void RollsKillaMiniEditor::PlayTile::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat();
    auto icon = r.removeFromTop (r.getHeight() * 0.62f);
    const auto s = juce::jmin (icon.getWidth(), icon.getHeight()) * 0.6f * (down ? 0.92f : 1.0f);
    const auto c = icon.getCentre();
    const auto col = hostPlaying ? colours::textDim : colours::accent.brighter (highlighted ? 0.25f : 0.0f);

    juce::Path shape;
    if (previewing)
        shape.addRoundedRectangle (c.x - s * 0.36f, c.y - s * 0.36f, s * 0.72f, s * 0.72f, 2.0f);
    else
        shape.addTriangle (c.x - s * 0.38f, c.y - s * 0.46f, c.x - s * 0.38f, c.y + s * 0.46f, c.x + s * 0.46f, c.y);

    if (! hostPlaying)
        for (int i = 3; i > 0; --i)
        {
            g.setColour (colours::accent.withAlpha (0.12f));
            g.strokePath (shape, juce::PathStrokeType ((float) i * 2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    g.setGradientFill (juce::ColourGradient (col.brighter (0.3f), c.x, c.y - s * 0.5f, col.darker (0.4f), c.x, c.y + s * 0.5f, false));
    g.fillPath (shape);

    g.setColour (colours::text.withAlpha (hostPlaying ? 0.5f : 0.9f));
    g.setFont (labelFont (10.0f).withExtraKerningFactor (0.35f));
    g.drawText (hostPlaying ? "SYNC" : previewing ? "STOP" : "PLAY", r.withTrimmedTop (-6.0f), juce::Justification::centredTop, false);
}

//==============================================================================
RollsKillaMiniEditor::RollsKillaMiniEditor (RollsKillaProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      visualizer (p, true),
      barsChoice (p.getState(), params::bars, params::barsChoices),
      hatSelector (p.getState(), [&p] { return p.getSampler().getSampleNames(); }),
      playMode (p.getState(), params::playMode, { "MIDI", "HOST" })
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (content);

    for (auto* c : std::initializer_list<juce::Component*> { &visualizer, &barsChoice, &undoButton, &redoButton, &killButton,
                                                              &waveform, &hatSelector, &dragZone, &playTile, &playMode })
        content.addAndMakeVisible (c);

    content.addAndMakeVisible (bpmBox);
    content.addAndMakeVisible (history);
    bpmBox.setTooltip ("Tempo KILL builds rolls for. AUTO = follows the project. Drag up/down to set it, double-click = AUTO.");
    history.setTooltip ("The last KILLs - click a dot to go back to that roll");

    const char* moods[] { "CHILL", "TRAP", "CRAZY" };
    const char* moodTips[] { "KILL picks simple, calm rolls", "KILL picks classic trap rolls", "KILL picks the craziest rolls" };
    for (int i = 0; i < 3; ++i)
    {
        auto* b = moodButtons.add (new juce::TextButton (moods[i]));
        b->setTooltip (moodTips[i]);
        b->setMouseCursor (juce::MouseCursor::PointingHandCursor);
        b->onClick = [this, i] { setMood (i); };
        content.addAndMakeVisible (b);
    }
    setMood (proc.getMood());

    undoButton.setTooltip ("Undo - e.g. back to the previous KILL (Ctrl+Z)");
    redoButton.setTooltip ("Redo (Ctrl+Shift+Z)");
    undoButton.onClick = [this] { proc.undo(); updateStatus(); };
    redoButton.onClick = [this] { proc.redo(); updateStatus(); };

    killButton.caption = "MILLIONS OF COMBINATIONS";
    killButton.setTooltip ("KILL - a new roll: random preset + new variation (right-click: same preset / back to original)");
    killButton.onClick = [this] { proc.killEverything(); updateStatus(); };
    killButton.onSamePresetVariation = [this] { proc.kill(); updateStatus(); };
    killButton.onBackToOriginal = [this] { proc.resetVariation(); updateStatus(); };

    waveform.onFileDropped = [this] (const juce::File& f) { proc.loadCustomSample (f); shownHat = -1; };
    hatSelector.onChange = [this] { shownHat = -1; };
    waveform.setSample (proc.getSampler().getSample (hatSelector.getIndex()));
    shownHat = hatSelector.getIndex();

    dragZone.vertical = true;
    dragZone.createFile = [this] { return proc.createDragMidiFile(); };
    dragZone.setTooltip ("Drag the pattern into your DAW (FL Studio: drop on the Playlist or a Piano roll)");

    playTile.onClick = [this] { proc.setPreviewEnabled (! proc.isPreviewEnabled()); updateStatus(); };
    playTile.setTooltip ("Play the pattern at the preset's tempo while the host is stopped");
    playMode.setTooltip ("MIDI: plays only while a note is held on this channel. HOST: plays whenever the host plays.");

    uiScale = (float) (double) proc.getState().state.getProperty (kMiniScaleProp, 1.25);  // 125 % default: small but readable
    setWantsKeyboardFocus (true);
    layout();
    setUiScale (uiScale);
    updateStatus();
    startTimerHz (15);
}

RollsKillaMiniEditor::~RollsKillaMiniEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void RollsKillaMiniEditor::setMood (int mood)
{
    proc.setMood (mood);
    for (int i = 0; i < moodButtons.size(); ++i)
        moodButtons[i]->setToggleState (i == proc.getMood(), juce::dontSendNotification);
}

//==============================================================================
void RollsKillaMiniEditor::BpmBox::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    const auto manual = proc.getTargetBpm() > 0.0;
    g.setColour (colours::background);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (manual ? colours::accent.withAlpha (0.8f) : colours::outline);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);

    auto top = r.removeFromTop (r.getHeight() * 0.62f);
    g.setColour (colours::text);
    g.setFont (uiFont (11.0f));
    g.drawText (juce::String (juce::roundToInt (proc.getEffectiveBpm())), top, juce::Justification::centredBottom, false);
    g.setColour (manual ? colours::accent : colours::textDark);
    g.setFont (labelFont (6.5f));
    g.drawText (manual ? "BPM" : "AUTO BPM", r, juce::Justification::centredTop, false);
}

void RollsKillaMiniEditor::BpmBox::mouseDrag (const juce::MouseEvent& e)
{
    proc.setTargetBpm (std::round (startBpm - e.getDistanceFromDragStartY() * 0.5));
    repaint();
}

juce::Rectangle<float> RollsKillaMiniEditor::HistoryStrip::dotBounds (int i, int count) const
{
    const auto spacing = 14.0f;
    const auto x0 = getWidth() * 0.5f - (count - 1) * spacing * 0.5f;
    return juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ x0 + i * spacing, getHeight() * 0.5f });
}

void RollsKillaMiniEditor::HistoryStrip::paint (juce::Graphics& g)
{
    const auto& h = proc.getKillHistory();
    const auto count = (int) h.size();
    if (count == 0)
        return;

    for (int i = 0; i < count; ++i)
    {
        auto d = dotBounds (i, count);
        const auto current = i == proc.getKillHistoryPosition();
        if (current)
        {
            g.setColour (colours::accent.withAlpha (0.3f));
            g.fillEllipse (d.expanded (2.5f));
        }
        g.setColour (current ? colours::accent : (i == hover ? colours::text : colours::outlineLight));
        g.fillEllipse (current || i == hover ? d : d.reduced (1.0f));
    }
}

void RollsKillaMiniEditor::HistoryStrip::mouseMove (const juce::MouseEvent& e)
{
    const auto& h = proc.getKillHistory();
    int found = -1;
    for (int i = 0; i < (int) h.size(); ++i)
        if (dotBounds (i, (int) h.size()).expanded (4.0f).contains (e.position))
            found = i;
    if (found != hover)
    {
        hover = found;
        setTooltip (found >= 0 ? h[(size_t) found].label : juce::String ("The last KILLs - click a dot to go back to that roll"));
        setMouseCursor (found >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void RollsKillaMiniEditor::HistoryStrip::mouseUp (const juce::MouseEvent&)
{
    if (hover >= 0)
    {
        proc.restoreKillHistory (hover);
        repaint();
    }
}

void RollsKillaMiniEditor::layout()
{
    visualizer.setBounds (kVisualizer);
    barsChoice.setBounds (532, 12, 58, 18);
    undoButton.setBounds (306, 11, 24, 20);
    redoButton.setBounds (334, 11, 24, 20);
    bpmBox.setBounds (456, 10, 44, 22);
    history.setBounds (kHistory);
    for (int i = 0; i < moodButtons.size(); ++i)
        moodButtons[i]->setBounds (kMood.getX() + i * (kMood.getWidth() / 3), kMood.getY(), kMood.getWidth() / 3 - (i < 2 ? 2 : 0), kMood.getHeight());
    killButton.setBounds (kKill);
    waveform.setBounds (kHat.getX() + 8, kHat.getY() + 20, kHat.getWidth() - 16, 26);
    hatSelector.setBounds (kHat.getX() + 6, kHat.getBottom() - 24, kHat.getWidth() - 12, 20);
    dragZone.setBounds (kDrag);
    playTile.setBounds (kPlay.getX(), kPlay.getY() + 2, kPlay.getWidth(), 48);
    playMode.setBounds (kPlay.getX() + 8, kPlay.getBottom() - 20, kPlay.getWidth() - 16, 15);
}

void RollsKillaMiniEditor::setUiScale (float scale)
{
    uiScale = juce::jlimit (1.0f, 2.0f, scale);
    proc.getState().state.setProperty (kMiniScaleProp, uiScale, nullptr);
    content.setTransform (juce::AffineTransform::scale (uiScale));
    logo = {};
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
    if (playTile.previewing != previewing || playTile.hostPlaying != hostPlaying)
    {
        playTile.previewing = previewing;
        playTile.hostPlaying = hostPlaying;
        playTile.repaint();
    }
    playTile.setEnabled (! hostPlaying);
    undoButton.setEnabled (proc.canUndo());
    redoButton.setEnabled (proc.canRedo());
    bpmBox.repaint();
    history.repaint();

    const auto seed = proc.getSeed();
    killButton.seedText = seed != 0 ? "#" + juce::String ((int) seed) : juce::String();
    content.repaint (0, 0, kBaseWidth, 40);
}

void RollsKillaMiniEditor::timerCallback()
{
    const auto hat = hatSelector.getIndex();
    if (hat != shownHat)
    {
        shownHat = hat;
        waveform.setSample (proc.getSampler().getSample (hat));
    }
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
