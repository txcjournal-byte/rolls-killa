#pragma once

#include "Parameters.h"
#include "engine/HatSampler.h"
#include "engine/MidiExport.h"
#include "engine/UndoHistory.h"
#include "engine/LockFreeSlot.h"
#include "engine/PatternPlayer.h"
#include "engine/RollLibrary.h"
#include "engine/RollModel.h"

#include <juce_audio_processors/juce_audio_processors.h>

class RollsKillaProcessor : public juce::AudioProcessor,
                            private juce::AudioProcessorValueTreeState::Listener,
                            private juce::Timer
{
public:
    RollsKillaProcessor();
    ~RollsKillaProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- message thread API for the editor ---------------------------------
    juce::AudioProcessorValueTreeState& getState() noexcept { return apvts; }
    rk::RollLibrary& getLibrary() noexcept { return library; }
    rk::RollModel& getModel() noexcept { return model; }
    rk::HatSampler& getSampler() noexcept { return sampler; }

    /** Rebuilds the pattern from the current parameters right away (message thread). */
    void rebuildPattern (bool force = false);

    // Presets / KILL
    void loadPreset (int index);                    // resets KILL seed and edits
    void stepPreset (int delta);                    // prev/next inside the whole library
    int getPresetIndex() const;
    void kill();                                    // new variation seed
    void resetVariation();                          // back to the original preset
    uint32_t getSeed() const;

    // Lock bars (bit per bar) and visualizer edits
    uint32_t getLockedBars() const;
    void setBarLocked (int bar, bool locked);
    rk::NoteEdits getEdits() const;
    void setNoteEdit (int tick, const rk::NoteEdit& edit);
    void clearEdits();

    // Undo / redo (snapshots of the whole state)
    bool undo();
    bool redo();
    bool canUndo() const noexcept { return history.canUndo(); }
    bool canRedo() const noexcept { return history.canRedo(); }
    /** Records the current state as an undo step now (discrete actions; knob drags are debounced). */
    void commitUndoStep();

    // Sampler
    juce::String loadCustomSample (const juce::File& file);

    // MIDI out of the plugin
    rk::MidiExportOptions getMidiExportOptions() const;
    juce::File createDragMidiFile();
    bool exportMidi (const juce::File& file);
    int saveUserPreset (const juce::String& name);

    void setPreviewEnabled (bool shouldPreview) noexcept;
    bool isPreviewEnabled() const noexcept { return previewEnabled.load(); }
    bool isHostPlaying() const noexcept { return hostPlaying.load(); }
    /** Position inside the pattern in beats, -1 when not running. */
    double getPlayheadBeat() const noexcept { return player.getDisplayBeat(); }
    /** Increments every time a new pattern is published (UI refresh). */
    int getPatternVersion() const noexcept { return patternVersion.load(); }

    static constexpr int kRootNote = rk::HatSampler::kRootNote;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void timerCallback() override;
    rk::ModelSettings readModelSettings() const;
    void setParam (const char* id, float plainValue);
    void markStateChanged();
    void applyRestoredState();
    rk::HatSamplerSettings readSamplerSettings() const noexcept;

    // Declaration order matters: the parameter layout needs the sampler's hat names.
    rk::HatSampler sampler;
    rk::RollLibrary library;
    rk::RollModel model { library };
    juce::AudioProcessorValueTreeState apvts;
    rk::PatternPlayer player;
    rk::LockFreeSlot<rk::PlaybackPattern> patternSlot;

    std::atomic<bool> patternDirty { true };
    std::atomic<bool> previewEnabled { false };
    std::atomic<bool> previewRestart { false };
    std::atomic<bool> hostPlaying { false };
    std::atomic<int> patternVersion { 0 };
    std::atomic<bool> stateChanged { false };
    std::atomic<juce::uint32> lastChangeMs { 0 };
    rk::UndoHistory history;
    double previewPpq = 0.0;
    double currentSampleRate = 44100.0;

    std::array<rk::PlayerEvent, rk::PatternPlayer::kMaxEventsPerBlock> events {};
    std::array<rk::PlayerEvent, 256> inputNotes {};

    struct RawParams
    {
        std::atomic<float>* hat = nullptr;
        std::atomic<float>* tune = nullptr;
        std::atomic<float>* decay = nullptr;
        std::atomic<float>* choke = nullptr;
        std::atomic<float>* volume = nullptr;
    } raw;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RollsKillaProcessor)
};
