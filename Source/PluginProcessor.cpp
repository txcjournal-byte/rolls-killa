#include "PluginProcessor.h"
#include "MiniEditor.h"
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
    raw.playMode = apvts.getRawParameterValue (params::playMode);
    raw.puff = apvts.getRawParameterValue (params::puff);

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
    previewBpm.store (getEffectiveBpm());

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

void RollsKillaProcessor::killEverything()
{
    auto& rng = juce::Random::getSystemRandom();
    const auto numFactory = library.getNumFactoryPresets();
    if (numFactory <= 0)
        return;

    auto pick = [&rng] (std::initializer_list<std::pair<float, float>> weighted)
    {
        float total = 0.0f;
        for (const auto& w : weighted) total += w.second;
        auto r = rng.nextFloat() * total;
        for (const auto& w : weighted)
        {
            if (r < w.second) return w.first;
            r -= w.second;
        }
        return weighted.begin()->first;
    };

    // Presets that fit the project tempo (half/double time counts as fitting) and the mood
    const auto bpm = getEffectiveBpm();
    const auto mood = getMood();
    std::vector<float> weights ((size_t) numFactory, 0.0f);
    float totalWeight = 0.0f;
    for (int i = 0; i < numFactory; ++i)
    {
        const auto& p = library.getPreset (i);
        const auto siblings = library.presetsInCategory (p.category);
        const auto inCategory = (int) std::distance (siblings.begin(), std::find (siblings.begin(), siblings.end(), i));
        const auto moodOf = inCategory <= 2 ? 0 : inCategory <= 5 ? 1 : 2;
        if (moodOf != mood || i == getPresetIndex())
            continue;
        const auto dist = std::min ({ std::abs (p.bpmHint - bpm), std::abs (p.bpmHint - 2.0 * bpm), std::abs (2.0 * p.bpmHint - bpm) });
        const auto w = (float) std::exp (-(dist / 12.0) * (dist / 12.0)) + 0.03f;
        weights[(size_t) i] = w;
        totalWeight += w;
    }

    auto preset = getPresetIndex();
    if (totalWeight > 0.0f)
    {
        auto r = rng.nextFloat() * totalWeight;
        for (int i = 0; i < numFactory; ++i)
        {
            if (r < weights[(size_t) i]) { preset = i; break; }
            r -= weights[(size_t) i];
        }
    }

    // slow projects get faster rolls, fast projects calmer ones; CRAZY pushes speed and variation
    const auto slow = bpm < 118.0, fast = bpm > 158.0;
    const auto faster = mood == 2 ? 0.45f : slow ? 0.45f : fast ? 0.1f : 0.25f;
    const auto slower = mood == 0 ? 0.25f : fast ? 0.35f : slow ? 0.05f : 0.15f;

    // 96 presets x 99,999 seeds x speed x variation x velocity x swing x density
    // -> tens of millions of rolls, all built on presets that follow the kits
    apvts.state.removeChild (apvts.state.getChildWithName (kEditsType), nullptr);
    setParam (params::preset, (float) preset);
    setParam (params::seed, (float) rng.nextInt ({ 1, params::kMaxSeed + 1 }));
    setParam (params::variation, (float) (mood == 0 ? rng.nextInt ({ 20, 61 }) : mood == 2 ? rng.nextInt ({ 60, 101 }) : rng.nextInt ({ 35, 81 })));
    setParam (params::rollSpeed, pick ({ { 1.0f, juce::jmax (0.1f, 1.0f - faster - slower) }, { 2.0f, faster }, { 0.0f, slower } }));
    setParam (params::velMode, pick ({ { 0.0f, 0.75f }, { 2.0f, 0.15f }, { 3.0f, 0.10f } }));
    setParam (params::swing, pick ({ { 0.0f, 0.7f }, { 10.0f, 0.15f }, { 20.0f, 0.15f } }));
    setParam (params::density, pick ({ { 100.0f, 0.8f }, { 80.0f, 0.1f }, { 125.0f, 0.1f } }));
    setParam (params::pitchRamp, pick ({ { 0.0f, 0.85f }, { 5.0f, 0.05f }, { 7.0f, 0.05f }, { -5.0f, 0.05f } }));
    rebuildPattern();
    commitUndoStep();

    // remember it in the KILL history (newest last; going back and killing again drops the "future")
    if (killHistoryPos >= 0 && killHistoryPos + 1 < (int) killHistory.size())
        killHistory.erase (killHistory.begin() + killHistoryPos + 1, killHistory.end());
    const auto& now = model.getPreset();
    killHistory.push_back ({ apvts.copyState(), juce::String (getCategoryProfile (now.category).name) + " - " + now.name
                                                    + "  #" + juce::String ((int) getSeed()) });
    while ((int) killHistory.size() > kMaxKillHistory)
        killHistory.erase (killHistory.begin());
    killHistoryPos = (int) killHistory.size() - 1;
}

void RollsKillaProcessor::restoreKillHistory (int index)
{
    if (index < 0 || index >= (int) killHistory.size())
        return;

    // keep the user's tempo / mood / UI choices - only the roll comes back
    const auto bpm = getTargetBpm();
    const auto mood = getMood();
    apvts.replaceState (killHistory[(size_t) index].state.createCopy());
    apvts.state.setProperty ("targetBpm", bpm, nullptr);
    apvts.state.setProperty ("mood", mood, nullptr);
    killHistoryPos = index;
    applyRestoredState();
    commitUndoStep();
}

double RollsKillaProcessor::getTargetBpm() const
{
    return (double) apvts.state.getProperty ("targetBpm", 0.0);
}

void RollsKillaProcessor::setTargetBpm (double bpm)
{
    apvts.state.setProperty ("targetBpm", bpm <= 0.0 ? 0.0 : juce::jlimit (40.0, 240.0, bpm), nullptr);
    previewBpm.store (getEffectiveBpm());
}

double RollsKillaProcessor::getEffectiveBpm() const
{
    const auto manual = getTargetBpm();
    if (manual > 0.0)
        return manual;
    const auto host = hostBpm.load();
    return host > 0.0 ? host : model.getPreset().bpmHint;
}

int RollsKillaProcessor::getMood() const
{
    return juce::jlimit (0, 2, (int) apvts.state.getProperty ("mood", 1));
}

void RollsKillaProcessor::setMood (int mood)
{
    apvts.state.setProperty ("mood", juce::jlimit (0, 2, mood), nullptr);
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
        e.deleted = child.getProperty ("deleted", false);
        e.removeRoll = child.getProperty ("removeRoll", false);
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
        existing.setProperty ("deleted", edit.deleted, nullptr);
        existing.setProperty ("removeRoll", edit.removeRoll, nullptr);
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
    o.bpm = getEffectiveBpm();
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
    smoke.prepare (sampleRate);
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
            if (const auto bpm = pos->getBpm())
                hostBpm.store (*bpm, std::memory_order_relaxed);
        }
    }

    hostPlaying.store (hostIsPlaying, std::memory_order_relaxed);
    const auto* pattern = patternSlot.acquire();

    if (! hostIsPlaying && previewEnabled.load (std::memory_order_relaxed))
    {
        if (previewRestart.exchange (false))
            previewPpq = 0.0;

        transport.playing = true;
        const auto wanted = previewBpm.load (std::memory_order_relaxed);
        transport.bpm = wanted > 0.0 ? wanted : (pattern != nullptr ? pattern->bpmHint : 140.0);
        transport.ppq = previewPpq;
        previewPpq += numSamples * transport.bpm / 60.0 / currentSampleRate;
    }

    // ---- MIDI gate: in MIDI mode the pattern only plays while a note is held on the channel
    //      (FL: a long note in the piano roll; a muted channel sends no notes -> silence)
    const auto midiGate = raw.playMode->load() < 0.5f;
    const auto previewing = ! hostIsPlaying && transport.playing;

    if (! hostIsPlaying && numHeldNotes > 0)
    {
        heldNotes.fill (false);
        numHeldNotes = 0;
    }

    auto updateGate = [this] (const juce::MidiMessage& m)
    {
        if (m.isNoteOn())
        {
            if (! heldNotes[(size_t) m.getNoteNumber()]) { heldNotes[(size_t) m.getNoteNumber()] = true; ++numHeldNotes; }
        }
        else if (m.isNoteOff())
        {
            if (heldNotes[(size_t) m.getNoteNumber()]) { heldNotes[(size_t) m.getNoteNumber()] = false; --numHeldNotes; }
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            heldNotes.fill (false);
            numHeldNotes = 0;
        }
    };

    int numEvents = 0;

    if (midiGate && hostIsPlaying)
    {
        // run the player in segments split at gate changes (sample accurate, no allocation)
        const auto beatsPerSample = transport.bpm / 60.0 / currentSampleRate;
        int segStart = 0;

        auto runSegment = [&] (int segEnd)
        {
            segEnd = juce::jlimit (0, numSamples, segEnd);
            if (segEnd <= segStart)
                return;

            auto t = transport;
            t.ppq = transport.ppq + segStart * beatsPerSample;
            t.playing = numHeldNotes > 0;
            const auto n = player.process (pattern, t, segEnd - segStart, segmentEvents);

            for (int i = 0; i < n && numEvents < (int) events.size(); ++i)
            {
                auto e = segmentEvents[(size_t) i];
                e.sampleOffset += segStart;
                events[(size_t) numEvents++] = e;
            }
            segStart = segEnd;
        };

        for (const auto meta : midi)
        {
            const auto m = meta.getMessage();
            if (m.isNoteOnOrOff() || m.isAllNotesOff() || m.isAllSoundOff())
            {
                runSegment (meta.samplePosition);
                updateGate (m);
            }
        }
        runSegment (numSamples);
    }
    else
    {
        for (const auto meta : midi)
            updateGate (meta.getMessage());
        numEvents = player.process (pattern, transport, numSamples, events);
    }

    waitingForMidi.store (midiGate && hostIsPlaying && numHeldNotes == 0, std::memory_order_relaxed);
    patternSlot.release();

    // ---- live input (Host mode only): incoming note-ons play the sampler directly.
    //      In MIDI mode the notes are the gate, not sounds.
    int numInput = 0;
    if (! midiGate || previewing)
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

    // ---- PUFF (blunt smoke FX), synced to the beat; audio only, the MIDI out stays clean
    const auto sounding = transport.playing && ! (midiGate && hostIsPlaying && numHeldNotes == 0);
    smoke.process (buffer.getWritePointer (0), buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr, numSamples,
                   raw.puff->load() / 100.0f, transport.ppq, transport.bpm / 60.0 / currentSampleRate, sounding);
}

//==============================================================================
juce::AudioProcessorEditor* RollsKillaProcessor::createEditor()
{
   #if ROLLSKILLA_MINI
    return new RollsKillaMiniEditor (*this);
   #else
    return new RollsKillaEditor (*this);
   #endif
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
