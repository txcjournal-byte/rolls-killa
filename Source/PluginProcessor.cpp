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

    writeKitToState();
    refreshKitSounds();
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
    s.volumeDb = raw.volume->load() + hatDrawerDb.load (std::memory_order_relaxed);
    return s;
}

void RollsKillaProcessor::rebuildPattern (bool force)
{
    patternDirty.store (false);

    if (model.update (readModelSettings(), force) || force)
    {
        auto pb = model.makePlayback (kRootNote);
        mergeKit (*pb);
        beatLength.store (pb->lengthBeats);
        patternSlot.publish (std::move (pb));
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

    syncKitFromState();
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
    kitSampler.prepare (sampleRate);
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
    bypassFaded = false;

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
    kitSampler.beginBlock();

    if (const auto a = pendingAudition.exchange (0); a > 0)
    {
        if (a - 1 == (int) DrumType::hat)
            sampler.noteOn (kRootNote, 112);
        else
            kitSampler.audition (a - 1, 112);
    }

    int rendered = 0;
    int nextInput = 0;

    auto renderUpTo = [&] (int offset)
    {
        offset = juce::jlimit (0, numSamples, offset);
        if (offset > rendered)
        {
            sampler.render (buffer, rendered, offset - rendered);
            kitSampler.render (buffer, rendered, offset - rendered);
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

        // slot 0 = the hi-hat rolls (MIDI channel 1), 1..8 = kit drawers (channel 2..9)
        const auto channel = e.slot + 1;
        if (e.vel > 0)
        {
            if (e.slot == 0)
            {
                sampler.noteOn (e.note, e.vel);
                kitSampler.choke ((int) DrumType::openHat);   // a closed hat cuts the open hat
                hitCounters[(size_t) DrumType::hat].fetch_add (1, std::memory_order_relaxed);
            }
            else
            {
                kitSampler.noteOn (e.slot - 1, e.note, e.vel);
                hitCounters[(size_t) juce::jlimit (0, kNumDrumTypes - 1, e.slot - 1)].fetch_add (1, std::memory_order_relaxed);
            }
            midi.addEvent (juce::MidiMessage::noteOn (channel, e.note, (juce::uint8) e.vel), e.sampleOffset);
        }
        else
        {
            midi.addEvent (juce::MidiMessage::noteOff (channel, e.note), e.sampleOffset);
        }
    }

    playInputUpTo (numSamples);
    renderUpTo (numSamples);
    sampler.endBlock();
    kitSampler.endBlock();

    // ---- PUFF (blunt smoke FX), synced to the beat; audio only, the MIDI out stays clean
    const auto sounding = transport.playing && ! (midiGate && hostIsPlaying && numHeldNotes == 0);
    smoke.process (buffer.getWritePointer (0), buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr, numSamples,
                   raw.puff->load() / 100.0f, transport.ppq, transport.bpm / 60.0 / currentSampleRate, sounding);
}

void RollsKillaProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    buffer.clear();
    midi.clear();

    // stopped transport -> the player releases every pending note (MIDI out must not hang)
    const auto* pattern = patternSlot.acquire();
    const auto numEvents = player.process (pattern, TransportState {}, numSamples, events);
    patternSlot.release();
    for (int i = 0; i < numEvents; ++i)
        if (events[(size_t) i].vel == 0)
            midi.addEvent (juce::MidiMessage::noteOff (events[(size_t) i].slot + 1, events[(size_t) i].note), events[(size_t) i].sampleOffset);

    if (! bypassFaded)
    {
        // the ringing hats fade out over a couple of ms instead of being cut (a cut = a click)
        sampler.beginBlock (readSamplerSettings());
        sampler.allNotesOff();
        sampler.render (buffer, 0, numSamples);
        sampler.endBlock();
        kitSampler.beginBlock();
        kitSampler.allNotesOff();
        kitSampler.render (buffer, 0, numSamples);
        kitSampler.endBlock();
        smoke.reset();
        heldNotes.fill (false);
        numHeldNotes = 0;
        hostPlaying.store (false, std::memory_order_relaxed);
        bypassFaded = true;
    }
}

//==============================================================================
// DRUM KIT
void RollsKillaProcessor::writeKitToState()
{
    auto old = apvts.state.getChildWithName (DrumKit::kTreeType);
    if (old.isValid())
        apvts.state.removeChild (old, nullptr);
    apvts.state.appendChild (kit.toValueTree(), nullptr);
}

void RollsKillaProcessor::syncKitFromState()
{
    const auto tree = apvts.state.getChildWithName (DrumKit::kTreeType);
    if (tree.isValid())
        kit.fromValueTree (tree);
    else
        writeKitToState();   // a project from before the kit: keep the defaults
    refreshKitSounds();
}

namespace
{
    bool sameSound (const KitSlot& a, const KitSlot& b)
    {
        return a.seed == b.seed && a.mood == b.mood && a.file == b.file
            && juce::approximatelyEqual (a.shape.tune, b.shape.tune) && juce::approximatelyEqual (a.shape.decay, b.shape.decay)
            && juce::approximatelyEqual (a.shape.punch, b.shape.punch) && juce::approximatelyEqual (a.shape.drive, b.shape.drive)
            && juce::approximatelyEqual (a.shape.tone, b.shape.tone) && juce::approximatelyEqual (a.shape.body, b.shape.body);
    }
}

void RollsKillaProcessor::refreshKitSounds()
{
    // sounds are rendered at 44.1 kHz once; the samplers resample to the host rate
    for (int t = 0; t < kNumDrumTypes; ++t)
    {
        const auto type = drumTypeFromIndex (t);
        const auto& s = kit.slot (type);
        kitSampler.setGain (type, s.volumeDb, s.muted);
       #if ! ROLLSKILLA_MINI
        if (type == DrumType::hat)
            hatDrawerDb.store (s.muted ? -100.0f : s.volumeDb);
       #endif
        if (slotRendered[(size_t) t] && sameSound (renderedSlots[(size_t) t], s))
            continue;
        juce::String error;
        auto snd = kit.renderSlot (type, kKitExportRate, error);
        renderedSlots[(size_t) t] = s;
        slotRendered[(size_t) t] = true;
        if (type == DrumType::hat)
        {
            hatKitSound = snd;
           #if ! ROLLSKILLA_MINI
            // Rolls Killa: the hi-hat drawer is what the rolls play (the rolls sampler's custom slot)
            auto hs = std::make_shared<HatSample>();
            hs->name = snd->name;
            hs->audio = snd->audio;
            hs->sampleRate = snd->sampleRate;
            hs->synthesized = ! snd->fromFile();
            sampler.setCustomSample (hs);
            if ((int) raw.hat->load() != HatSampler::kCustomSlot)
                if (auto* p = apvts.getParameter (params::hat))
                    p->setValueNotifyingHost (p->convertTo0to1 ((float) HatSampler::kCustomSlot));
           #endif
            continue;
        }
        kitSampler.setSound (type, snd);
    }
}

void RollsKillaProcessor::mergeKit (PlaybackPattern& pb) const
{
    double len = pb.lengthBeats;
    bool any = false;
    for (int t = 0; t < kNumDrumTypes; ++t)
    {
        const auto& s = kit.slots[(size_t) t];
        if (s.patternOn && ! s.muted && (DrumType) t != DrumType::hat)
        {
            len = juce::jmax (len, s.patternBars * 4.0);
            any = true;
        }
    }
    if (! any)
        return;

    // the shorter patterns repeat inside the longest one
    const auto hatLen = juce::jmax (1.0, pb.lengthBeats);
    const auto hatEvents = pb.events;
    for (double offset = hatLen; offset < len - 1.0e-6; offset += hatLen)
        for (auto e : hatEvents)
        {
            e.beat += offset;
            pb.events.push_back (e);
        }

    for (int t = 0; t < kNumDrumTypes; ++t)
    {
        const auto type = drumTypeFromIndex (t);
        const auto& s = kit.slot (type);
        if (! s.patternOn || s.muted || type == DrumType::hat)
            continue;
        const auto snd = kitSampler.getSound (type);
        const auto root = snd != nullptr ? snd->rootNote : 60;
        const auto slotLen = s.patternBars * 4.0;
        const auto hits = kit.pattern (type);
        for (double offset = 0.0; offset < len - 1.0e-6; offset += slotLen)
            for (const auto& h : hits)
                pb.events.push_back ({ h.beat + offset, h.len, juce::jlimit (0, 127, root + h.semi),
                                       juce::jlimit (1, 127, (int) std::lround (h.vel * 127.0f)), t + 1 });
    }
    pb.lengthBeats = len;
    std::stable_sort (pb.events.begin(), pb.events.end(), [] (const PlaybackPattern::Event& a, const PlaybackPattern::Event& b) { return a.beat < b.beat; });
}

void RollsKillaProcessor::updateKitSlot (DrumType type, const std::function<void (KitSlot&)>& change, bool commit)
{
    change (kit.slot (type));
    writeKitToState();
    refreshKitSounds();
    rebuildPattern (true);
    if (commit)
        commitUndoStep();
    else
        markStateChanged();
}

void RollsKillaProcessor::setKitName (const juce::String& name)
{
    kit.name = name.trim().isEmpty() ? juce::String ("ROLLS KILLA KIT") : name.trim().substring (0, 48);
    writeKitToState();
    commitUndoStep();
}

void RollsKillaProcessor::killSound (DrumType type)
{
    const auto mood = getMood();
    updateKitSlot (type, [mood] (KitSlot& s)
    {
        s.seed = (juce::uint32) juce::Random::getSystemRandom().nextInt (1 << 30) + 1;
        s.mood = mood;
        s.file = {};
    }, true);
}

void RollsKillaProcessor::killPattern (DrumType type)
{
    if (type == DrumType::hat)
    {
        kill();
        return;
    }
    updateKitSlot (type, [] (KitSlot& s)
    {
        s.patternSeed = (juce::uint32) juce::Random::getSystemRandom().nextInt (1 << 30) + 1;
        s.patternOn = true;
        s.muted = false;
    }, true);
}

void RollsKillaProcessor::killKit()
{
    auto& rng = juce::Random::getSystemRandom();
    for (auto& s : kit.slots)
    {
        s.seed = (juce::uint32) rng.nextInt (1 << 30) + 1;
        s.mood = getMood();
        s.file = {};
    }
    writeKitToState();
    refreshKitSounds();
    rebuildPattern (true);
    commitUndoStep();
}

void RollsKillaProcessor::killBeat()
{
    auto& rng = juce::Random::getSystemRandom();
    bool anyOn = false;
    for (int t = 0; t < kNumDrumTypes; ++t)
        if (drumTypeFromIndex (t) != DrumType::hat && kit.slots[(size_t) t].patternOn)
            anyOn = true;
    for (int t = 0; t < kNumDrumTypes; ++t)
    {
        auto& s = kit.slots[(size_t) t];
        const auto type = drumTypeFromIndex (t);
        if (type == DrumType::hat)
            continue;
        // a first KILL BEAT starts a full trap beat: 808, kick, clap (+ the hi-hat rolls)
        if (! anyOn && (type == DrumType::b808 || type == DrumType::kick || type == DrumType::clap))
            s.patternOn = true;
        if (s.patternOn)
            s.patternSeed = (juce::uint32) rng.nextInt (1 << 30) + 1;
    }
    writeKitToState();
    killEverything();   // new hi-hat roll + rebuild + one undo step (includes the kit)
}

juce::String RollsKillaProcessor::loadSlotFile (DrumType type, const juce::File& file)
{
    juce::String error;
    if (loadDrumFile (type, file, {}, kKitExportRate, error) == nullptr)
        return error;
    updateKitSlot (type, [&file] (KitSlot& s) { s.file = file.getFullPathName(); }, true);
    return {};
}

void RollsKillaProcessor::clearSlotFile (DrumType type)
{
    updateKitSlot (type, [] (KitSlot& s) { s.file = {}; }, true);
}

bool RollsKillaProcessor::keepSound (DrumType type)
{
    const auto snd = getSlotSound (type);
    if (! kit.keep (type, snd != nullptr ? snd->name : juce::String (drumTypeName (type))))
        return false;
    writeKitToState();
    commitUndoStep();
    return true;
}

void RollsKillaProcessor::removeKept (int index)
{
    if (index < 0 || index >= (int) kit.kept.size())
        return;
    kit.kept.erase (kit.kept.begin() + index);
    writeKitToState();
    commitUndoStep();
}

void RollsKillaProcessor::auditionSlot (DrumType type) noexcept
{
    pendingAudition.store ((int) type + 1);
}

DrumSoundPtr RollsKillaProcessor::getSlotSound (DrumType type) const
{
    return type == DrumType::hat ? hatKitSound : kitSampler.getSound (type);
}

std::vector<KitMidi> RollsKillaProcessor::kitMidis()
{
    std::vector<KitMidi> out;
    const auto bpm = getEffectiveBpm();
    out.push_back ({ "Hi-Hat Rolls", createMidiFile (model.getPattern(), getMidiExportOptions()) });
    for (int t = 0; t < kNumDrumTypes; ++t)
    {
        const auto type = drumTypeFromIndex (t);
        const auto& s = kit.slot (type);
        if (type == DrumType::hat || ! s.patternOn)
            continue;
        const auto snd = getSlotSound (type);
        out.push_back ({ juce::String (drumTypeName (type)) + " Pattern",
                         drumHitsToMidi (kit.pattern (type), snd != nullptr ? snd->rootNote : 60, bpm, drumTypeName (type)) });
    }
    return out;
}

juce::File RollsKillaProcessor::createSlotMidiFile (DrumType type)
{
    if (type == DrumType::hat)
        return createDragMidiFile();
    const auto snd = getSlotSound (type);
    const auto mf = drumHitsToMidi (kit.pattern (type), snd != nullptr ? snd->rootNote : 60, getEffectiveBpm(), drumTypeName (type));
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Rolls Killa MIDI");
    dir.createDirectory();
    auto f = dir.getChildFile (safeFileName ("Rolls Killa - " + juce::String (drumTypeName (type)) + " " + juce::String (juce::roundToInt (getEffectiveBpm())) + " BPM") + ".mid");
    f.deleteFile();
    if (juce::FileOutputStream os (f); os.openedOk())
        mf.writeTo (os, 1);
    return f;
}

juce::File RollsKillaProcessor::createBeatMidiFile()
{
    // one type-1 file: tempo track + one track per playing drawer (FL makes a channel per track)
    juce::MidiFile all;
    all.setTicksPerQuarterNote (kTicksPerBeat);
    for (const auto& m : kitMidis())
        for (int i = 0; i < m.midi.getNumTracks(); ++i)
            all.addTrack (*m.midi.getTrack (i));
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Rolls Killa MIDI");
    dir.createDirectory();
    auto f = dir.getChildFile ("Rolls Killa - Beat " + juce::String (juce::roundToInt (getEffectiveBpm())) + " BPM.mid");
    f.deleteFile();
    if (juce::FileOutputStream os (f); os.openedOk())
        all.writeTo (os, 1);
    return f;
}

KitExportResult RollsKillaProcessor::exportKitTo (const juce::File& parentDir)
{
    return exportKit (kit, parentDir, kitMidis());
}

KitExportResult RollsKillaProcessor::exportOneShotsTo (const juce::File& parentDir, DrumType type, int count)
{
    const auto& s = kit.slot (type);
    const auto name = kit.name + " - " + juce::String (count) + " " + drumTypeFolder (type);
    return exportOneShotKit (type, count, getMood(), s.shape, (juce::uint32) juce::Random::getSystemRandom().nextInt (1 << 30) + 1, name, parentDir);
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
