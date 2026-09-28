// Offline renderer: plays every factory preset through the real plugin processor with a fake host
// transport and writes WAV (+ MIDI note count check). Used for listening tests and as a smoke test.
//
//   RollsKillaRender <outDir> [--loops N] [--preset "Name"] [--check]

#include "PluginEditor.h"
#include "ui/Sigils.h"
#include "PluginProcessor.h"

#include <juce_audio_formats/juce_audio_formats.h>

namespace
{
    struct FakePlayHead : juce::AudioPlayHead
    {
        double ppq = 0.0, bpm = 140.0;
        bool playing = true;

        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo info;
            info.setIsPlaying (playing);
            info.setPpqPosition (ppq);
            info.setBpm (bpm);
            info.setTimeSignature (juce::AudioPlayHead::TimeSignature { 4, 4 });
            return info;
        }
    };
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI init;

    juce::StringArray args;
    for (int i = 1; i < argc; ++i)
        args.add (argv[i]);

    if (args.isEmpty())
    {
        std::cout << "usage: RollsKillaRender <outDir> [--loops N] [--preset Name] [--check]" << std::endl;
        return 1;
    }

    const juce::File outDir = juce::File::getCurrentWorkingDirectory().getChildFile (args[0]);
    const auto loops = args.contains ("--loops") ? args[args.indexOf ("--loops") + 1].getIntValue() : 2;
    const auto only = args.contains ("--preset") ? args[args.indexOf ("--preset") + 1] : juce::String();
    const auto checkOnly = args.contains ("--check");
    const auto screenshot = args.contains ("--screenshot");
    outDir.createDirectory();

    const double sr = 44100.0;
    const int block = 512;
    int failures = 0;

    std::unique_ptr<juce::AudioProcessor> base (createPluginFilter());
    auto& proc = dynamic_cast<RollsKillaProcessor&> (*base);
    FakePlayHead playHead;
    proc.setPlayHead (&playHead);
    proc.setPlayConfigDetails (0, 2, sr, block);
    proc.prepareToPlay (sr, block);

    auto& lib = proc.getLibrary();

    // renders and checks run the pattern from the transport (no notes on the channel)
    if (auto* pm = proc.getState().getParameter (rk::params::playMode))
        pm->setValueNotifyingHost (pm->convertTo0to1 (1.0f));

    if (args.contains ("--demo"))
    {
        // One listening file: calmest (1) and craziest (8) preset of every category, one loop each.
        const auto file = outDir.getChildFile ("RollsKilla_Demo.wav");
        juce::AudioBuffer<float> demo (1, 0);
        std::cout << "Demo order:" << std::endl;
        double t = 0.0;

        for (int c = 0; c < rk::kNumFactoryCategories; ++c)
        {
            const auto idx = lib.presetsInCategory (c);
            for (auto i : { idx.front(), idx.back() })
            {
                const auto& preset = lib.getPreset (i);
                proc.loadPreset (i);
                if (auto* barsParam = proc.getState().getParameter (rk::params::bars))
                    barsParam->setValueNotifyingHost (barsParam->convertTo0to1 ((float) rk::params::choiceFromBars (juce::jmin (2, preset.bars()))));
                proc.rebuildPattern();
                proc.prepareToPlay (sr, block);

                playHead.bpm = preset.bpmHint;
                const auto loopBeats = proc.getModel().getPattern().lengthBeats() * (preset.bars() == 1 ? 2 : 1);
                const auto n = (int) std::ceil (loopBeats * 60.0 / playHead.bpm * sr);
                const auto gap = (int) (0.6 * sr);
                const auto start = demo.getNumSamples();
                demo.setSize (1, start + n + gap, true, true);

                juce::AudioBuffer<float> buf (2, block);
                juce::MidiBuffer midi;
                for (int pos = 0; pos < n + gap; pos += block)
                {
                    const auto len = std::min (block, n + gap - pos);
                    playHead.playing = pos < n;
                    playHead.ppq = pos * playHead.bpm / 60.0 / sr;
                    buf.setSize (2, len, false, false, true);
                    midi.clear();
                    proc.processBlock (buf, midi);
                    demo.addFrom (0, start + pos, buf, 0, 0, len, 0.5f);
                    demo.addFrom (0, start + pos, buf, 1, 0, len, 0.5f);
                }

                std::cout << "  " << juce::String (t, 1).paddedLeft (' ', 6) << " s  "
                          << juce::String (rk::getCategoryProfile (c).name).paddedRight (' ', 13) << " " << preset.name
                          << " (" << (int) preset.bpmHint << " BPM)" << std::endl;
                t += (double) (n + gap) / sr;
            }
        }

        demo.applyGain (0.8f / juce::jmax (0.001f, demo.getMagnitude (0, 0, demo.getNumSamples())));
        file.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());
        if (auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions {}.withSampleRate (sr).withNumChannels (1).withBitsPerSample (16)))
            writer->writeFromAudioSampleBuffer (demo, 0, demo.getNumSamples());
        std::cout << "wrote " << file.getFullPathName() << std::endl;
        return 0;
    }

    if (screenshot)
    {
        // Renders the editor (and the preset browser) to PNG files in outDir.
        if (only.isNotEmpty())
            for (int i = 0; i < lib.getNumPresets(); ++i)
                if (lib.getPreset (i).name.equalsIgnoreCase (only))
                    proc.loadPreset (i);

        std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
        editor->setVisible (true);

        // advance the playhead a bit so the visualizer shows it
        playHead.bpm = proc.getModel().getPreset().bpmHint;
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;
        for (int b = 0; b < 60; ++b)
        {
            playHead.ppq = b * block * playHead.bpm / 60.0 / sr;
            proc.processBlock (buf, midi);
        }

        auto save = [&] (const juce::String& name, float scale)
        {
            const auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, scale);
            const auto file = outDir.getChildFile (name);
            file.deleteFile();
            juce::FileOutputStream os (file);
            juce::PNGImageFormat().writeImageToStream (img, os);
            std::cout << "wrote " << file.getFullPathName() << std::endl;
        };

        save ("ui_main.png", 2.0f);

        {
            const auto logo = rk::ui::renderLogo (60, 5.0f);
            juce::FileOutputStream os (outDir.getChildFile ("logo.png"));
            os.setPosition (0);
            os.truncate();
            juce::PNGImageFormat().writeImageToStream (logo, os);
        }

        if (auto* browser = editor->findChildWithID ("browser"))
            juce::ignoreUnused (browser);

        for (auto* c : editor->getChildren())
            for (auto* cc : c->getChildren())
                if (auto* b = dynamic_cast<rk::ui::PresetBrowser*> (cc))
                {
                    b->open (proc.getModel().getPreset().category);
                    save ("ui_browser.png", 2.0f);
                }

        editor.reset();
        proc.setPlayHead (nullptr);
        return 0;
    }
    auto* presetParam = proc.getState().getParameter (rk::params::preset);

    for (int i = 0; i < lib.getNumFactoryPresets(); ++i)
    {
        const auto& preset = lib.getPreset (i);
        if (only.isNotEmpty() && ! preset.name.equalsIgnoreCase (only))
            continue;

        presetParam->setValueNotifyingHost (presetParam->convertTo0to1 ((float) i));
        if (auto* barsParam = proc.getState().getParameter (rk::params::bars))
            barsParam->setValueNotifyingHost (barsParam->convertTo0to1 ((float) rk::params::choiceFromBars (preset.bars())));
        proc.rebuildPattern();

        playHead.bpm = preset.bpmHint;
        playHead.ppq = 0.0;
        const auto beats = preset.pattern.lengthBeats() * loops;
        const auto totalSamples = (int) std::ceil (beats * 60.0 / playHead.bpm * sr) + (int) (0.3 * sr);

        juce::AudioBuffer<float> out (2, totalSamples);
        out.clear();
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;
        int noteOns = 0, noteOffs = 0;
        double playedUntilPpq = 0.0;

        for (int pos = 0; pos < totalSamples; pos += block)
        {
            const auto n = std::min (block, totalSamples - pos);
            playHead.playing = pos < totalSamples - (int) (0.3 * sr);
            playHead.ppq = pos * playHead.bpm / 60.0 / sr;
            buf.setSize (2, n, false, false, true);
            midi.clear();
            proc.processBlock (buf, midi);
            if (playHead.playing)
                playedUntilPpq = (pos + n) * playHead.bpm / 60.0 / sr;
            for (const auto m : midi)
            {
                noteOns += m.getMessage().isNoteOn() ? 1 : 0;
                noteOffs += m.getMessage().isNoteOff() ? 1 : 0;
            }
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, pos, buf, ch, 0, n);
        }

        // Every preset note that starts inside the played range must come out as MIDI.
        int expectedOns = 0;
        for (double loopStart = 0.0; loopStart < playedUntilPpq; loopStart += preset.pattern.lengthBeats())
            for (const auto& note : preset.pattern.notes)
                expectedOns += loopStart + note.beat < playedUntilPpq - 1.0e-9 ? 1 : 0;
        const auto peak = out.getMagnitude (0, totalSamples);
        const auto ok = noteOns == expectedOns && noteOffs == noteOns && peak > 0.05f && peak < 4.0f;
        failures += ok ? 0 : 1;

        std::cout << (ok ? "ok   " : "FAIL ") << juce::String (rk::getCategoryProfile (preset.category).name).paddedRight (' ', 13)
                  << " " << preset.name.paddedRight (' ', 20) << " notes " << noteOns << "/" << expectedOns
                  << " offs " << noteOffs << " peak " << juce::String (juce::Decibels::gainToDecibels (peak), 1) << " dB" << std::endl;

        if (checkOnly)
            continue;

        // Normalise loosely so quiet presets are audible, keep headroom.
        if (peak > 0.0f)
            out.applyGain (juce::jmin (1.0f, 0.8f / peak));

        const auto file = outDir.getChildFile (juce::String (i + 1).paddedLeft ('0', 2) + " "
                                               + juce::File::createLegalFileName (juce::String (rk::getCategoryProfile (preset.category).name)
                                                                                  + " - " + preset.name) + ".wav");
        file.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());
        if (stream != nullptr)
            if (auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions {}.withSampleRate (sr)
                                                                                        .withNumChannels (2)
                                                                                        .withBitsPerSample (24)))
                writer->writeFromAudioSampleBuffer (out, 0, totalSamples);
    }

    if (checkOnly)
    {
        auto expectTrue = [&failures] (bool ok, const char* what)
        {
            std::cout << (ok ? "ok   " : "FAIL ") << what << std::endl;
            failures += ok ? 0 : 1;
        };

        auto samePattern = [] (const rk::Pattern& a, const rk::Pattern& b)
        {
            if (a.notes.size() != b.notes.size() || a.bars != b.bars)
                return false;
            for (size_t i = 0; i < a.notes.size(); ++i)
                if (std::abs (a.notes[i].beat - b.notes[i].beat) > 1.0e-9 || a.notes[i].vel != b.notes[i].vel
                    || a.notes[i].pitch != b.notes[i].pitch || a.notes[i].muted != b.notes[i].muted)
                    return false;
            return true;
        };

        auto set = [&proc] (const char* id, float v)
        {
            auto* prm = proc.getState().getParameter (id);
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
        };

        // ---- state round trip -------------------------------------------------
        proc.loadPreset (lib.search ("Codex Trappus").front());
        set (rk::params::bars, 2.0f);
        set (rk::params::density, 140.0f);
        set (rk::params::swing, 20.0f);
        set (rk::params::rollSpeed, 2.0f);
        proc.rebuildPattern();
        proc.kill();
        proc.setBarLocked (1, true);
        const auto firstTick = proc.getModel().getEditablePattern().notes.front().srcTick;
        proc.setNoteEdit (firstTick, { true, -1, -1 });
        const auto before = proc.getModel().getPattern();

        juce::MemoryBlock state;
        proc.getStateInformation (state);

        std::unique_ptr<juce::AudioProcessor> other (createPluginFilter());
        auto& proc2 = dynamic_cast<RollsKillaProcessor&> (*other);
        proc2.setStateInformation (state.getData(), (int) state.getSize());
        expectTrue (samePattern (before, proc2.getModel().getPattern()), "state round trip restores the exact pattern");
        expectTrue (proc2.getSeed() == proc.getSeed() && proc2.getSeed() != 0, "KILL seed restored");
        expectTrue (proc2.getLockedBars() == 2u, "bar locks restored");
        expectTrue (proc2.getEdits().size() == 1 && proc2.getEdits().begin()->second.muted, "visualizer edits restored");
        expectTrue (proc2.getModel().getPattern().notes.front().muted, "muted note stays muted");

        // ---- undo / redo ------------------------------------------------------
        {
            std::unique_ptr<juce::AudioProcessor> third (createPluginFilter());
            auto& p3 = dynamic_cast<RollsKillaProcessor&> (*third);
            const auto original = p3.getModel().getPattern();
            p3.kill();
            const auto killed = p3.getModel().getPattern();
            expectTrue (p3.canUndo(), "KILL can be undone");
            p3.undo();
            expectTrue (samePattern (original, p3.getModel().getPattern()), "undo restores the pattern before KILL");
            p3.redo();
            expectTrue (samePattern (killed, p3.getModel().getPattern()), "redo brings the KILL back");
        }

        // ---- MIDI play mode: silent without a note, plays while a note is held --
        {
            std::unique_ptr<juce::AudioProcessor> gateBase (createPluginFilter());
            auto& gp = dynamic_cast<RollsKillaProcessor&> (*gateBase);
            FakePlayHead gph;
            gph.bpm = 140.0;
            gp.setPlayHead (&gph);
            gp.setPlayConfigDetails (0, 2, sr, block);
            gp.prepareToPlay (sr, block);
            juce::AudioBuffer<float> buf (2, block);

            auto runBlocks = [&] (int blocks, bool holdNote, int& ons, float& peak)
            {
                for (int b = 0; b < blocks; ++b)
                {
                    juce::MidiBuffer midi;
                    if (holdNote && b == 0)
                        midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
                    gp.processBlock (buf, midi);
                    gph.ppq += block * gph.bpm / 60.0 / sr;
                    for (const auto m : midi)
                        ons += m.getMessage().isNoteOn() ? 1 : 0;
                    peak = std::max (peak, buf.getMagnitude (0, block));
                }
            };

            int onsSilent = 0, onsHeld = 0;
            float peakSilent = 0.0f, peakHeld = 0.0f;
            runBlocks (200, false, onsSilent, peakSilent);
            runBlocks (200, true, onsHeld, peakHeld);
            expectTrue (onsSilent == 0 && peakSilent < 1.0e-6f && gp.isWaitingForMidi() == false,
                        "MIDI mode: host playing, no note -> silent");
            expectTrue (onsHeld > 0 && peakHeld > 0.05f, "MIDI mode: note held -> pattern plays");
            gp.setPlayHead (nullptr);
        }

        // ---- performance ------------------------------------------------------
        {
            proc.loadPreset (lib.search ("Thrash Chapel").front());
            set (rk::params::bars, 3.0f);
            set (rk::params::density, 200.0f);
            set (rk::params::rollSpeed, 2.0f);
            set (rk::params::choke, 0.0f);
            proc.rebuildPattern();
            playHead.bpm = 165.0;
            playHead.playing = true;
            juce::AudioBuffer<float> buf (2, 128);
            juce::MidiBuffer midi;
            const int blocks = (int) (sr * 20.0 / 128.0);   // 20 s of audio
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            for (int b = 0; b < blocks; ++b)
            {
                playHead.ppq = b * 128.0 * playHead.bpm / 60.0 / sr;
                midi.clear();
                proc.processBlock (buf, midi);
            }
            const auto ms = juce::Time::getMillisecondCounterHiRes() - t0;
            const auto cpu = ms / 20000.0 * 100.0;
            std::cout << "     20 s of the densest pattern (128-sample blocks, no choke) took " << juce::String (ms, 1)
                      << " ms = " << juce::String (cpu, 2) << " % CPU" << std::endl;
            expectTrue (cpu < 5.0, "processBlock uses < 5 % of one core");
        }
    }

    proc.releaseResources();
    proc.setPlayHead (nullptr);
    std::cout << (failures == 0 ? "RENDER OK" : "RENDER FAILURES: " + std::to_string (failures)) << std::endl;
    return failures == 0 ? 0 : 1;
}
