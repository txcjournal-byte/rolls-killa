#pragma once

#include "Parameters.h"
#include "engine/HatSampler.h"
#include "engine/KitExport.h"
#include "engine/KitSampler.h"
#include "engine/MidiExport.h"
#include "engine/UndoHistory.h"
#include "engine/LockFreeSlot.h"
#include "engine/PatternPlayer.h"
#include "engine/RollLibrary.h"
#include "engine/RollModel.h"
#include "engine/SmokeFx.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <limits>

class RollsKillaProcessor : public juce::AudioProcessor,
                            private juce::AudioProcessorValueTreeState::Listener,
                            private juce::Timer
{
public:
    RollsKillaProcessor();
    ~RollsKillaProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    /** Bypassed / switched off: fade the hats out (no click), send note-offs, then stay silent. */
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    // The pattern plays by itself (from the transport), so the plugin is never "done":
    // an infinite tail keeps hosts (FL Studio smart disable) from switching it off mid-pattern.
    double getTailLengthSeconds() const override { return std::numeric_limits<double>::infinity(); }

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
    /** Rolls Killa Mini: random preset + new variation + a musical shuffle of the knobs (one undo step).
        Picks presets that fit the project tempo and the mood (CHILL / TRAP / CRAZY). */
    void killEverything();

    // Tempo: the host tempo is followed automatically; a manual value overrides it (0 = auto)
    double getEffectiveBpm() const;
    double getHostBpm() const noexcept { return hostBpm.load(); }
    double getTargetBpm() const;
    void setTargetBpm (double bpm);

    // Mood for KILL: 0 = CHILL (presets 1-3 of a category), 1 = TRAP (4-6), 2 = CRAZY (7-8)
    int getMood() const;
    void setMood (int mood);

    // History of the last KILLs (Mini): click a dot to go back to that roll
    static constexpr int kMaxKillHistory = 8;
    struct HistoryEntry { juce::ValueTree state; juce::String label; };
    const std::vector<HistoryEntry>& getKillHistory() const noexcept { return killHistory; }
    int getKillHistoryPosition() const noexcept { return killHistoryPos; }
    void restoreKillHistory (int index);
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
    /** MIDI play mode, host playing, but no note held on the channel -> silent on purpose. */
    bool isWaitingForMidi() const noexcept { return waitingForMidi.load(); }
    /** Position inside the pattern in beats, -1 when not running. */
    double getPlayheadBeat() const noexcept { return player.getDisplayBeat(); }
    /** 0..1 - how hard the blunt's ember glows right now (PUFF, UI). */
    float getSmokeGlow() const noexcept { return smoke.getGlow(); }
    /** Increments every time a new pattern is published (UI refresh). */
    int getPatternVersion() const noexcept { return patternVersion.load(); }

    static constexpr int kRootNote = rk::HatSampler::kRootNote;

    // ---- DRUM KIT (Rolls Killa): eight drawers, each a sound + a pattern ----------------------
    // Every change is one undo step and is saved in the project (the kit lives in the state as numbers).
    const rk::DrumKit& getKit() const noexcept { return kit; }
    /** Changes a drawer (knobs, pattern settings). commit = record an undo step now (false = after a pause). */
    void updateKitSlot (rk::DrumType type, const std::function<void (rk::KitSlot&)>& change, bool commit);
    void setKitName (const juce::String& name);
    void killSound (rk::DrumType type);            // a brand new sound in the drawer (current mood)
    void killPattern (rk::DrumType type);          // a new pattern (the hi-hat = a new roll), turns it on
    void killKit();                                // new sounds in every drawer
    void killBeat();                               // new patterns for every playing drawer + a new hi-hat roll
    juce::String loadSlotFile (rk::DrumType type, const juce::File& file);
    void clearSlotFile (rk::DrumType type);
    bool keepSound (rk::DrumType type);            // into the kit list
    void removeKept (int index);
    void auditionSlot (rk::DrumType type) noexcept;
    /** The drawer's current sound (the hi-hat: the rolls sampler's hat). */
    rk::DrumSoundPtr getSlotSound (rk::DrumType type) const;
    /** Counts the hits per drawer (UI flashes the pads). */
    int getHitCount (rk::DrumType type) const noexcept { return hitCounters[(size_t) type].load (std::memory_order_relaxed); }
    std::vector<rk::DrumHit> getSlotPattern (rk::DrumType type) const { return kit.pattern (type); }
    /** Length of the whole beat in beats (the longest playing pattern). */
    double getBeatLength() const noexcept { return beatLength.load(); }
    /** Increments on every kit change (UI refresh). */
    int getKitVersion() const noexcept { return kitVersion.load(); }

    juce::File createSlotMidiFile (rk::DrumType type);
    juce::File createBeatMidiFile();               // all playing drawers, one track each
    rk::KitExportResult exportKitTo (const juce::File& parentDir);
    rk::KitExportResult exportOneShotsTo (const juce::File& parentDir, rk::DrumType type, int count);

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void timerCallback() override;
    rk::ModelSettings readModelSettings() const;
    void setParam (const char* id, float plainValue);
    void markStateChanged();
    void applyRestoredState();
    void syncKitFromState();
    void writeKitToState();
    void refreshKitSounds();
    void mergeKit (rk::PlaybackPattern& pb) const;
    std::vector<rk::KitMidi> kitMidis();
    rk::HatSamplerSettings readSamplerSettings() const noexcept;

    // Declaration order matters: the parameter layout needs the sampler's hat names.
    rk::HatSampler sampler;
    rk::RollLibrary library;
    rk::RollModel model { library };
    juce::AudioProcessorValueTreeState apvts;
    rk::PatternPlayer player;
    rk::SmokeFx smoke;
    rk::LockFreeSlot<rk::PlaybackPattern> patternSlot;
    rk::DrumKit kit;
    rk::KitSampler kitSampler;
    std::array<rk::KitSlot, rk::kNumDrumTypes> renderedSlots;
    std::array<bool, rk::kNumDrumTypes> slotRendered {};
    rk::DrumSoundPtr hatKitSound;
    std::array<std::atomic<int>, rk::kNumDrumTypes> hitCounters {};
    std::atomic<int> pendingAudition { 0 };
    std::atomic<double> beatLength { 4.0 };
    std::atomic<int> kitVersion { 0 };
    std::atomic<float> hatDrawerDb { 0.0f };   // the hi-hat drawer's volume on top of the rolls sampler (Rolls Killa)

    std::atomic<bool> patternDirty { true };
    std::atomic<bool> previewEnabled { false };
    std::atomic<double> hostBpm { 0.0 };
    std::atomic<double> previewBpm { 0.0 };
    std::vector<HistoryEntry> killHistory;
    int killHistoryPos = -1;
    std::atomic<bool> previewRestart { false };
    std::atomic<bool> hostPlaying { false };
    std::atomic<bool> waitingForMidi { false };
    std::array<bool, 128> heldNotes {};
    int numHeldNotes = 0;
    std::array<rk::PlayerEvent, rk::PatternPlayer::kMaxEventsPerBlock> segmentEvents {};
    std::atomic<int> patternVersion { 0 };
    std::atomic<bool> stateChanged { false };
    std::atomic<juce::uint32> lastChangeMs { 0 };
    rk::UndoHistory history;
    double previewPpq = 0.0;
    bool bypassFaded = false;
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
        std::atomic<float>* playMode = nullptr;
        std::atomic<float>* puff = nullptr;
    } raw;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RollsKillaProcessor)
};
