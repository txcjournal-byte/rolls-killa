#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout HatGoblinProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto f = [&] (const char* id, const char* name, float lo, float hi, float def, float skewCentre = -1.0f, const char* unit = "")
    {
        juce::NormalisableRange<float> r (lo, hi);
        if (skewCentre > 0) r.setSkewForCentre (skewCentre);
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, r, def,
                        juce::AudioParameterFloatAttributes().withLabel (unit)));
    };
    auto b = [&] (const char* id, const char* name, bool def)
    {
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def));
    };
    auto c = [&] (const char* id, const char* name, juce::StringArray items, int def)
    {
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def));
    };

    p.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "hat", 1 }, "Hat", 0, 119, 0));
    b ("useUser", "Use Your WAV", false);
    f ("volume", "Volume", -24.0f, 6.0f, 0.0f, -1, "dB");
    f ("tune", "Tune", -12.0f, 12.0f, 0.0f, -1, "st");
    f ("decay", "Decay", 0.05f, 1.0f, 1.0f);
    f ("swing", "Swing", 0.0f, 0.75f, 0.0f);
    f ("humanize", "Humanize", 0.0f, 1.0f, 0.15f);
    c ("velShape", "Velocity Shape", { "Flat", "Ramp Up", "Ramp Down", "Accent" }, 1);
    f ("velDepth", "Velocity Depth", 0.0f, 1.0f, 0.8f);
    f ("velRand", "Velocity Random", 0.0f, 1.0f, 0.1f);
    f ("prob", "Probability", 0.0f, 1.0f, 1.0f);
    f ("pitchRamp", "Pitch Ramp", -12.0f, 12.0f, 0.0f, -1, "st");
    c ("panMode", "Pan Mode", { "Center", "Ping-Pong", "Sweep" }, 0);
    f ("panWidth", "Pan Width", 0.0f, 1.0f, 0.3f);
    c ("speed", "Speed", { "1/2x", "1x", "2x" }, 1);
    b ("mono", "Mono", true);
    b ("latch", "Latch", false);
    b ("choke", "Choke", true);
    f ("drive", "Saturate", 0.0f, 1.0f, 0.15f);
    f ("crush", "Crush", 0.0f, 1.0f, 0.0f);
    f ("hpf", "High Pass", 20.0f, 2000.0f, 250.0f, 300.0f, "Hz");
    f ("lpf", "Low Pass", 2000.0f, 20000.0f, 20000.0f, 8000.0f, "Hz");
    f ("reverb", "Reverb", 0.0f, 1.0f, 0.06f);
    f ("out", "Output", -24.0f, 6.0f, -3.0f, -1, "dB");
    b ("groove", "Groove Plays", true);
    c ("bars", "Length", { "1 Bar", "2 Bars", "4 Bars" }, 0);
    {
        juce::StringArray styles;
        for (int i = 0; i < GrooveGen::numStyles; ++i) styles.add (GrooveGen::styleName (i));
        c ("style", "Style", styles, 0);
    }
    f ("complexity", "Complexity", 0.0f, 1.0f, 0.5f);
    c ("midiTarget", "Drag MIDI For", { "Any sampler (root C5 in FL)", "Hat Goblin channel" }, 0);
    c ("noteNames", "Note Names", { "FL Studio (C5 = 60)", "Ableton / Logic (C3 = 60)" }, 0);
    return { p.begin(), p.end() };
}

HatGoblinProcessor::HatGoblinProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "HATGOBLIN", createLayout())
{
    HatLibrary::get();   // load the factory hats now, not on the audio thread
    grooveName = GrooveGen::presetName (6);
    presetIndex = 6;
    pushHistory();
}

bool HatGoblinProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void HatGoblinProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    engine.prepare (sampleRate, samplesPerBlock);
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) juce::jmax (1, samplesPerBlock), 2 };
    hpf.prepare (spec); hpf.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    lpf.prepare (spec); lpf.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    reverb.setSampleRate (sampleRate);
    reverb.reset();
    keyboardState.reset();
}

const HatSample* HatGoblinProcessor::currentSample() const
{
    if (apvts.getRawParameterValue ("useUser")->load() > 0.5f)
        if (auto* u = userSample.load())
            return u;
    return &HatLibrary::get().hat ((int) apvts.getRawParameterValue ("hat")->load());
}

RollParams HatGoblinProcessor::currentParams() const
{
    auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    RollParams p;
    p.swing = v ("swing"); p.humanize = v ("humanize"); p.velDepth = v ("velDepth"); p.velRand = v ("velRand");
    p.prob = v ("prob"); p.pitchRamp = v ("pitchRamp"); p.panWidth = v ("panWidth"); p.tune = v ("tune");
    p.decay = v ("decay"); p.gain = juce::Decibels::decibelsToGain (v ("volume"));
    p.velShape = (int) v ("velShape"); p.panMode = (int) v ("panMode");
    const int sp = (int) v ("speed");
    p.speed = sp == 0 ? 0.5 : (sp == 2 ? 2.0 : 1.0);
    p.mono = v ("mono") > 0.5f; p.latch = v ("latch") > 0.5f; p.choke = v ("choke") > 0.5f;
    p.groove = v ("groove") > 0.5f;
    const int bars = (int) v ("bars");
    p.bars = bars == 2 ? 4 : (bars == 1 ? 2 : 1);
    return p;
}

void HatGoblinProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    for (const auto m : midi)
    {
        const auto msg = m.getMessage();
        if (msg.isNoteOn() && (KeyMap::isRate (msg.getNoteNumber()) || KeyMap::isPattern (msg.getNoteNumber())))
            lastKey.store (msg.getNoteNumber());
    }

    TransportInfo t;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm()) t.bpm = juce::jlimit (20.0, 400.0, *bpm);
            t.playing = pos->getIsPlaying();
            if (auto ppq = pos->getPpqPosition()) t.ppqAtBlockStart = *ppq;
            else t.playing = false;
        }
    currentBpm.store (t.bpm);
    hostPlaying.store (t.playing);

    engine.setSample (currentSample());
    engine.process (buffer, midi, currentParams(), t);
    applyFx (buffer);
    midi.clear();
}

void HatGoblinProcessor::applyFx (juce::AudioBuffer<float>& buffer)
{
    auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const int n = buffer.getNumSamples();
    const int chans = juce::jmin (2, buffer.getNumChannels());

    // saturation
    const float drive = v ("drive");
    if (drive > 0.001f)
    {
        const float k = 1.0f + drive * 8.0f, norm = 1.0f / std::tanh (k);
        for (int ch = 0; ch < chans; ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            for (int i = 0; i < n; ++i) d[i] = std::tanh (d[i] * k) * norm;
        }
    }

    // bit / rate crush
    const float crush = v ("crush");
    if (crush > 0.001f)
    {
        const float levels = std::pow (2.0f, 16.0f - crush * 12.0f);
        const int hold = 1 + (int) (crush * 7.0f);
        for (int i = 0; i < n; ++i)
        {
            if (crushCounter <= 0)
            {
                for (int ch = 0; ch < chans; ++ch)
                    crushHold[ch] = std::round (buffer.getSample (ch, i) * levels) / levels;
                crushCounter = hold;
            }
            --crushCounter;
            for (int ch = 0; ch < chans; ++ch) buffer.setSample (ch, i, crushHold[ch]);
        }
    }

    // filters
    hpf.setCutoffFrequency (v ("hpf"));
    lpf.setCutoffFrequency (juce::jmin (v ("lpf"), (float) (sr * 0.45)));
    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) chans, (size_t) n);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    hpf.process (ctx);
    if (v ("lpf") < 19900.0f) lpf.process (ctx);

    // short room
    const float mix = v ("reverb");
    if (mix > 0.001f)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.3f; rp.damping = 0.6f; rp.width = 1.0f;
        rp.wetLevel = mix * 0.6f; rp.dryLevel = 1.0f;
        reverb.setParameters (rp);
        if (chans == 2) reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), n);
        else reverb.processMono (buffer.getWritePointer (0), n);
    }

    buffer.applyGain (juce::Decibels::decibelsToGain (v ("out")));
    // soft limiter: transparent below -2 dBFS, never above 0 dBFS; never pass NaN to the host
    constexpr float knee = 0.8f;
    for (int ch = 0; ch < chans; ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
        {
            float x = std::isfinite (d[i]) ? d[i] : 0.0f;
            const float a = std::abs (x);
            if (a > knee)
                x = std::copysign (knee + (1.0f - knee) * std::tanh ((a - knee) / (1.0f - knee)), x);
            d[i] = x;
        }
    }
}

bool HatGoblinProcessor::loadUserSample (const juce::File& file)
{
    auto s = std::make_unique<HatSample>();
    if (! HatLibrary::loadFile (file, *s))
        return false;
    userSample.store (s.get());
    userSamples.push_back (std::move (s));
    userPath = file.getFullPathName();
    if (auto* p = apvts.getParameter ("useUser")) p->setValueNotifyingHost (1.0f);
    return true;
}

void HatGoblinProcessor::clearUserSample()
{
    if (auto* p = apvts.getParameter ("useUser")) p->setValueNotifyingHost (0.0f);
}

juce::String HatGoblinProcessor::userSampleName() const
{
    if (auto* u = userSample.load()) return u->name;
    return {};
}

void HatGoblinProcessor::pushHistory()
{
    if (historyPos + 1 < (int) history.size()) history.erase (history.begin() + historyPos + 1, history.end());
    history.push_back ({ engine.getGroove(), engine.grooveSeed.load(), grooveName, presetIndex });
    if (history.size() > 100) history.erase (history.begin());
    historyPos = (int) history.size() - 1;
}

void HatGoblinProcessor::restore (const Snapshot& s)
{
    engine.setGroove (s.groove);
    engine.grooveSeed.store (s.seed);
    grooveName = s.name;
    presetIndex = s.preset;
}

void HatGoblinProcessor::generate()
{
    juce::Random rng;
    auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    int used = 0;
    const auto p = currentParams();
    auto g = GrooveGen::generate ((int) v ("style"), v ("complexity"), p.bars, rng, &used);
    engine.setGroove (g);
    engine.grooveSeed.store ((juce::uint32) rng.nextInt());
    grooveName = "GEN " + juce::String (++generatedCount).paddedLeft ('0', 2) + " - " + GrooveGen::styleName (used);
    presetIndex = -1;
    pushHistory();
}

void HatGoblinProcessor::loadPreset (int index)
{
    index = juce::jlimit (0, GrooveGen::numPresets() - 1, index);
    engine.setGroove (GrooveGen::preset (index, currentParams().bars));
    engine.grooveSeed.store ((juce::uint32) (index * 7919 + 13));
    grooveName = GrooveGen::presetName (index);
    presetIndex = index;
    pushHistory();
}

void HatGoblinProcessor::goBack()    { if (canGoBack())    restore (history[(size_t) --historyPos]); }
void HatGoblinProcessor::goForward() { if (canGoForward()) restore (history[(size_t) ++historyPos]); }

void HatGoblinProcessor::vary()
{
    engine.grooveSeed.store ((juce::uint32) juce::Random().nextInt());
    pushHistory();
}

void HatGoblinProcessor::clearGroove()
{
    GrooveGen::Groove g;
    for (int b = 0; b < GrooveGen::maxBeats; ++b) g.hits[(size_t) b] = { 1, 0, 1, 0, 0, 0 };
    engine.setGroove (g);
    grooveName = "EMPTY 1/8";
    presetIndex = -1;
    pushHistory();
}

void HatGoblinProcessor::grooveEdited()
{
    if (! grooveName.endsWith ("*")) grooveName << " *";
    presetIndex = -1;
    pushHistory();
}

juce::String HatGoblinProcessor::exportName (int key) const
{
    juce::String what;
    if (key < 0) what = grooveName.removeCharacters ("*").trim();
    else if (KeyMap::isPattern (key)) what = "P" + juce::String (key - KeyMap::firstPattern + 1) + " " + engine.patternName (key - KeyMap::firstPattern);
    else what = KeyMap::rateLabel (key).replace ("/", "-");
    return "HAT GOBLIN " + what + " " + juce::String (juce::roundToInt (currentBpm.load())) + "bpm";
}

static juce::File exportFile (const juce::String& name)
{
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("HAT GOBLIN exports");
    dir.createDirectory();
    auto f = dir.getChildFile (juce::File::createLegalFileName (name));
    f.deleteFile();
    return f;
}

juce::File HatGoblinProcessor::writeMidi (int key)
{
    auto file = exportFile (exportName (key) + ".mid");
    auto midi = key < 0 ? engine.renderGrooveMidi (currentParams(), midiRoot())
                        : engine.renderMidi (key, currentParams(), midiRoot() == 60 ? 60 : KeyMap::hitRoot);
    if (auto out = file.createOutputStream())
    {
        midi.writeTo (*out);
        out->flush();
    }
    return file;
}

namespace
{
    struct FixedPlayHead : juce::AudioPlayHead
    {
        double bpm = 140.0, ppq = 0.0;
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo i;
            i.setBpm (bpm);
            i.setPpqPosition (ppq);
            i.setIsPlaying (true);
            i.setTimeSignature (TimeSignature { 4, 4 });
            return i;
        }
    };

    bool writeWav (const juce::File& file, const juce::AudioBuffer<float>& audio, double rate)
    {
        std::unique_ptr<juce::OutputStream> out (file.createOutputStream());
        if (out == nullptr) return false;
        juce::WavAudioFormat wav;
        auto writer = wav.createWriterFor (out, juce::AudioFormatWriterOptions()
                                                    .withSampleRate (rate)
                                                    .withNumChannels (audio.getNumChannels())
                                                    .withBitsPerSample (24));
        if (writer == nullptr) return false;
        return writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
    }
}

juce::File HatGoblinProcessor::writeAudio (int key)
{
    // a second Hat Goblin with the same settings renders offline, exactly as it plays here
    HatGoblinProcessor tmp;
    juce::MemoryBlock state;
    getStateInformation (state);
    tmp.setStateInformation (state.getData(), (int) state.getSize());
    if (key >= 0)
        if (auto* g = tmp.apvts.getParameter ("groove")) g->setValueNotifyingHost (0.0f);

    const double rate = getSampleRate() > 0 ? getSampleRate() : 44100.0;
    const double bpm = currentBpm.load();
    const int block = 512;
    FixedPlayHead head;
    head.bpm = bpm;
    tmp.setPlayHead (&head);
    tmp.setPlayConfigDetails (0, 2, rate, block);
    tmp.prepareToPlay (rate, block);

    const auto p = currentParams();
    double beats = 4.0;
    if (key < 0) beats = RollEngine::grooveLength (p);
    else if (KeyMap::isPattern (key) && p.speed < 0.75) beats = 8.0;
    const int total = (int) std::round (beats * 60.0 / bpm * rate);
    juce::AudioBuffer<float> outAudio (2, total), buf (2, block);
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = juce::jmin (block, total - pos);
        head.ppq = pos / rate * bpm / 60.0;
        juce::MidiBuffer midi;
        if (pos == 0 && key >= 0) midi.addEvent (juce::MidiMessage::noteOn (1, key, (juce::uint8) 115), 0);
        buf.setSize (2, n, false, false, true);
        tmp.processBlock (buf, midi);
        for (int ch = 0; ch < 2; ++ch) outAudio.copyFrom (ch, pos, buf, ch, 0, n);
    }
    tmp.setPlayHead (nullptr);
    tmp.releaseResources();

    auto file = exportFile (exportName (key) + ".wav");
    writeWav (file, outAudio, rate);
    return file;
}

juce::File HatGoblinProcessor::writeHatWav()
{
    auto* s = currentSample();
    if (s == nullptr) return {};
    auto file = exportFile ("HAT GOBLIN - " + s->name + ".wav");
    writeWav (file, s->audio, s->sampleRate);
    return file;
}

void HatGoblinProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("userPath", userPath, nullptr);
    for (int i = 0; i < 12; ++i)
    {
        state.setProperty ("pat" + juce::String (i), KeyMap::toString (engine.getPattern (i)), nullptr);
        state.setProperty ("patName" + juce::String (i), engine.patternName (i), nullptr);
    }
    state.setProperty ("genCount", generatedCount, nullptr);
    state.setProperty ("groove", GrooveGen::toString (engine.getGroove()), nullptr);
    state.setProperty ("grooveSeed", (juce::int64) engine.grooveSeed.load(), nullptr);
    state.setProperty ("grooveName", grooveName, nullptr);
    state.setProperty ("presetIndex", presetIndex, nullptr);
    state.setProperty ("helpSeen", helpSeen, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void HatGoblinProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        if (tree.hasType (apvts.state.getType()))
        {
            const juce::String path = tree.getProperty ("userPath").toString();
            for (int i = 0; i < 12; ++i)
            {
                Pattern pat;
                if (KeyMap::fromString (tree.getProperty ("pat" + juce::String (i)).toString(), pat))
                    engine.setPattern (i, pat, tree.getProperty ("patName" + juce::String (i), KeyMap::factoryPatternName (i)).toString());
                else
                    engine.resetPattern (i);
            }
            generatedCount = (int) tree.getProperty ("genCount", 0);
            GrooveGen::Groove g;
            if (GrooveGen::fromString (tree.getProperty ("groove").toString(), g))
            {
                engine.setGroove (g);
                engine.grooveSeed.store ((juce::uint32) (juce::int64) tree.getProperty ("grooveSeed", 12345));
                grooveName = tree.getProperty ("grooveName", "GROOVE").toString();
                presetIndex = (int) tree.getProperty ("presetIndex", -1);
                history.clear(); historyPos = -1;
                pushHistory();
            }
            helpSeen = (bool) tree.getProperty ("helpSeen", false);
            apvts.replaceState (tree);
            if (path.isNotEmpty())
            {
                const bool wasOn = apvts.getRawParameterValue ("useUser")->load() > 0.5f;
                juce::File f (path);
                if (f.existsAsFile() && loadUserSample (f) && ! wasOn)
                    clearUserSample();
            }
        }
    }
}

#if ! defined (HG_TEST_BUILD)
juce::AudioProcessorEditor* HatGoblinProcessor::createEditor() { return new HatGoblinEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HatGoblinProcessor(); }
#else
juce::AudioProcessorEditor* HatGoblinProcessor::createEditor() { return new HatGoblinEditor (*this); }
#endif
