#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "HatLibrary.h"
#include "GrooveGen.h"
#include <array>
#include <atomic>

// Key map (C3 = MIDI 60):
//   C1 (36) pitch -1 st   D1 (38) pitch +1 st   E1 (40) latch on/off
//   C2..B2 (48..59)  patterns P1..P12
//   C3..C4 (60..72)  rates 1/2 .. 1/256, triplets on the black keys
//   every other note below C2 (e.g. F#1 = 42, the note DRAG MIDI writes) plays ONE hit of the hat,
//   so the MIDI dragged out of the plugin plays the factory hats when it is put back on the plugin's channel.
// FL Studio names MIDI 60 "C5" (two octaves higher than the names above).
using Pattern = std::array<int, 16>;   // 16 steps of a 1/16 grid; the value is how many hits fall inside that step

namespace KeyMap
{
    constexpr int pitchDown = 36, pitchUp = 38, latch = 40;
    constexpr int firstPattern = 48, lastPattern = 59;
    constexpr int firstRate = 60, lastRate = 72;
    constexpr int hitNote = 42;                  // F#1, GM closed hat (one hit, no pitch)
    constexpr int hitRoot = 24;                  // C0: notes 0..35 play one hit pitched relative to C0
    constexpr int maxHitsPerStep = 6;
    inline double hitSemis (int n) { return n <= 35 ? (double) (n - hitRoot) : 0.0; }
    inline bool isRate (int n)    { return n >= firstRate && n <= lastRate; }
    inline bool isPattern (int n) { return n >= firstPattern && n <= lastPattern; }
    inline bool isHit (int n)     { return n >= 0 && n < firstPattern && n != pitchDown && n != pitchUp && n != latch; }
    double rateBeats (int note);                 // length of one step in quarter notes
    juce::String rateLabel (int note);           // "1/16T"
    juce::String factoryPatternName (int index); // 0..11
    const Pattern& factoryPattern (int index);
    juce::String keyLabel (int note);            // label drawn on the on-screen keyboard
    juce::String noteName (int note, bool flStudio);   // "C3" or, FL style, "C5"
    juce::String describe (int note);            // what a key does: "RATE 1/16", "PATTERN 3", "ONE HIT", "PITCH -1", ...

    // A new pattern. chaos 0 = steady 1/8-1/16 with one roll at the end, 1 = gaps, triplet and 1/64 bursts everywhere.
    Pattern generate (float chaos, juce::Random& rng);
    juce::String toString (const Pattern&);
    bool fromString (const juce::String&, Pattern&);
}

struct RollParams
{
    float swing = 0, humanize = 0, velDepth = 1, velRand = 0, prob = 1;
    float pitchRamp = 0, panWidth = 0, tune = 0, decay = 1, gain = 1;
    int velShape = 1, panMode = 0;      // velShape 0 flat 1 up 2 down 3 accent; panMode 0 center 1 ping-pong 2 sweep
    double speed = 1.0;                 // 0.5 / 1 / 2 (multiplies roll speed)
    bool mono = true, latch = false, choke = true;
    bool withDaw = false;               // the groove also plays while the DAW plays (off: only PLAY / space bar)
    int bars = 1;                       // groove length 1, 2 or 4 bars
};

// One hit of the groove, fully decided (swing, humanize, probability, velocity, pitch, pan are baked in),
// so the sound, DRAG MIDI and DRAG AUDIO are always the same.
struct GrooveHit
{
    double beat = 0;     // from the start of the groove
    float vel = 1, semis = 0, pan = 0;
    int beatIndex = 0, slot = 0;
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
    // Notes are F#1 (42): they play one hit each on Hat Goblin, or any GM drum kit / sampler.
    juce::MidiFile renderMidi (int key, const RollParams& p, int outNote = KeyMap::hitNote) const;

    // --- the 12 patterns of this instance (C2..B2), editable and saved with the project ---
    int  step (int pattern, int i) const noexcept   { return pats[(size_t) juce::jlimit (0, 11, pattern)][(size_t) (i & 15)].load(); }
    void setStep (int pattern, int i, int hits) noexcept
    {
        pats[(size_t) juce::jlimit (0, 11, pattern)][(size_t) (i & 15)].store (juce::jlimit (0, KeyMap::maxHitsPerStep, hits));
    }
    Pattern getPattern (int pattern) const noexcept;
    void setPattern (int pattern, const Pattern& p, const juce::String& name);   // message thread
    juce::String patternName (int pattern) const;                                // message thread
    void resetPattern (int pattern);                                             // back to the factory one
    std::atomic<int> lastIncomingNote { -1 };        // the last note-on that arrived (any key), for the UI

    // KIT page: play the current sound once
    std::atomic<bool> auditionRequest { false };

    // --- GROOVE: up to 4 bars = 16 beats. Each beat is split into 4 (1/16), 3 (1/8T) or 6 (1/16T) slots;
    //     each slot holds 0..6 hits (1 = a normal hat, 2+ = a roll inside the slot). Plays by itself with the transport.
    static constexpr int maxBeats = 16, maxSlots = 6, maxGrooveHits = maxBeats * maxSlots * KeyMap::maxHitsPerStep;
    int  slotHits (int beat, int slot) const noexcept { return groove[(size_t) juce::jlimit (0, maxBeats - 1, beat)][(size_t) juce::jlimit (0, maxSlots - 1, slot)].load(); }
    void setSlotHits (int beat, int slot, int hits) noexcept
    {
        groove[(size_t) juce::jlimit (0, maxBeats - 1, beat)][(size_t) juce::jlimit (0, maxSlots - 1, slot)].store (juce::jlimit (0, KeyMap::maxHitsPerStep, hits));
    }
    int  beatDivision (int beat) const noexcept       { return division[(size_t) juce::jlimit (0, maxBeats - 1, beat)].load(); }
    void setBeatDivision (int beat, int d) noexcept   { division[(size_t) juce::jlimit (0, maxBeats - 1, beat)].store (d == 3 || d == 6 ? d : 4); }
    void setGroove (const GrooveGen::Groove& g) noexcept;
    GrooveGen::Groove getGroove() const noexcept;
    std::atomic<juce::uint32> grooveSeed { 12345 };  // decides humanize / probability / velocity random
    static double grooveLength (const RollParams& p) { return p.bars * 4.0 / p.speed; }   // in beats
    // exports are always whole bars: at 2x the loop is repeated to fill the bar(s)
    static double exportLength (const RollParams& p) { return std::ceil (grooveLength (p) / 4.0 - 1.0e-9) * 4.0; }
    int realizeGroove (GrooveHit* out, int maxHits, const RollParams& p) const;           // returns the count
    juce::MidiFile renderGrooveMidi (const RollParams& p, int rootNote) const;            // rootNote = pitch 0

    std::atomic<bool> previewPlaying { false };      // PLAY button / space bar in the plugin (stops when the DAW starts)
    std::atomic<int>  grooveNowBeat { -1 }, grooveNowSlot { -1 };   // under the playhead, for the UI
    std::atomic<bool> pausedByMidi { false };        // notes are coming in from the DAW: they play instead of the groove

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
    std::array<std::array<std::atomic<int>, 16>, 12> pats;
    std::array<juce::String, 12> patNames;
    void hit (float vel, double semis, const RollParams& p);

    std::array<std::array<std::atomic<int>, maxSlots>, maxBeats> groove;
    std::array<std::atomic<int>, maxBeats> division;
    std::array<GrooveHit, maxGrooveHits> grooveHits;
    int grooveHitCount = 0;
    double grooveOrigin = 0.0;
    bool grooveOn = false, lastPreview = false;
    void renderGroove (int start, int num, double b0, double b1, double origin, const RollParams& p);
};
