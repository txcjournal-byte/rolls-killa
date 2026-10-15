// Offline sanity test: runs the real processor without a host, plays rolls and patterns,
// checks the output is audible, finite and in range, and writes a demo WAV.
#include "../Source/PluginProcessor.h"
#include <iostream>

static int failures = 0;
static void check (bool ok, const juce::String& what)
{
    std::cout << (ok ? "  ok   " : "  FAIL ") << what << std::endl;
    if (! ok) ++failures;
}

struct Stats { float peak = 0; double rms = 0; bool finite = true; };

static Stats render (HatGoblinProcessor& p, std::vector<std::pair<int, int>> notes /* note, holdBlocks */, int blocks,
                     juce::AudioBuffer<float>* capture = nullptr, bool playing = false)
{
    const int bs = 512;
    juce::AudioBuffer<float> buf (2, bs);
    Stats st; double sum = 0; long long n = 0;
    int b = 0;
    struct Head : juce::AudioPlayHead
    {
        bool playing = false; double ppq = 0;
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo info; info.setBpm (140.0); info.setIsPlaying (playing); info.setPpqPosition (ppq); return info;
        }
    } head;
    head.playing = playing;
    p.setPlayHead (&head);
    for (auto [note, hold] : notes)
    {
        for (int i = 0; i < blocks; ++i, ++b)
        {
            juce::MidiBuffer midi;
            if (i == 0) midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 110), 17);
            if (i == hold) midi.addEvent (juce::MidiMessage::noteOff (1, note), 3);
            p.processBlock (buf, midi);
            head.ppq += bs / 44100.0 * 140.0 / 60.0;
            for (int ch = 0; ch < 2; ++ch)
            {
                auto* d = buf.getReadPointer (ch);
                for (int s = 0; s < bs; ++s)
                {
                    if (! std::isfinite (d[s])) st.finite = false;
                    st.peak = juce::jmax (st.peak, std::abs (d[s]));
                    sum += d[s] * d[s]; ++n;
                }
            }
            if (capture != nullptr)
            {
                const int at = capture->getNumSamples();
                capture->setSize (2, at + bs, true);
                for (int ch = 0; ch < 2; ++ch) capture->copyFrom (ch, at, buf, ch, 0, bs);
            }
        }
    }
    p.setPlayHead (nullptr);
    st.rms = n ? std::sqrt (sum / n) : 0;
    return st;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    HatGoblinProcessor proc;
    proc.setPlayConfigDetails (0, 2, 44100.0, 512);
    proc.prepareToPlay (44100.0, 512);

    std::cout << "HAT GOBLIN render test" << std::endl;
    check (HatLibrary::get().size() == 120, "120 factory hats loaded (" + juce::String (HatLibrary::get().size()) + ")");

    auto silence = render (proc, { { 30, 1 } }, 40);
    check (silence.peak < 1.0e-6f, "unmapped key stays silent");

    for (int note = KeyMap::firstRate; note <= KeyMap::lastRate; ++note)
    {
        auto st = render (proc, { { note, 60 } }, 80);
        check (st.finite && st.peak > 0.02f && st.peak <= 1.0f, "rate " + KeyMap::rateLabel (note) + " rolls (peak " + juce::String (st.peak, 3) + ")");
    }
    for (int note = KeyMap::firstPattern; note <= KeyMap::lastPattern; ++note)
    {
        auto st = render (proc, { { note, 120 } }, 140);
        check (st.finite && st.peak > 0.02f, "pattern " + KeyMap::patternName (note - KeyMap::firstPattern));
    }

    // every factory hat plays
    int quiet = 0;
    for (int h = 0; h < HatLibrary::get().size(); ++h)
    {
        if (auto* prm = proc.apvts.getParameter ("hat")) prm->setValueNotifyingHost (prm->convertTo0to1 ((float) h));
        auto st = render (proc, { { 65, 20 } }, 30);
        if (! (st.finite && st.peak > 0.01f)) { ++quiet; std::cout << "    silent: " << HatLibrary::get().hat (h).name << std::endl; }
    }
    check (quiet == 0, "all factory hats are audible (" + juce::String (quiet) + " silent)");

    // latch: a released key keeps rolling, pressing it again stops it
    if (auto* l = proc.apvts.getParameter ("latch")) l->setValueNotifyingHost (1.0f);
    {
        juce::AudioBuffer<float> cap (2, 0);
        render (proc, { { 65, 2 } }, 40, &cap);                  // tap and release
        const float tail = cap.getMagnitude (0, cap.getNumSamples() - 20 * 512, 20 * 512);
        check (tail > 0.02f, "latch keeps rolling after the key is released");
        juce::AudioBuffer<float> cap2 (2, 0);
        render (proc, { { 65, 2 } }, 60, &cap2);                 // tap again = stop
        const float tail2 = cap2.getMagnitude (0, cap2.getNumSamples() - 20 * 512, 20 * 512);
        check (tail2 < 0.01f, "pressing a latched roll again stops it");
    }
    if (auto* l = proc.apvts.getParameter ("latch")) l->setValueNotifyingHost (0.0f);

    // host transport playing
    auto host = render (proc, { { 67, 80 } }, 100, nullptr, true);
    check (host.finite && host.peak > 0.02f, "rolls follow a playing host transport");

    // MIDI export
    RollParams prm;
    auto midi = RollEngine::renderMidi (65, prm);
    int ons = 0;
    for (auto* ev : *midi.getTrack (0)) if (ev->message.isNoteOn()) ++ons;
    check (ons == 16, "DRAG MIDI 1/16 = 16 notes per bar (" + juce::String (ons) + ")");
    auto midiT = RollEngine::renderMidi (66, prm); ons = 0;
    for (auto* ev : *midiT.getTrack (0)) if (ev->message.isNoteOn()) ++ons;
    check (ons == 24, "DRAG MIDI 1/16T = 24 notes per bar (" + juce::String (ons) + ")");

    // state round trip
    juce::MemoryBlock mb;
    proc.getStateInformation (mb);
    HatGoblinProcessor proc2;
    proc2.setStateInformation (mb.getData(), (int) mb.getSize());
    check (proc2.apvts.getRawParameterValue ("hat")->load() == proc.apvts.getRawParameterValue ("hat")->load(), "state save / load");

    // demo WAV: a little roll sequence
    if (argc > 1)
    {
        if (auto* prm2 = proc.apvts.getParameter ("hat")) prm2->setValueNotifyingHost (prm2->convertTo0to1 (0.0f));
        juce::AudioBuffer<float> cap (2, 0);
        render (proc, { { 65, 40 }, { 66, 30 }, { 67, 30 }, { 53, 120 }, { 70, 25 }, { 48, 120 } }, 60, &cap);
        juce::File out (argv[1]);
        out.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (out), 44100.0, 2, 16, {}, 0));
        if (w) w->writeFromAudioSampleBuffer (cap, 0, cap.getNumSamples());
        std::cout << "  demo written: " << out.getFullPathName() << std::endl;
    }

    std::cout << (failures == 0 ? "ALL TESTS PASSED" : juce::String (failures) + " FAILED") << std::endl;
    return failures == 0 ? 0 : 1;
}
