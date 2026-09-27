#include "PluginEditor.h"
#include "ui/Sigils.h"

using namespace rk;
using namespace rk::ui;

namespace
{
    // Layout at 100 % (matches docs/design/main_window.webp)
    const juce::Rectangle<int> kVisualizer   { 14, 68, 872, 150 };
    const juce::Rectangle<int> kPresetBar    { 14, 226, 640, 46 };
    const juce::Rectangle<int> kTransport    { 662, 226, 224, 46 };
    const juce::Rectangle<int> kControls     { 14, 280, 606, 168 };
    const juce::Rectangle<int> kSampler      { 628, 280, 258, 168 };
    const juce::Rectangle<int> kKill         { 14, 456, 450, 94 };
    const juce::Rectangle<int> kMidiPanel    { 474, 456, 412, 94 };

    constexpr const char* kUiScaleProp = "uiScale";
}

//==============================================================================
void RollsKillaEditor::Content::paint (juce::Graphics& g)
{
    auto& ed = editor;
    g.fillAll (colours::background);

    // faint grain so the black is not flat
    juce::Random rng (7);
    for (int i = 0; i < 1400; ++i)
    {
        g.setColour (juce::Colours::white.withAlpha (rng.nextFloat() * 0.025f));
        g.fillRect (rng.nextInt (getWidth()), rng.nextInt (getHeight()), 1, 1);
    }
    g.setGradientFill (juce::ColourGradient (colours::accent.withAlpha (0.07f), 120.0f, 0.0f,
                                             juce::Colours::transparentBlack, 120.0f, 120.0f, true));
    g.fillRect (0, 0, 420, 140);

    // logo + subtitle
    const auto scale = ed.uiScale * (float) g.getInternalContext().getPhysicalPixelScaleFactor() / juce::jmax (0.01f, ed.uiScale);
    if (ed.logo.isNull() || std::abs ((float) ed.logo.getHeight() - 60.0f * scale) > 1.0f)
        ed.logo = renderLogo (60, scale);
    g.drawImage (ed.logo, juce::Rectangle<float> (10.0f, 4.0f, (float) ed.logo.getWidth() / scale, 60.0f));

    const auto logoRight = 10.0f + (float) ed.logo.getWidth() / scale;
    g.setColour (colours::text.withAlpha (0.85f));
    g.setFont (labelFont (13.0f).withExtraKerningFactor (0.42f));
    g.drawText ("HI-HAT ROLL PRESETS", juce::Rectangle<float> (logoRight + 14.0f, 18.0f, 260.0f, 26.0f), juce::Justification::centredLeft, false);

    g.setColour (colours::textDim);
    g.setFont (labelFont (10.5f));
    g.drawText ("UI SCALE", juce::Rectangle<int> (636, 18, 70, 26), juce::Justification::centredRight, false);

    // section panels
    drawPanel (g, kTransport.toFloat());
    drawPanel (g, kControls.toFloat());
    drawPanel (g, kSampler.toFloat());
    drawPanel (g, kMidiPanel.toFloat());

    g.setColour (colours::text);
    g.setFont (labelFont (12.5f));
    g.drawText ("ROLL SPEED", juce::Rectangle<int> (kControls.getX() + 16, kControls.getY() + 12, 100, 30), juce::Justification::centredLeft, false);
    g.drawText ("VELOCITY", juce::Rectangle<int> (kControls.getX() + 312, kControls.getY() + 12, 80, 30), juce::Justification::centredLeft, false);
    g.setColour (colours::outline);
    g.drawVerticalLine (kControls.getX() + 302, (float) kControls.getY() + 14.0f, (float) kControls.getY() + 40.0f);
    g.drawHorizontalLine (kControls.getY() + 50, (float) kControls.getX() + 12.0f, (float) kControls.getRight() - 12.0f);
    for (int i = 1; i < 5; ++i)
        g.drawVerticalLine (kControls.getX() + 8 + i * 118, (float) kControls.getY() + 62.0f, (float) kControls.getBottom() - 12.0f);

    g.setColour (colours::textDim);
    g.setFont (labelFont (11.0f));
    g.drawText ("HI-HAT SAMPLE", juce::Rectangle<int> (kSampler.getX() + 12, kSampler.getY() + 6, 200, 18), juce::Justification::centredLeft, false);
    g.drawText ("CHOKE", juce::Rectangle<int> (kSampler.getRight() - 64, kSampler.getY() + 100, 56, 16), juce::Justification::centred, false);
}

//==============================================================================
RollsKillaEditor::RollsKillaEditor (RollsKillaProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      visualizer (p),
      presetBar (p),
      rollSpeed (p.getState(), params::rollSpeed, params::rollSpeedChoices),
      velMode (p.getState(), params::velMode, params::velModeChoices),
      density (p.getState(), params::density, "DENSITY"),
      groove (p.getState(), params::groove, "GROOVE"),
      pitchRamp (p.getState(), params::pitchRamp, "PITCH RAMP"),
      swing (p.getState(), params::swing, "SWING"),
      variation (p.getState(), params::variation, "VARIATION"),
      hatSelector (p.getState(), [&p] { return p.getSampler().getSampleNames(); }),
      tune (p.getState(), params::tune, "TUNE", true),
      decay (p.getState(), params::decay, "DECAY", true),
      volume (p.getState(), params::volume, "VOLUME", true),
      chokeAttachment (p.getState(), params::choke, chokeSwitch),
      browser (p)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (content);

    for (auto pct : { 100, 125, 150 })
    {
        auto* b = scaleButtons.add (new juce::TextButton (juce::String (pct) + "%"));
        b->setMouseCursor (juce::MouseCursor::PointingHandCursor);
        b->onClick = [this, pct] { setUiScale ((float) pct / 100.0f); };
        content.addAndMakeVisible (b);
    }

    content.addAndMakeVisible (visualizer);
    content.addAndMakeVisible (presetBar);
    presetBar.onOpenBrowser = [this] (int category) { browser.open (category); };

    for (auto* b : { &undoButton, &redoButton, &previewButton, &loadWavButton, &exportButton })
        content.addAndMakeVisible (b);
    undoButton.setTooltip ("Undo (Ctrl+Z)");
    redoButton.setTooltip ("Redo (Ctrl+Shift+Z)");
    undoButton.onClick = [this] { processor.undo(); updateStatus(); };
    redoButton.onClick = [this] { processor.redo(); updateStatus(); };
    previewButton.label = "PREVIEW";
    previewButton.setTooltip ("Play the pattern at the preset's tempo while the host is stopped");
    previewButton.onClick = [this] { processor.setPreviewEnabled (! processor.isPreviewEnabled()); updateStatus(); };

    for (auto* c : std::initializer_list<juce::Component*> { &rollSpeed, &velMode, &density, &groove, &pitchRamp, &swing, &variation,
                                                              &hatSelector, &waveform, &tune, &decay, &volume, &chokeSwitch,
                                                              &killButton, &dragZone })
        content.addAndMakeVisible (c);

    density.setTooltip ("Density: fewer (< 100 %) or more (> 100 %) rolls");
    variation.setTooltip ("How much KILL changes the preset");
    pitchRamp.setTooltip ("Pitch ramp added across every roll");

    loadWavButton.label = "LOAD WAV";
    loadWavButton.setTooltip ("Load your own hi-hat (WAV/AIFF) - or drop it on the waveform");
    loadWavButton.onClick = [this] { chooseCustomSample(); };
    waveform.onFileDropped = [this] (const juce::File& f) { processor.loadCustomSample (f); shownHat = -1; };
    hatSelector.onChange = [this] { shownHat = -1; };

    killButton.onClick = [this] { processor.kill(); updateStatus(); };
    killButton.addMouseListener (this, false);
    killButton.onStateChange = [] {};

    dragZone.createFile = [this] { return processor.createDragMidiFile(); };
    dragZone.setTooltip ("Drag the pattern into your DAW (FL Studio: drop on the Playlist or a Piano roll)");
    exportButton.label = "EXPORT .MID";
    exportButton.onClick = [this] { exportMidi(); };

    content.addChildComponent (browser);
    browser.onPresetLoaded = [this] { updateStatus(); };

    waveform.setSample (processor.getSampler().getSample (hatSelector.getIndex()));
    shownHat = hatSelector.getIndex();

    uiScale = (float) (double) processor.getState().state.getProperty (kUiScaleProp, 1.0);
    setWantsKeyboardFocus (true);
    layout();
    setUiScale (uiScale);
    updateStatus();
    startTimerHz (15);
}

RollsKillaEditor::~RollsKillaEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void RollsKillaEditor::setUiScale (float scale)
{
    uiScale = juce::jlimit (1.0f, 1.5f, scale);
    processor.getState().state.setProperty (kUiScaleProp, uiScale, nullptr);
    content.setTransform (juce::AffineTransform::scale (uiScale));
    logo = {};
    setSize (juce::roundToInt (kBaseWidth * uiScale), juce::roundToInt (kBaseHeight * uiScale));

    const int pcts[] { 100, 125, 150 };
    for (int i = 0; i < scaleButtons.size(); ++i)
        scaleButtons[i]->setToggleState (juce::roundToInt (uiScale * 100.0f) == pcts[i], juce::dontSendNotification);
}

void RollsKillaEditor::resized()
{
    content.setBounds (0, 0, kBaseWidth, kBaseHeight);
}

void RollsKillaEditor::layout()
{
    for (int i = 0; i < scaleButtons.size(); ++i)
        scaleButtons[i]->setBounds (714 + i * 58, 16, 54, 30);

    visualizer.setBounds (kVisualizer);
    presetBar.setBounds (kPresetBar);

    auto t = kTransport.reduced (8, 7);
    undoButton.setBounds (t.removeFromLeft (38));
    t.removeFromLeft (6);
    redoButton.setBounds (t.removeFromLeft (38));
    t.removeFromLeft (8);
    previewButton.setBounds (t);

    const auto c = kControls;
    rollSpeed.setBounds (c.getX() + 110, c.getY() + 12, 180, 30);
    velMode.setBounds (c.getX() + 392, c.getY() + 12, 202, 30);
    int x = c.getX() + 12;
    for (auto* k : { &density, &groove, &pitchRamp, &swing, &variation })
    {
        k->setBounds (x, c.getY() + 58, 110, 104);
        x += 118;
    }

    const auto s = kSampler;
    hatSelector.setBounds (s.getX() + 10, s.getY() + 26, 132, 28);
    loadWavButton.setBounds (s.getX() + 148, s.getY() + 26, 100, 28);
    waveform.setBounds (s.getX() + 10, s.getY() + 60, 238, 34);
    tune.setBounds (s.getX() + 6, s.getY() + 96, 62, 70);
    decay.setBounds (s.getX() + 68, s.getY() + 96, 62, 70);
    volume.setBounds (s.getX() + 130, s.getY() + 96, 62, 70);
    chokeSwitch.setBounds (s.getRight() - 60, s.getY() + 122, 48, 30);

    killButton.setBounds (kKill);
    dragZone.setBounds (kMidiPanel.getX() + 10, kMidiPanel.getY() + 10, 270, kMidiPanel.getHeight() - 20);
    exportButton.setBounds (kMidiPanel.getRight() - 126, kMidiPanel.getCentreY() - 20, 116, 40);

    browser.setBounds (0, 0, kBaseWidth, kBaseHeight);
}

void RollsKillaEditor::updateStatus()
{
    undoButton.setEnabled (processor.canUndo());
    redoButton.setEnabled (processor.canRedo());

    const auto hostPlaying = processor.isHostPlaying();
    const auto previewing = processor.isPreviewEnabled() && ! hostPlaying;
    previewButton.setToggleState (previewing, juce::dontSendNotification);
    previewButton.setIcon (previewing ? Icon::stop : Icon::play);
    previewButton.label = hostPlaying ? "HOST SYNC" : previewing ? "STOP" : "PREVIEW";
    previewButton.setEnabled (! hostPlaying);
    previewButton.repaint();

    const auto seed = processor.getSeed();
    killButton.seedText = seed != 0 ? "#" + juce::String ((int) seed) : juce::String();
    killButton.repaint();

    presetBar.refresh();
}

void RollsKillaEditor::timerCallback()
{
    const auto version = processor.getPatternVersion();
    if (version != shownPatternVersion)
    {
        shownPatternVersion = version;
        presetBar.refresh();
    }

    const auto hat = hatSelector.getIndex();
    if (hat != shownHat)
    {
        shownHat = hat;
        waveform.setSample (processor.getSampler().getSample (hat));
    }

    updateStatus();
}

bool RollsKillaEditor::keyPressed (const juce::KeyPress& key)
{
    const auto cmd = key.getModifiers().isCommandDown();
    if (cmd && key.getKeyCode() == 'Z')
    {
        key.getModifiers().isShiftDown() ? processor.redo() : processor.undo();
        updateStatus();
        return true;
    }
    if (cmd && key.getKeyCode() == 'Y')
    {
        processor.redo();
        updateStatus();
        return true;
    }
    if (key == juce::KeyPress::leftKey || key == juce::KeyPress::rightKey)
    {
        processor.stepPreset (key == juce::KeyPress::leftKey ? -1 : 1);
        updateStatus();
        return true;
    }
    return false;
}

void RollsKillaEditor::chooseCustomSample()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Load a hi-hat sample", juce::File(), "*.wav;*.aif;*.aiff;*.flac");
    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  const auto file = fc.getResult();
                                  if (file.existsAsFile())
                                  {
                                      const auto error = processor.loadCustomSample (file);
                                      if (error.isNotEmpty())
                                          juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Rolls Killa", error);
                                      shownHat = -1;
                                  }
                              });
}

void RollsKillaEditor::exportMidi()
{
    const auto name = "Rolls Killa - " + processor.getModel().getPreset().name + ".mid";
    fileChooser = std::make_unique<juce::FileChooser> ("Export MIDI",
                                                       juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                                           .getChildFile (juce::File::createLegalFileName (name)),
                                                       "*.mid");
    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc)
                              {
                                  auto file = fc.getResult();
                                  if (file == juce::File())
                                      return;
                                  if (! file.hasFileExtension ("mid"))
                                      file = file.withFileExtension ("mid");
                                  if (! processor.exportMidi (file))
                                      juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Rolls Killa",
                                                                              "Could not write " + file.getFullPathName());
                              });
}
