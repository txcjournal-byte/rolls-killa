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

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    void applyFx (juce::AudioBuffer<float>& buffer);

    std::vector<std::unique_ptr<HatSample>> userSamples;   // kept alive for the session (the audio thread may still read one)
    std::atomic<const HatSample*> userSample { nullptr };
    juce::String userPath;
    std::atomic<int> lastKey { 65 };

    juce::dsp::StateVariableTPTFilter<float> hpf, lpf;
    juce::Reverb reverb;
    float crushHold[2] {};
    int crushCounter = 0;
    double sr = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HatGoblinProcessor)
};
