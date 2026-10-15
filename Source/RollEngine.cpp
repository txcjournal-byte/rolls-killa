#include "RollEngine.h"
#include <cmath>

namespace KeyMap
{
    // one step length in quarter notes, for MIDI notes 60..72
    static const double rates[13] = { 2.0, 2.0 / 3.0, 1.0, 1.0 / 3.0, 0.5, 0.25, 1.0 / 6.0,
                                      0.125, 1.0 / 12.0, 1.0 / 16.0, 1.0 / 24.0, 1.0 / 32.0, 1.0 / 64.0 };
    static const char* rateNames[13] = { "1/2", "1/4T", "1/4", "1/8T", "1/8", "1/16", "1/16T",
                                         "1/32", "1/32T", "1/64", "1/64T", "1/128", "1/256" };

    // 16 steps of a 1/16 grid; the value is how many hits fall inside that step (0 = rest)
    static const std::array<std::array<int, 16>, 12> patterns { {
        { 1,0,1,0, 1,0,1,0, 1,0,1,0, 1,0,3,0 },   // FLICK 3
        { 1,0,1,0, 1,1,1,1, 2,2,2,2, 3,3,4,4 },   // RAMP UP
        { 4,4,3,3, 2,2,2,2, 1,1,1,1, 1,0,1,0 },   // RAMP DN
        { 1,0,1,1, 1,0,1,1, 1,0,1,1, 1,0,1,1 },   // GALLOP
        { 1,0,0,0, 4,0,0,0, 1,0,0,0, 4,0,2,0 },   // STUTTER
        { 1,0,1,0, 1,0,0,1, 0,1,1,0, 1,0,2,0 },   // SKIP
        { 2,0,1,0, 2,0,1,0, 2,0,1,0, 2,0,1,0 },   // FLAM 32
        { 0,1,0,1, 0,1,0,1, 0,1,0,1, 0,1,0,1 },   // OFF 16
        { 1,0,1,0, 1,0,1,0, 4,4,4,4, 1,0,1,0 },   // RATCHET
        { 3,0,0,1, 0,2,0,0, 1,0,4,0, 0,1,3,0 },   // GLITCH
        { 1,0,0,1, 0,0,1,0, 1,0,0,1, 0,0,2,2 },   // BOUNCE
        { 1,0,1,0, 1,0,1,0, 1,0,1,0, 5,0,5,0 },   // ROLL 5
    } };
    static const char* patternNames[12] = { "FLICK 3", "RAMP UP", "RAMP DN", "GALLOP", "STUTTER", "SKIP",
                                            "FLAM 32", "OFF 16", "RATCHET", "GLITCH", "BOUNCE", "ROLL 5" };

    double rateBeats (int note)          { return rates[juce::jlimit (0, 12, note - firstRate)]; }
    juce::String rateLabel (int note)    { return rateNames[juce::jlimit (0, 12, note - firstRate)]; }
    juce::String patternName (int index) { return patternNames[juce::jlimit (0, 11, index)]; }
    const std::array<int, 16>& pattern (int index) { return patterns[(size_t) juce::jlimit (0, 11, index)]; }

    juce::String keyLabel (int note)
    {
        if (note == pitchDown) return "P-";
        if (note == pitchUp)   return "P+";
        if (note == latch)     return "LT";
        if (isPattern (note))  return "P" + juce::String (note - firstPattern + 1);
        if (isRate (note))     return rateLabel (note);
        return {};
    }
}

RollEngine::RollEngine()
{
    for (auto& h : history) h.store (0.0f);
}

void RollEngine::prepare (double sampleRate, int)
{
    sr = sampleRate > 0 ? sampleRate : 44100.0;
    reset();
}

void RollEngine::reset()
{
    for (auto& r : rolls) r = Roll{};
    for (auto& v : voices) v = Voice{};
    displayNote.store (-1);
}

bool RollEngine::rollActive (const Roll& r, const RollParams& p) const
{
    juce::ignoreUnused (p);
    if (r.note < 0) return false;
    if (! r.held && ! latchNow) return false;
    if (p.mono)
    {
        // only the most recent roll plays
        for (auto& o : rolls)
            if (o.note >= 0 && (o.held || latchNow) && o.order > r.order)
                return false;
    }
    return true;
}

void RollEngine::noteOn (int note, float vel, double beat)
{
    if (note == KeyMap::pitchDown) { pitchOffset.store (juce::jmax (-24.0f, pitchOffset.load() - 1.0f)); return; }
    if (note == KeyMap::pitchUp)   { pitchOffset.store (juce::jmin ( 24.0f, pitchOffset.load() + 1.0f)); return; }
    if (note == KeyMap::latch)     { latchState = ! latchState; latchToggled.store (latchState); return; }
    if (! KeyMap::isRate (note) && ! KeyMap::isPattern (note)) return;

    // pressing a latched roll again stops it
    for (auto& r : rolls)
        if (r.note == note && ! r.held)
        {
            r = Roll{};
            if (latchNow) return;
        }

    Roll* slot = nullptr;
    for (auto& r : rolls) if (r.note < 0) { slot = &r; break; }
    if (slot == nullptr)
    {
        slot = &rolls[0];
        for (auto& r : rolls) if (r.order < slot->order) slot = &r;
    }
    slot->note = note;
    slot->velocity = juce::jlimit (0.05f, 1.0f, vel);
    slot->startBeat = beat;
    slot->held = true;
    slot->order = ++orderCounter;
}

void RollEngine::noteOff (int note)
{
    for (auto& r : rolls)
        if (r.note == note && r.held)
        {
            if (latchNow) r.held = false;   // a latched roll keeps going until it is pressed again
            else r = Roll{};
        }
}

float RollEngine::shapeVelocity (const RollParams& p, double phase01, bool onBeat)
{
    float v = 1.0f;
    switch (p.velShape)
    {
        case 1: v = 0.3f + 0.7f * (float) phase01; break;     // ramp up
        case 2: v = 1.0f - 0.7f * (float) phase01; break;     // ramp down
        case 3: v = onBeat ? 1.0f : 0.55f; break;            // accent on the beat
        default: break;                                      // flat
    }
    return juce::jmap (p.velDepth, 1.0f, v);
}

void RollEngine::trigger (Roll& r, double hitBeat, int offset, const RollParams& p, int stepIndex)
{
    auto* s = sample.load();
    if (s == nullptr || s->audio.getNumSamples() < 2) return;
    if (p.prob < 0.999f && rng.nextFloat() > p.prob) return;

    const bool isPattern = KeyMap::isPattern (r.note);
    const double phraseBeats = isPattern ? 4.0 / p.speed : 1.0;
    const double phase = std::fmod (juce::jmax (0.0, hitBeat - r.startBeat), phraseBeats) / phraseBeats;
    const bool onBeat = std::abs (hitBeat - std::round (hitBeat)) < 1.0e-6;

    float vel = shapeVelocity (p, phase, onBeat) * r.velocity;
    vel *= 1.0f - p.velRand * rng.nextFloat();
    vel *= 1.0f - p.humanize * 0.2f * rng.nextFloat();
    vel = juce::jlimit (0.0f, 1.0f, vel);

    const int delay = (int) (p.humanize * 0.010 * sr * rng.nextFloat());
    const double semis = p.tune + pitchOffset.load() + p.pitchRamp * phase;

    float pan = 0.0f;
    if (p.panMode == 1) pan = ((stepIndex & 1) ? 1.0f : -1.0f) * p.panWidth;
    if (p.panMode == 2) pan = (float) std::sin (juce::MathConstants<double>::twoPi * phase) * p.panWidth;

    const int rollIndex = (int) (&r - rolls.data());
    if (p.choke)
        for (auto& v : voices)
            if (v.active && v.roll == rollIndex && v.chokeLeft < 0)
                v.chokeLeft = juce::jmax (1, (int) (0.003 * sr)) + delay + offset;

    startVoice (rollIndex, vel, semis, pan, offset + delay, p);

    auto w = historyWrite.load();
    history[(size_t) w].store (vel);
    historyWrite.store ((w + 1) % historySize);
}

void RollEngine::startVoice (int rollIndex, float vel, double semis, float pan, int offset, const RollParams& p)
{
    auto* s = sample.load();
    if (s == nullptr || s->audio.getNumSamples() < 2) return;
    Voice* slot = nullptr;
    for (auto& v : voices) if (! v.active) { slot = &v; break; }
    if (slot == nullptr)
    {
        slot = &voices[0];
        for (auto& v : voices) if (v.age < slot->age) slot = &v;
    }
    const float angle = (juce::jlimit (-1.0f, 1.0f, pan) + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
    const int len = s->audio.getNumSamples();
    slot->active = true;
    slot->roll = rollIndex;
    slot->pos = 0.0;
    slot->inc = std::pow (2.0, semis / 12.0) * s->sampleRate / sr;
    slot->delay = juce::jmax (0, offset);
    slot->length = juce::jmax (8, (int) (len * juce::jlimit (0.05f, 1.0f, p.decay)));
    slot->gL = vel * p.gain * std::cos (angle) * 1.41421356f;
    slot->gR = vel * p.gain * std::sin (angle) * 1.41421356f;
    slot->chokeLeft = -1;
    slot->age = ++ageCounter;
}

void RollEngine::renderSegment (juce::AudioBuffer<float>& out, int start, int num, const RollParams& p, const TransportInfo& t)
{
    if (num <= 0) return;
    const double bps = t.bpm / 60.0 / sr;      // beats per sample
    const double b0 = engineBeat, b1 = engineBeat + num * bps;

    int shown = -1, shownOrder = -1;
    for (auto& r : rolls)
    {
        if (! rollActive (r, p)) continue;
        if (r.order > shownOrder) { shown = r.note; shownOrder = r.order; }

        const double origin = t.playing ? 0.0 : r.startBeat;
        const double lo = juce::jmax (b0, r.startBeat - 1.0e-9);

        auto fire = [&] (double hitBeat, int stepIndex)
        {
            if (hitBeat < lo || hitBeat >= b1) return;
            int off = start + (int) ((hitBeat - b0) / bps);
            off = juce::jlimit (start, start + num - 1, off);
            trigger (r, hitBeat, off - start, p, stepIndex);
        };

        if (KeyMap::isRate (r.note))
        {
            const double step = KeyMap::rateBeats (r.note) / p.speed;
            const double swingShift = p.swing * step * 0.5;
            long long k = (long long) std::floor ((lo - origin) / step) - 1;
            if (k < 0) k = 0;
            for (;; ++k)
            {
                const double base = origin + (double) k * step;
                if (base >= b1) break;
                fire (base + ((k & 1) ? swingShift : 0.0), (int) k);
            }
        }
        else if (KeyMap::isPattern (r.note))
        {
            const auto& pat = KeyMap::pattern (r.note - KeyMap::firstPattern);
            const double stepLen = 0.25 / p.speed;
            const double swingShift = p.swing * stepLen * 0.5;
            long long s = (long long) std::floor ((lo - origin) / stepLen) - 1;
            if (s < 0) s = 0;
            for (;; ++s)
            {
                const double base = origin + (double) s * stepLen + ((s & 1) ? swingShift : 0.0);
                if (base >= b1) break;
                const int n = pat[(size_t) (s % 16)];
                for (int j = 0; j < n; ++j)
                    fire (base + j * stepLen / n, (int) s * 8 + j);
            }
        }
    }
    displayNote.store (shown);

    // voices
    auto* s = sample.load();
    float* L = out.getWritePointer (0);
    float* R = out.getNumChannels() > 1 ? out.getWritePointer (1) : nullptr;
    const int chokeLen = juce::jmax (1, (int) (0.003 * sr));
    for (auto& v : voices)
    {
        if (! v.active) continue;
        if (s == nullptr) { v.active = false; continue; }
        const float* src = s->audio.getReadPointer (0);
        const int len = juce::jmin (v.length, s->audio.getNumSamples() - 1);
        const double fadeStart = len * 0.8;
        for (int i = start; i < start + num; ++i)
        {
            if (v.chokeLeft == 0) { v.active = false; break; }
            float g = 1.0f;
            if (v.chokeLeft > 0)
            {
                --v.chokeLeft;
                if (v.chokeLeft < chokeLen) g = (float) v.chokeLeft / (float) chokeLen;
            }
            if (v.delay > 0) { --v.delay; continue; }
            const int idx = (int) v.pos;
            if (idx >= len) { v.active = false; break; }
            const float frac = (float) (v.pos - idx);
            float x = src[idx] + frac * (src[idx + 1] - src[idx]);
            if (v.pos > fadeStart) g *= (float) ((len - v.pos) / (len - fadeStart));
            x *= g;
            L[i] += x * v.gL;
            if (R != nullptr) R[i] += x * v.gR;
            v.pos += v.inc;
        }
    }

    engineBeat = b1;
}

void RollEngine::process (juce::AudioBuffer<float>& out, const juce::MidiBuffer& midi, const RollParams& p, const TransportInfo& t)
{
    // follow the host clock while it plays; re-anchor the rolls when it jumps (loop, transport start)
    if (t.playing)
    {
        if (! lastPlaying || std::abs (t.ppqAtBlockStart - engineBeat) > 0.05)
            for (auto& r : rolls) if (r.note >= 0) r.startBeat = t.ppqAtBlockStart;
        engineBeat = t.ppqAtBlockStart;
    }
    lastPlaying = t.playing;
    latchNow = p.latch != latchState;
    if (! latchNow)
        for (auto& r : rolls) if (r.note >= 0 && ! r.held) r = Roll{};   // latch switched off: released rolls stop
    if (auditionRequest.exchange (false))
        startVoice (-1, 0.9f, p.tune + pitchOffset.load(), 0.0f, 0, p);

    int pos = 0;
    const int n = out.getNumSamples();
    for (const auto meta : midi)
    {
        const int at = juce::jlimit (0, n, meta.samplePosition);
        renderSegment (out, pos, at - pos, p, t);
        pos = at;
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())       noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), engineBeat);
        else if (msg.isNoteOff()) noteOff (msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) reset();
    }
    renderSegment (out, pos, n - pos, p, t);
}

juce::MidiFile RollEngine::renderMidi (int key, const RollParams& p, int outNote)
{
    constexpr int tpq = 960;
    juce::MidiMessageSequence seq;
    const double bar = 4.0;
    std::vector<std::pair<double, int>> hits;   // beat, step index

    if (KeyMap::isRate (key))
    {
        const double step = KeyMap::rateBeats (key) / p.speed;
        for (int k = 0; k * step < bar - 1.0e-9; ++k)
            hits.push_back ({ k * step + ((k & 1) ? p.swing * step * 0.5 : 0.0), k });
    }
    else if (KeyMap::isPattern (key))
    {
        const auto& pat = KeyMap::pattern (key - KeyMap::firstPattern);
        const double stepLen = 0.25 / p.speed;
        for (int s = 0; s * stepLen < bar - 1.0e-9; ++s)
        {
            const int n = pat[(size_t) (s % 16)];
            for (int j = 0; j < n; ++j)
                hits.push_back ({ s * stepLen + ((s & 1) ? p.swing * stepLen * 0.5 : 0.0) + j * stepLen / n, s * 8 + j });
        }
    }

    const double phrase = KeyMap::isPattern (key) ? 4.0 / p.speed : 1.0;
    for (size_t i = 0; i < hits.size(); ++i)
    {
        const double beat = hits[i].first;
        const double next = i + 1 < hits.size() ? hits[i + 1].first : bar;
        const double len = juce::jlimit (1.0 / 64.0, 0.25, (next - beat) * 0.8);
        const double phase = std::fmod (beat, phrase) / phrase;
        const bool onBeat = std::abs (beat - std::round (beat)) < 1.0e-6;
        const int vel = juce::jlimit (1, 127, (int) std::round (127.0f * shapeVelocity (p, phase, onBeat)));
        seq.addEvent (juce::MidiMessage::noteOn (1, outNote, (juce::uint8) vel), beat * tpq);
        seq.addEvent (juce::MidiMessage::noteOff (1, outNote), (beat + len) * tpq);
    }
    seq.updateMatchedPairs();
    juce::MidiFile file;
    file.setTicksPerQuarterNote (tpq);
    file.addTrack (seq);
    return file;
}
