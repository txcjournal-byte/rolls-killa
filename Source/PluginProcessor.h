#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "RollEngine.h"

class HatGoblinProcessor : public juce::AudioProcessor
{
public:
    HatGoblinProcessor();
    ~HatGoblinProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HAT GOBLIN"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- used by the editor ---
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    RollEngine engine;
    std::atomic<double> currentBpm { 140.0 };
    std::atomic<bool> hostPlaying { false };

    bool loadUserSample (const juce::File& file);     // message thread
    void clearUserSample();
    juce::String userSampleName() const;
    const HatSample* currentSample() const;            // what plays right now
    RollParams currentParams() const;
    int lastRollKey() const noexcept { return lastKey.load(); }
    void setLastRollKey (int k) noexcept { lastKey.store (k); }
    bool flNoteNames() const { return apvts.getRawParameterValue ("noteNames")->load() < 0.5f; }

    // --- GROOVE (main page) ---
    void generate();                                    // new groove from STYLE + COMPLEXITY + LENGTH
    void loadPreset (int index);
    bool canGoBack() const    { return historyPos > 0; }
    bool canGoForward() const { return historyPos + 1 < (int) history.size(); }
    void goBack();
    void goForward();
    void vary();                                        // same grid, new humanize/probability dice
    void clearGroove();
    juce::String grooveName;                            // "TRIPLET 03" / "GEN 12 - STUTTER" / "EDITED"
    int presetIndex = -1;
    void grooveEdited();                                // call after editing the grid (keeps undo working)
    bool helpSeen = false;

    // exports for drag and drop (message thread). Files go to the temp folder.
    // key < 0 = the groove, otherwise a KEYS roll
    juce::File writeMidi (int key = -1);
    juce::File writeAudio (int key = -1);               // exactly as it sounds now, FX included (24-bit WAV)
    juce::File writeHatWav();                           // the current hat sample
    juce::String exportName (int key) const;
    int midiRoot() const { return apvts.getRawParameterValue ("midiTarget")->load() < 0.5f ? 60 : KeyMap::hitRoot; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    void applyFx (juce::AudioBuffer<float>& buffer);

    std::vector<std::unique_ptr<HatSample>> userSamples;   // kept alive for the session (the audio thread may still read one)
    std::atomic<const HatSample*> userSample { nullptr };
    juce::String userPath;
    std::atomic<int> lastKey { 65 };
    struct Snapshot { GrooveGen::Groove groove; juce::uint32 seed; juce::String name; int preset; };
    std::vector<Snapshot> history;
    int historyPos = -1;
    void pushHistory();
    void restore (const Snapshot&);
    int generatedCount = 0;

    juce::dsp::StateVariableTPTFilter<float> hpf, lpf;
    juce::Reverb reverb;
    float crushHold[2] {};
    int crushCounter = 0;
    double sr = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HatGoblinProcessor)
};
