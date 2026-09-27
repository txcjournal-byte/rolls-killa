#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace rk;

RollsKillaProcessor::RollsKillaProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "RollsKilla", params::createLayout (sampler.getSampleNames()))
{
    library.setUserDirectory (RollLibrary::defaultUserDirectory());

    for (const auto& id : params::patternParameterIds())
        apvts.addParameterListener (id, this);

    raw.hat = apvts.getRawParameterValue (params::hat);
    raw.tune = apvts.getRawParameterValue (params::tune);
    raw.decay = apvts.getRawParameterValue (params::decay);
    raw.choke = apvts.getRawParameterValue (params::choke);
    raw.volume = apvts.getRawParameterValue (params::volume);

    rebuildPattern (true);
    startTimerHz (30);
}

RollsKillaProcessor::~RollsKillaProcessor()
{
    stopTimer();
    for (const auto& id : params::patternParameterIds())
        apvts.removeParameterListener (id, this);
}

//==============================================================================
ModelSettings RollsKillaProcessor::readModelSettings() const
{
    ModelSettings s;
    s.presetIndex = (int) apvts.getRawParameterValue (params::preset)->load();
    s.bars = params::barsFromChoice ((int) apvts.getRawParameterValue (params::bars)->load());
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
    // May be called on the audio thread (automation): only flag, the timer rebuilds.
    patternDirty.store (true);
}

void RollsKillaProcessor::timerCallback()
{
    if (patternDirty.load())
        rebuildPattern();

    patternSlot.collectGarbage();
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
    state.setProperty ("presetId", model.getPreset().id, nullptr);
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
    const auto idx = library.indexOfId (state.getProperty ("presetId").toString());
    if (idx >= 0)
        if (auto* p = apvts.getParameter (params::preset))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) idx));

    rebuildPattern (true);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RollsKillaProcessor();
}
