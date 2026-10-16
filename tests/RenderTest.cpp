// Offline sanity test: runs the real processor without a host, plays rolls and patterns,
// checks the output is audible, finite and in range, and writes a demo WAV.
#include "../Source/PluginProcessor.h"
#include <iostream>
#include <set>
#include "../Source/PluginEditor.h"

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
            if (note >= 0 && i == 0) midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 110), 17);
            if (note >= 0 && i == hold) midi.addEvent (juce::MidiMessage::noteOff (1, note), 3);
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

    auto silence = render (proc, { { -1, 0 } }, 40);
    check (silence.peak < 1.0e-6f, "silent while nothing plays");

    // every factory hat plays (a note = one hit, C5 = original pitch)
    int quiet = 0;
    for (int h = 0; h < HatLibrary::get().size(); ++h)
    {
        if (auto* prm = proc.apvts.getParameter ("hat")) prm->setValueNotifyingHost (prm->convertTo0to1 ((float) h));
        auto st = render (proc, { { 60, 20 } }, 30);
        if (! (st.finite && st.peak > 0.01f)) { ++quiet; std::cout << "    silent: " << HatLibrary::get().hat (h).name << std::endl; }
    }
    check (quiet == 0, "all factory hats are audible (" + juce::String (quiet) + " silent)");

    // MIDI export
    RollParams prm;
    auto midi = proc.engine.renderMidi (65, prm);
    int ons = 0;
    for (auto* ev : *midi.getTrack (0)) if (ev->message.isNoteOn()) ++ons;
    check (ons == 16, "DRAG MIDI 1/16 = 16 notes per bar (" + juce::String (ons) + ")");
    auto midiT = proc.engine.renderMidi (66, prm); ons = 0;
    for (auto* ev : *midiT.getTrack (0)) if (ev->message.isNoteOn()) ++ons;
    check (ons == 24, "DRAG MIDI 1/16T = 24 notes per bar (" + juce::String (ons) + ")");

    // ---------------------------------------------------------------- GROOVE (the main workflow)
    auto setP = [] (HatGoblinProcessor& pr, const char* id, float v) { if (auto* q = pr.apvts.getParameter (id)) q->setValueNotifyingHost (q->convertTo0to1 (v)); };
    setP (proc, "hat", 3.0f);
    render (proc, { { -1, 0 } }, 120, nullptr, false);    // let the room tail die away
    {
        auto daw = render (proc, { { -1, 0 } }, 120, nullptr, true);
        check (daw.peak < 1.0e-4f, "quiet while the DAW plays (no doubled hats) " + juce::String (daw.peak, 6));
        setP (proc, "withDaw", 1.0f);
        render (proc, { { -1, 0 } }, 2, nullptr, false);
        auto st = render (proc, { { -1, 0 } }, 120, nullptr, true);
        check (st.finite && st.peak > 0.05f && st.peak <= 1.0f, "WITH DAW on: plays with the DAW (peak " + juce::String (st.peak, 3) + ")");
        render (proc, { { -1, 0 } }, 60, nullptr, false);
        juce::AudioBuffer<float> cap (2, 0);
        render (proc, { { -1, 0 } }, 40, &cap, false);
        check (cap.getMagnitude (0, cap.getNumSamples()) < 1.0e-4f, "stops when the DAW stops");
        render (proc, { { 30, 5 } }, 10, nullptr, true);
        check (proc.engine.pausedByMidi.load(), "notes from the DAW pause the groove");
        render (proc, { { -1, 0 } }, 4, nullptr, false);
        check (! proc.engine.pausedByMidi.load(), "groove comes back when the DAW stops");
        setP (proc, "withDaw", 0.0f);

        proc.engine.previewPlaying.store (true);
        auto pv = render (proc, { { -1, 0 } }, 80, nullptr, false);
        check (pv.peak > 0.05f, "PLAY / space bar plays the groove");
        render (proc, { { -1, 0 } }, 2, nullptr, true);
        check (! proc.engine.previewPlaying.load(), "the DAW starting stops the goblin");
        render (proc, { { -1, 0 } }, 2, nullptr, false);
    }

    // generator: every style, every length, plays and is valid
    {
        int bad = 0;
        juce::Random rng (99);
        for (int style = 0; style < GrooveGen::numStyles; ++style)
            for (int bars : { 1, 2, 4 })
                for (int rep = 0; rep < 20; ++rep)
                {
                    auto g = GrooveGen::generate (style, rng.nextFloat(), bars, rng);
                    int total = 0;
                    for (int b = 0; b < bars * 4; ++b)
                    {
                        const int d = g.div[(size_t) b];
                        if (d != 3 && d != 4 && d != 6) ++bad;
                        for (int k = 0; k < 6; ++k)
                        {
                            const int v = g.hits[(size_t) b][(size_t) k];
                            if (v < 0 || v > 6 || (k >= d && v != 0)) ++bad;
                            total += v;
                        }
                    }
                    if (g.hits[0][0] < 1 || total < bars * 4) ++bad;
                }
        check (bad == 0, "generator: all styles x lengths give valid grooves (" + juce::String (bad) + " bad)");
        std::set<juce::String> uniq;
        for (int i = 0; i < GrooveGen::numPresets(); ++i) uniq.insert (GrooveGen::toString (GrooveGen::preset (i, 2)));
        check ((int) uniq.size() >= GrooveGen::numPresets() - 2, "factory presets are distinct (" + juce::String ((int) uniq.size()) + "/" + juce::String (GrooveGen::numPresets()) + ")");
        check (GrooveGen::preset (5, 1) == GrooveGen::preset (5, 1), "factory presets never change");
        GrooveGen::Groove rt;
        check (GrooveGen::fromString (GrooveGen::toString (GrooveGen::preset (9, 4)), rt) && rt == GrooveGen::preset (9, 4), "groove text round trip");
    }

    // GENERATE / history / every style audible through the processor
    {
        int quietStyles = 0;
        for (int style = 0; style < GrooveGen::numStyles; ++style)
        {
            setP (proc, "style", (float) style);
            proc.generate();
            proc.engine.previewPlaying.store (true);
            auto st = render (proc, { { -1, 0 } }, 90, nullptr, false);
            proc.engine.previewPlaying.store (false);
            render (proc, { { -1, 0 } }, 2, nullptr, false);
            if (st.peak < 0.05f) ++quietStyles;
        }
        check (quietStyles == 0, "GENERATE in every style plays (" + juce::String (quietStyles) + " silent)");
        const auto now = GrooveGen::toString (proc.engine.getGroove());
        proc.goBack();
        const auto before = GrooveGen::toString (proc.engine.getGroove());
        proc.goForward();
        check (before != now && GrooveGen::toString (proc.engine.getGroove()) == now, "back / forward through generated grooves");
        setP (proc, "style", 0.0f);
    }

    // WYSIWYG: the dragged MIDI has exactly the hits you hear, and plays on Hat Goblin
    {
        setP (proc, "humanize", 0.4f); setP (proc, "prob", 0.8f); setP (proc, "pitchRamp", 5.0f);
        for (int bars : { 0, 1, 2 })
        {
            setP (proc, "bars", (float) bars);
            proc.loadPreset (7 + bars * 5);
            const auto p = proc.currentParams();
            std::vector<GrooveHit> hits (RollEngine::maxGrooveHits);
            const int n = proc.engine.realizeGroove (hits.data(), RollEngine::maxGrooveHits, p);
            auto mf = proc.engine.renderGrooveMidi (p, 60);
            int ons = 0; double last = 0;
            for (auto* ev : *mf.getTrack (0)) if (ev->message.isNoteOn()) { ++ons; last = ev->message.getTimeStamp() / 960.0; }
            check (ons == n && n > 0 && last < RollEngine::grooveLength (p), "DRAG MIDI = the groove you hear, " + juce::String (p.bars) + " bar(s): " + juce::String (ons) + " notes");
        }
        setP (proc, "bars", 0.0f);
        // the MIDI for the Hat Goblin channel, fed back into a fresh Hat Goblin with the groove off, plays
        auto mf = proc.engine.renderGrooveMidi (proc.currentParams(), 60);
        HatGoblinProcessor back;
        back.setPlayConfigDetails (0, 2, 44100.0, 512);
        back.prepareToPlay (44100.0, 512);
        juce::AudioBuffer<float> buf (2, 512);
        float peak = 0; int b = 0;
        std::vector<std::pair<double, juce::MidiMessage>> evs;
        for (auto* ev : *mf.getTrack (0)) evs.push_back ({ ev->message.getTimeStamp() / 960.0, ev->message });
        const double spb = 44100.0 * 60.0 / 140.0;
        for (b = 0; b < 40; ++b)
        {
            juce::MidiBuffer m;
            for (auto& [beat, msg] : evs)
            {
                const int at = (int) (beat * spb) - b * 512;
                if (at >= 0 && at < 512) m.addEvent (msg, at);
            }
            back.processBlock (buf, m);
            peak = juce::jmax (peak, buf.getMagnitude (0, 512));
        }
        check (peak > 0.05f, "dragged MIDI plays the factory hat on a Hat Goblin channel (peak " + juce::String (peak, 3) + ")");
        setP (proc, "humanize", 0.15f); setP (proc, "prob", 1.0f); setP (proc, "pitchRamp", 0.0f);
    }

    // DRAG AUDIO / DRAG HAT files
    {
        proc.currentBpm.store (140.0);
        auto wav = proc.writeAudio (-1);
        auto hat = proc.writeHatWav();
        auto keyWav = proc.writeAudio (-1);
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        auto len = [&] (const juce::File& f, float& pk) -> double
        {
            pk = 0;
            std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
            if (r == nullptr) return 0;
            juce::AudioBuffer<float> a ((int) r->numChannels, (int) r->lengthInSamples);
            r->read (&a, 0, (int) r->lengthInSamples, 0, true, true);
            pk = a.getMagnitude (0, a.getNumSamples());
            return r->lengthInSamples / r->sampleRate;
        };
        float p1 = 0, p2 = 0, p3 = 0;
        const double l1 = len (wav, p1), l2 = len (hat, p2), l3 = len (keyWav, p3);
        check (std::abs (l1 - 4.0 * 60.0 / 140.0) < 0.01 && p1 > 0.05f, "DRAG AUDIO: 1 bar WAV, audible (" + juce::String (l1, 3) + " s)");
        check (l2 > 0.01 && p2 > 0.05f, "DRAG HAT: the sample as WAV");
        check (l3 > 1.0 && p3 > 0.05f, "DRAG AUDIO again gives the same length");
    }

    // knob reset values exist
    {
        auto* sw = proc.apvts.getParameter ("swing");
        check (sw != nullptr && sw->getDefaultValue() == sw->convertTo0to1 (0.0f), "swing resets to 0");
    }

    // state round trip
    juce::MemoryBlock mb;
    proc.getStateInformation (mb);
    HatGoblinProcessor proc2;
    proc2.setStateInformation (mb.getData(), (int) mb.getSize());
    check (proc2.apvts.getRawParameterValue ("hat")->load() == proc.apvts.getRawParameterValue ("hat")->load(), "state save / load");
    check (GrooveGen::toString (proc2.engine.getGroove()) == GrooveGen::toString (proc.engine.getGroove())
           && proc2.engine.grooveSeed.load() == proc.engine.grooveSeed.load() && proc2.grooveName == proc.grooveName, "groove saved with the project");

    // generator demo: HatGoblinTests --grooves out.wav  (every style, 2 bars each, at 140 BPM)
    if (argc > 2 && juce::String (argv[1]) == "--grooves")
    {
        juce::AudioBuffer<float> cap (2, 0);
        const int hats[] = { 3, 22, 41, 2, 64, 85, 27, 100 };
        setP (proc, "bars", 1.0f);
        for (int style = 1; style < GrooveGen::numStyles; ++style)
        {
            setP (proc, "style", (float) style);
            setP (proc, "complexity", 0.55f);
            setP (proc, "hat", (float) hats[style - 1]);
            setP (proc, "pitchRamp", style == 3 || style == 7 ? 5.0f : 0.0f);
            setP (proc, "swing", style == 5 ? 0.25f : 0.0f);
            proc.generate();
            render (proc, { { -1, 0 } }, 2, nullptr, false);
            // 2 bars at 140 = 3.43 s = 295 blocks of 512
            proc.engine.previewPlaying.store (true);
            render (proc, { { -1, 0 } }, 296, &cap, false);
            proc.engine.previewPlaying.store (false);
            render (proc, { { -1, 0 } }, 30, &cap, false);   // a short gap
            std::cout << "  " << GrooveGen::styleName (style) << ": " << proc.grooveName << std::endl;
        }
        juce::File out (argv[2]); out.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (out), 44100.0, 2, 16, {}, 0));
        if (w) w->writeFromAudioSampleBuffer (cap, 0, cap.getNumSamples());
        return 0;
    }

    // showcase audio: HatGoblinTests --showcase out.wav
    if (argc > 2 && juce::String (argv[1]) == "--showcase")
    {
        auto setQ = [&] (const char* id, float v) { setP (proc, id, v); };

        juce::AudioBuffer<float> cap (2, 0);
        struct Seg { int hat; std::vector<std::pair<int,int>> notes; int blocks; float swing; int shape; float ramp; };
        std::vector<Seg> segs {
            { 2,  { { 65, 70 }, { 67, 70 }, { 65, 70 }, { 70, 70 } }, 74, 0.0f, 1, 0.0f },   // BASIC: 1/16, 1/32, 1/16, 1/64T
            { 21, { { 66, 70 }, { 68, 70 }, { 66, 70 }, { 69, 70 } }, 74, 0.0f, 1, 4.0f },   // CRISP: triplets, pitch ramp
            { 43, { { 53, 290 } }, 296, 0.15f, 3, 0.0f },                                  // DIRTY: SKIP pattern
            { 65, { { 48, 290 } }, 296, 0.2f, 3, 0.0f },                                   // LOFI: FLICK 3
            { 83, { { 56, 290 } }, 296, 0.0f, 1, 0.0f },                                   // EXTREME: RATCHET
            { 101,{ { 57, 290 } }, 296, 0.0f, 2, -5.0f },                                  // WEIRD: GLITCH
        };
        for (auto& sg : segs)
        {
            setQ ("hat", (float) sg.hat); setQ ("swing", sg.swing); setQ ("velShape", (float) sg.shape); setQ ("pitchRamp", sg.ramp);
            render (proc, sg.notes, sg.blocks, &cap, false);
        }
        juce::File out (argv[2]); out.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (out), 44100.0, 2, 16, {}, 0));
        if (w) w->writeFromAudioSampleBuffer (cap, 0, cap.getNumSamples());
        std::cout << "  showcase written" << std::endl;
        return 0;
    }

    // UI snapshots (run under Xvfb): HatGoblinTests --snapshot <dir>
    if (argc > 2 && juce::String (argv[1]) == "--snapshot")
    {
        juce::File dir (argv[2]);
        dir.createDirectory();
        proc.helpSeen = true;
        setP (proc, "style", 0.0f);
        proc.loadPreset (8);
        proc.engine.previewPlaying.store (true);
        render (proc, { { -1, 0 } }, 37, nullptr, false);     // playhead somewhere in the bar
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        ed->setSize (HatGoblinEditor::baseW, HatGoblinEditor::baseH);
        auto snap = [&] (const juce::String& name)
        {
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
            juce::PNGImageFormat png;
            juce::FileOutputStream os (dir.getChildFile (name + ".png"));
            os.setPosition (0); os.truncate();
            png.writeImageToStream (img, os);
        };
        std::function<void (juce::Component&)> pump;
        auto clickTab = [&] (const juce::String& text)
        {
            std::function<juce::TextButton* (juce::Component&)> find = [&] (juce::Component& c) -> juce::TextButton*
            {
                for (auto* ch : c.getChildren())
                {
                    if (auto* b = dynamic_cast<juce::TextButton*> (ch); b != nullptr && b->getButtonText() == text) return b;
                    if (auto* f = find (*ch)) return f;
                }
                return nullptr;
            };
            if (auto* b = find (*ed)) { b->setToggleState (true, juce::dontSendNotification); if (b->onClick) b->onClick(); }
        };
        snap ("groove");
        clickTab ("?");     snap ("help");
        clickTab ("GOT IT - LET'S ROLL");
        clickTab ("SHAPE"); snap ("shape");
        clickTab ("FX");    snap ("fx");
        clickTab ("KIT");   snap ("kit");

        ed.reset();
        std::cout << "  snapshots written to " << dir.getFullPathName() << std::endl;
        return 0;
    }

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
