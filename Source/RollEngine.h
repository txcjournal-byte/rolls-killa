#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "HatLibrary.h"
#include <array>
#include <atomic>

// Key map (C3 = MIDI 60):
//   C1 (36) pitch -1 st   D1 (38) pitch +1 st   E1 (40) latch on/off
//   C2..B2 (48..59)  patterns P1..P12
//   C3..C4 (60..72)  rates 1/2 .. 1/256, triplets on the black keys
namespace KeyMap
{
    constexpr int pitchDown = 36, pitchUp = 38, latch = 40;
    constexpr int firstPattern = 48, lastPattern = 59;
    constexpr int firstRate = 60, lastRate = 72;
    inline bool isRate (int n)    { return n >= firstRate && n <= lastRate; }
    inline bool isPattern (int n) { return n >= firstPattern && n <= lastPattern; }
    double rateBeats (int note);                 // length of one step in quarter notes
    juce::String rateLabel (int note);           // "1/16T"
    juce::String patternName (int index);        // 0..11
    const std::array<int, 16>& pattern (int index);
    juce::String keyLabel (int note);            // label drawn on the on-screen keyboard
}

struct RollParams
{
    float swing = 0, humanize = 0, velDepth = 1, velRand = 0, prob = 1;
    float pitchRamp = 0, panWidth = 0, tune = 0, decay = 1, gain = 1;
    int velShape = 1, panMode = 0;      // velShape 0 flat 1 up 2 down 3 accent; panMode 0 center 1 ping-pong 2 sweep
    double speed = 1.0;                 // 0.5 / 1 / 2 (multiplies roll speed)
    bool mono = true, latch = false, choke = true;
};

struct TransportInfo
{
    double bpm = 140.0;
    double ppqAtBlockStart = 0.0;
    bool playing = false;
};

class RollEngine
{
public:
    RollEngine();
    void prepare (double sampleRate, int maxBlock);
    void reset();

    void setSample (const HatSample* s) noexcept { sample.store (s); }

    // Renders the block, handling MIDI sample-accurately. Output is added to `out` (stereo).
    void process (juce::AudioBuffer<float>& out, const juce::MidiBuffer& midi, const RollParams& p, const TransportInfo& t);

    // --- UI feedback (read from the message thread) ---
    std::atomic<int> displayNote { -1 };            // the roll currently shown in the LED
    std::atomic<float> pitchOffset { 0.0f };
    std::atomic<bool> latchToggled { false };
    static constexpr int historySize = 48;
    std::array<std::atomic<float>, historySize> history {};   // recent hit velocities (0 = gap)
    std::atomic<int> historyWrite { 0 };

    // MIDI for "DRAG MIDI": one bar of the given rate/pattern key, same swing and velocity shape as the sound.
    static juce::MidiFile renderMidi (int key, const RollParams& p, int outNote = 42);

    // KIT page: play the current sound once
    std::atomic<bool> auditionRequest { false };

private:
    struct Roll
    {
        int note = -1;
        float velocity = 1.0f;
        double startBeat = 0.0;     // engine beat when the key went down
        bool held = false;
        int order = 0;
    };
    struct Voice
    {
        bool active = false;
        int roll = -1;
        double pos = 0, inc = 1;
        int delay = 0, length = 0;
        float gL = 0, gR = 0;
        int chokeLeft = -1;          // >0 while fading out because a newer hit choked it
        int age = 0;
    };

    void noteOn (int note, float vel, double beat);
    void noteOff (int note);
    void renderSegment (juce::AudioBuffer<float>& out, int start, int num, const RollParams& p, const TransportInfo& t);
    void trigger (Roll& r, double hitBeat, int offsetSamples, const RollParams& p, int stepIndex);
    void startVoice (int rollIndex, float vel, double semis, float pan, int offset, const RollParams& p);
    bool rollActive (const Roll& r, const RollParams& p) const;
    static float shapeVelocity (const RollParams& p, double phase01, bool onBeat);

    std::atomic<const HatSample*> sample { nullptr };
    double sr = 44100.0;
    double engineBeat = 0.0;          // free-running beat clock (follows the host when it plays)
    bool lastPlaying = false;
    std::array<Roll, 16> rolls {};
    std::array<Voice, 48> voices {};
    int orderCounter = 0, ageCounter = 0;
    bool latchState = false;
    bool latchNow = false;      // effective latch for this block (parameter XOR the E1 key)
    juce::Random rng;
};
