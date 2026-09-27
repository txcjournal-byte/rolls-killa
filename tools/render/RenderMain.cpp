// Offline renderer: plays every factory preset through the real plugin processor with a fake host
// transport and writes WAV (+ MIDI note count check). Used for listening tests and as a smoke test.
//
//   RollsKillaRender <outDir> [--loops N] [--preset "Name"] [--check]

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
        if (auto stream = file.createOutputStream())
            if (auto writer = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (stream.get(), sr, 2, 24, {}, 0)))
            {
                stream.release();
                writer->writeFromAudioSampleBuffer (out, 0, totalSamples);
            }
    }

    proc.releaseResources();
    proc.setPlayHead (nullptr);
    std::cout << (failures == 0 ? "RENDER OK" : "RENDER FAILURES: " + std::to_string (failures)) << std::endl;
    return failures == 0 ? 0 : 1;
}
