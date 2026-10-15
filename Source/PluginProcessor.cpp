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
    return { p.begin(), p.end() };
}

HatGoblinProcessor::HatGoblinProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "HATGOBLIN", createLayout())
{
    HatLibrary::get();   // load the factory hats now, not on the audio thread
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

void HatGoblinProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("userPath", userPath, nullptr);
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
