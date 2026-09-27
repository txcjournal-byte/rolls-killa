#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rk;

RollsKillaProcessor::RollsKillaProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "RollsKilla", params::createLayout (sampler.getSampleNames()))
{
    library.setUserDirectory (RollLibrary::defaultUserDirectory());

    for (auto* param : getParameters())
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (param))
            apvts.addParameterListener (p->getParameterID(), this);

    raw.hat = apvts.getRawParameterValue (params::hat);
    raw.tune = apvts.getRawParameterValue (params::tune);
    raw.decay = apvts.getRawParameterValue (params::decay);
    raw.choke = apvts.getRawParameterValue (params::choke);
    raw.volume = apvts.getRawParameterValue (params::volume);

    rebuildPattern (true);
    history.reset (apvts.copyState());
    startTimerHz (30);
}

RollsKillaProcessor::~RollsKillaProcessor()
{
    stopTimer();
    for (auto* param : getParameters())
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (param))
            apvts.removeParameterListener (p->getParameterID(), this);
}

//==============================================================================
ModelSettings RollsKillaProcessor::readModelSettings() const
{
    auto value = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    ModelSettings s;
    s.presetIndex = (int) value (params::preset);
    s.bars = params::barsFromChoice ((int) value (params::bars));
    s.seed = (uint32_t) value (params::seed);
    s.variation = juce::roundToInt (value (params::variation));
    s.lockedBars = getLockedBars();
    s.shape.speedShift = (int) value (params::rollSpeed) - 1;
    s.shape.density = juce::roundToInt (value (params::density));
    s.shape.velMode = (VelocityMode) (int) value (params::velMode);
    s.shape.groove = juce::roundToInt (value (params::groove));
    s.shape.pitchRamp = (int) value (params::pitchRamp);
    s.shape.swing = juce::roundToInt (value (params::swing));
    s.edits = getEdits();
    return s;
}

HatSamplerSettings RollsKillaProcessor::readSamplerSettings() const noexcept
{
    HatSamplerSettings s;
    s.sampleIndex = (int) raw.hat->load();
    s.tuneSemitones = raw.tune->load();
    s.decayMs = raw.decay->load();
    s.choke = raw.choke->load() > 0.5f;
    s.volumeDb = raw.volume->load();
    return s;
}

void RollsKillaProcessor::rebuildPattern (bool force)
{
    patternDirty.store (false);

    if (model.update (readModelSettings(), force) || force)
    {
        patternSlot.publish (model.makePlayback (kRootNote));
        ++patternVersion;
    }
}

void RollsKillaProcessor::parameterChanged (const juce::String&, float)
{
    // May be called on the audio thread (automation): only flag, the timer does the work.
    patternDirty.store (true);
    stateChanged.store (true);
    lastChangeMs.store (juce::Time::getMillisecondCounter());
}

void RollsKillaProcessor::commitUndoStep()
{
    stateChanged.store (false);
    history.push (apvts.copyState());
}

void RollsKillaProcessor::markStateChanged()
{
    patternDirty.store (true);
    stateChanged.store (true);
    lastChangeMs.store (juce::Time::getMillisecondCounter());
}

void RollsKillaProcessor::timerCallback()
{
    if (patternDirty.load())
        rebuildPattern();

    // Record an undo step once the user stopped touching things for a moment (knob drags = 1 step).
    if (stateChanged.load() && juce::Time::getMillisecondCounter() - lastChangeMs.load() > 350)
        commitUndoStep();

    patternSlot.collectGarbage();
}

//==============================================================================
namespace
{
    constexpr const char* kLockedBarsProp = "lockedBars";
    constexpr const char* kEditsType = "Edits";
    constexpr const char* kEditType = "Edit";
    constexpr const char* kCustomSampleProp = "customSample";
    constexpr const char* kPresetIdProp = "presetId";
}

void RollsKillaProcessor::setParam (const char* id, float plainValue)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
        p->endChangeGesture();
    }
}

int RollsKillaProcessor::getPresetIndex() const
{
    return (int) apvts.getRawParameterValue (params::preset)->load();
}

uint32_t RollsKillaProcessor::getSeed() const
{
    return (uint32_t) apvts.getRawParameterValue (params::seed)->load();
}

void RollsKillaProcessor::loadPreset (int index)
{
    index = juce::jlimit (0, library.getNumPresets() - 1, index);
    apvts.state.removeChild (apvts.state.getChildWithName (kEditsType), nullptr);
    setParam (params::seed, 0.0f);
    setParam (params::preset, (float) index);
    rebuildPattern();
    commitUndoStep();
}

void RollsKillaProcessor::stepPreset (int delta)
{
    const auto n = library.getNumPresets();
    if (n > 0)
        loadPreset (((getPresetIndex() + delta) % n + n) % n);
}

void RollsKillaProcessor::kill()
{
    auto seed = (int) getSeed();
    auto& rng = juce::Random::getSystemRandom();
    int next = seed;
    while (next == seed)
        next = rng.nextInt ({ 1, params::kMaxSeed + 1 });

    apvts.state.removeChild (apvts.state.getChildWithName (kEditsType), nullptr);
    setParam (params::seed, (float) next);
    rebuildPattern();
    commitUndoStep();
}

void RollsKillaProcessor::resetVariation()
{
    apvts.state.removeChild (apvts.state.getChildWithName (kEditsType), nullptr);
    setParam (params::seed, 0.0f);
    rebuildPattern();
    commitUndoStep();
}

uint32_t RollsKillaProcessor::getLockedBars() const
{
    return (uint32_t) (int) apvts.state.getProperty (kLockedBarsProp, 0);
}

void RollsKillaProcessor::setBarLocked (int bar, bool locked)
{
    auto mask = getLockedBars();
    mask = locked ? (mask | (1u << bar)) : (mask & ~(1u << bar));
    apvts.state.setProperty (kLockedBarsProp, (int) mask, nullptr);
    rebuildPattern();
    commitUndoStep();
}

NoteEdits RollsKillaProcessor::getEdits() const
{
    NoteEdits edits;
    const auto tree = apvts.state.getChildWithName (kEditsType);
    for (const auto& child : tree)
    {
        NoteEdit e;
        e.muted = child.getProperty ("muted", false);
        e.vel = child.getProperty ("vel", -1);
        e.rate = child.getProperty ("rate", -1);
        edits[(int) child.getProperty ("tick", 0)] = e;
    }
    return edits;
}

void RollsKillaProcessor::setNoteEdit (int tick, const NoteEdit& edit)
{
    auto tree = apvts.state.getOrCreateChildWithName (kEditsType, nullptr);
    auto existing = tree.getChildWithProperty ("tick", tick);

    if (edit.isEmpty())
    {
        tree.removeChild (existing, nullptr);
    }
    else
    {
        if (! existing.isValid())
        {
            existing = juce::ValueTree (kEditType);
            existing.setProperty ("tick", tick, nullptr);
            tree.appendChild (existing, nullptr);
        }
        existing.setProperty ("muted", edit.muted, nullptr);
        existing.setProperty ("vel", edit.vel, nullptr);
        existing.setProperty ("rate", edit.rate, nullptr);
    }

    rebuildPattern();
    markStateChanged();
}

void RollsKillaProcessor::clearEdits()
{
    apvts.state.removeChild (apvts.state.getChildWithName (kEditsType), nullptr);
    rebuildPattern();
    markStateChanged();
}

//==============================================================================
void RollsKillaProcessor::applyRestoredState()
{
    const auto customPath = apvts.state.getProperty (kCustomSampleProp).toString();
    if (customPath.isNotEmpty() && juce::File (customPath) != sampler.getCustomSampleFile())
        sampler.loadCustomSample (juce::File (customPath));

    rebuildPattern (true);
}

bool RollsKillaProcessor::undo()
{
    auto state = history.undo();
    if (! state.isValid())
        return false;
    apvts.replaceState (state);
    stateChanged.store (false);
    applyRestoredState();
    return true;
}

bool RollsKillaProcessor::redo()
{
    auto state = history.redo();
    if (! state.isValid())
        return false;
    apvts.replaceState (state);
    stateChanged.store (false);
    applyRestoredState();
    return true;
}

juce::String RollsKillaProcessor::loadCustomSample (const juce::File& file)
{
    const auto error = sampler.loadCustomSample (file);
    if (error.isNotEmpty())
        return error;

    apvts.state.setProperty (kCustomSampleProp, file.getFullPathName(), nullptr);
    setParam (params::hat, (float) HatSampler::kCustomSlot);
    commitUndoStep();
    return {};
}

MidiExportOptions RollsKillaProcessor::getMidiExportOptions() const
{
    MidiExportOptions o;
    o.bpm = model.getPreset().bpmHint;
    o.rootNote = kRootNote;
    o.trackName = "Rolls Killa - " + model.getPreset().name;
    return o;
}

juce::File RollsKillaProcessor::createDragMidiFile()
{
    const auto& preset = model.getPreset();
    auto name = "Rolls Killa - " + preset.name;
    if (getSeed() != 0)
        name << " KILL " << (int) getSeed();
    return createDragAndDropMidiFile (model.getPattern(), getMidiExportOptions(), name);
}

bool RollsKillaProcessor::exportMidi (const juce::File& file)
{
    return writeMidiFile (model.getPattern(), getMidiExportOptions(), file);
}

int RollsKillaProcessor::saveUserPreset (const juce::String& name)
{
    const auto& preset = model.getPreset();
    const auto idx = library.saveUserPreset (name, model.getPattern(), preset.bpmHint, preset.tags);
    if (idx >= 0)
        loadPreset (idx);
    return idx;
}

void RollsKillaProcessor::setPreviewEnabled (bool shouldPreview) noexcept
{
    if (shouldPreview && ! previewEnabled.load())
        previewRestart.store (true);
    previewEnabled.store (shouldPreview);
}

//==============================================================================
void RollsKillaProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    sampler.prepare (sampleRate);
    player.prepare (sampleRate);
    previewPpq = 0.0;
}

bool RollsKillaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void RollsKillaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    buffer.clear();

    // ---- transport ---------------------------------------------------------
    TransportState transport;
    bool hostIsPlaying = false;

    if (auto* ph = getPlayHead())
    {
        if (const auto pos = ph->getPosition())
        {
            hostIsPlaying = pos->getIsPlaying();
            transport.playing = hostIsPlaying;
            transport.ppq = pos->getPpqPosition().orFallback (0.0);
            transport.bpm = pos->getBpm().orFallback (120.0);
        }
    }

    hostPlaying.store (hostIsPlaying, std::memory_order_relaxed);
    const auto* pattern = patternSlot.acquire();

    if (! hostIsPlaying && previewEnabled.load (std::memory_order_relaxed))
    {
        if (previewRestart.exchange (false))
            previewPpq = 0.0;

        transport.playing = true;
        transport.bpm = pattern != nullptr ? pattern->bpmHint : 140.0;
        transport.ppq = previewPpq;
        previewPpq += numSamples * transport.bpm / 60.0 / currentSampleRate;
    }

    const auto numEvents = player.process (pattern, transport, numSamples, events);
    patternSlot.release();

    // ---- live input: incoming note-ons play the sampler too (copied, no allocation)
    int numInput = 0;
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn() && numInput < (int) inputNotes.size())
            inputNotes[(size_t) numInput++] = { meta.samplePosition, m.getNoteNumber(), (int) m.getVelocity() };
    }

    midi.clear();   // output = generated pattern only (keeps the host's preallocated storage)

    // ---- render ------------------------------------------------------------
    sampler.beginBlock (readSamplerSettings());

    int rendered = 0;
    int nextInput = 0;

    auto renderUpTo = [&] (int offset)
    {
        offset = juce::jlimit (0, numSamples, offset);
        if (offset > rendered)
        {
            sampler.render (buffer, rendered, offset - rendered);
            rendered = offset;
        }
    };

    auto playInputUpTo = [&] (int offset)
    {
        for (; nextInput < numInput && inputNotes[(size_t) nextInput].sampleOffset <= offset; ++nextInput)
        {
            const auto& in = inputNotes[(size_t) nextInput];
            renderUpTo (in.sampleOffset);
            sampler.noteOn (in.note, in.vel);
        }
    };

    for (int i = 0; i < numEvents; ++i)
    {
        const auto& e = events[(size_t) i];
        playInputUpTo (e.sampleOffset);
        renderUpTo (e.sampleOffset);

        if (e.vel > 0)
        {
            sampler.noteOn (e.note, e.vel);
            midi.addEvent (juce::MidiMessage::noteOn (1, e.note, (juce::uint8) e.vel), e.sampleOffset);
        }
        else
        {
            midi.addEvent (juce::MidiMessage::noteOff (1, e.note), e.sampleOffset);
        }
    }

    playInputUpTo (numSamples);
    renderUpTo (numSamples);
    sampler.endBlock();
}

//==============================================================================
juce::AudioProcessorEditor* RollsKillaProcessor::createEditor()
{
    return new RollsKillaEditor (*this);
}

void RollsKillaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty (kPresetIdProp, model.getPreset().id, nullptr);
    state.setProperty ("version", JucePlugin_VersionString, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void RollsKillaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto state = juce::ValueTree::fromXml (*xml);
    apvts.replaceState (state);

    // The preset is stored by id too, so a project still finds it if the library order changes.
    const auto idx = library.indexOfId (state.getProperty (kPresetIdProp).toString());
    if (idx >= 0)
        if (auto* p = apvts.getParameter (params::preset))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) idx));

    applyRestoredState();
    history.reset (apvts.copyState());
    stateChanged.store (false);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RollsKillaProcessor();
}
