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
    static const std::array<Pattern, 12> patterns { {
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
    juce::String factoryPatternName (int index) { return patternNames[juce::jlimit (0, 11, index)]; }
    const Pattern& factoryPattern (int index) { return patterns[(size_t) juce::jlimit (0, 11, index)]; }

    juce::String noteName (int note, bool fl)
    {
        return juce::MidiMessage::getMidiNoteName (note, true, true, fl ? 5 : 3);
    }

    juce::String describe (int note)
    {
        if (note < 0) return {};
        if (note == pitchDown) return "PITCH -1 ST";
        if (note == pitchUp)   return "PITCH +1 ST";
        if (note == latch)     return "LATCH ON/OFF";
        if (isPattern (note))  return "PATTERN " + juce::String (note - firstPattern + 1);
        if (isRate (note))     return "RATE " + rateLabel (note);
        if (isHit (note))      return "ONE HIT";
        return "NOTHING ON THIS KEY";
    }

    juce::String toString (const Pattern& p)
    {
        juce::String s;
        for (auto v : p) s << v;
        return s;
    }

    bool fromString (const juce::String& s, Pattern& p)
    {
        if (s.length() != 16) return false;
        for (int i = 0; i < 16; ++i)
        {
            const int v = s[i] - '0';
            if (v < 0 || v > maxHitsPerStep) return false;
            p[(size_t) i] = v;
        }
        return true;
    }

    Pattern generate (float chaos, juce::Random& rng)
    {
        const float c = juce::jlimit (0.0f, 1.0f, chaos);
        auto chance = [&] (float pr) { return rng.nextFloat() < pr; };
        Pattern s {};

        // backbone: 1/8s always, 1/16 fills more often with chaos
        for (int i = 0; i < 16; ++i)
            s[(size_t) i] = (i % 2 == 0) ? 1 : (chance (0.2f + 0.45f * c) ? 1 : 0);

        // gaps off the beat
        for (int i = 0; i < 16; ++i)
            if (i % 4 != 0 && chance (0.06f + 0.24f * c)) s[(size_t) i] = 0;

        // rolls: short bursts of 2, 3, 4 or 6 hits per step
        const int bursts = 1 + (int) std::round (c * 2.5f) + (chance (0.35f) ? 1 : 0);
        for (int b = 0; b < bursts; ++b)
        {
            const int len = 1 + rng.nextInt (1 + (int) std::round (c * 3.0f));   // 1..4 steps
            int start;
            if (b == 0 && chance (0.75f - 0.45f * c)) start = 16 - len - rng.nextInt (juce::jmin (3, 16 - len + 1)) * 2;   // classic: roll into the next bar
            else start = rng.nextInt (16 - len + 1);
            start = juce::jlimit (0, 16 - len, start);

            static const int calm[] = { 2, 3, 4 }, wild[] = { 3, 4, 6 };
            const int* opts = chance (c) ? wild : calm;
            const int base = opts[rng.nextInt (3)];
            const bool ramp = len > 1 && chance (0.5f);
            for (int k = 0; k < len; ++k)
            {
                int v = base;
                if (ramp) v = juce::jlimit (2, maxHitsPerStep, base - (len - 1 - k));   // accelerate into the end
                s[(size_t) (start + k)] = v;
            }
            if (chance (0.4f) && start + len < 16) s[(size_t) (start + len)] = 0;   // breath after the roll
        }

        if (s[0] == 0) s[0] = 1;
        return s;
    }

    juce::String keyLabel (int note)
    {
        if (note == pitchDown) return "P-";
        if (note == pitchUp)   return "P+";
        if (note == latch)     return "LT";
        if (isPattern (note))  return "P" + juce::String (note - firstPattern + 1);
        if (isRate (note))     return rateLabel (note);
        if (note == hitNote)   return "HIT";
        return {};
    }
}

RollEngine::RollEngine()
{
    for (auto& h : history) h.store (0.0f);
    for (int i = 0; i < 12; ++i) resetPattern (i);
    setGroove (GrooveGen::preset (6, 1));   // END ROLL 01
}

void RollEngine::setGroove (const GrooveGen::Groove& g) noexcept
{
    for (int b = 0; b < maxBeats; ++b)
    {
        setBeatDivision (b, g.div[(size_t) b]);
        for (int k = 0; k < maxSlots; ++k) setSlotHits (b, k, g.hits[(size_t) b][(size_t) k]);
    }
}

GrooveGen::Groove RollEngine::getGroove() const noexcept
{
    GrooveGen::Groove g;
    for (int b = 0; b < maxBeats; ++b)
    {
        g.div[(size_t) b] = beatDivision (b);
        for (int k = 0; k < maxSlots; ++k) g.hits[(size_t) b][(size_t) k] = slotHits (b, k);
    }
    return g;
}

int RollEngine::realizeGroove (GrooveHit* out, int maxOut, const RollParams& p) const
{
    juce::Random r ((juce::int64) grooveSeed.load());
    const int beats = juce::jlimit (1, maxBeats, p.bars * 4);
    const double scale = 1.0 / p.speed;
    int count = 0;

    // runs of consecutive rolled slots: pitch ramps and "up/down" velocity shapes go across a whole run
    struct Slot { int b, k, d, n; };
    std::array<Slot, maxBeats * maxSlots> slots {};
    int ns = 0;
    for (int b = 0; b < beats; ++b)
    {
        const int d = beatDivision (b);
        for (int k = 0; k < d; ++k) slots[(size_t) ns++] = { b, k, d, slotHits (b, k) };
    }
    std::array<int, maxBeats * maxSlots> runStart {}, runLen {};
    for (int i = 0; i < ns; )
    {
        if (slots[(size_t) i].n < 2) { runStart[(size_t) i] = i; runLen[(size_t) i] = 1; ++i; continue; }
        int j = i;
        int hitsInRun = 0;
        while (j < ns && slots[(size_t) j].n >= 2) hitsInRun += slots[(size_t) j++].n;
        for (int k = i; k < j; ++k) { runStart[(size_t) k] = i; runLen[(size_t) k] = hitsInRun; }
        i = j;
    }

    int hitIndex = 0;
    for (int i = 0; i < ns; ++i)
    {
        const auto& sl = slots[(size_t) i];
        const double slotLen = 1.0 / sl.d;
        double t0 = sl.b + sl.k * slotLen;
        if (sl.d == 4 && (sl.k & 1)) t0 += p.swing * slotLen * 0.5;

        float base = 0.66f;                                  // dynamics of the grid itself
        if (sl.k == 0) base = 1.0f;
        else if (sl.d == 4 && sl.k == 2) base = 0.82f;
        else if (sl.d != 4) base = 0.74f;

        // position inside the run (for rolls)
        int before = 0;
        for (int k = runStart[(size_t) i]; k < i; ++k) before += slots[(size_t) k].n;
        const int runHits = runLen[(size_t) i];

        for (int j = 0; j < sl.n; ++j)
        {
            // always draw the same numbers so changing a knob never reshuffles the groove
            const float rProb = r.nextFloat(), rTime = r.nextFloat(), rVel = r.nextFloat(), rHum = r.nextFloat();
            if (count >= maxOut) return count;
            const bool downbeat = sl.k == 0 && j == 0;
            if (! downbeat && rProb > p.prob) { ++hitIndex; continue; }

            double t = t0 + j * slotLen / sl.n;
            t += p.humanize * 0.012 * rTime;

            float v = base;
            const bool rolled = sl.n >= 2;
            const float pos = runHits > 1 ? (float) (before + j) / (float) (runHits - 1) : 0.0f;
            if (rolled)
            {
                switch (p.velShape)
                {
                    case 1: v = 0.42f + 0.58f * pos; break;                    // roll swells up
                    case 2: v = 1.0f - 0.55f * pos; break;                     // roll fades
                    case 3: v = j == 0 ? 0.95f : 0.55f; break;                 // accent each slot
                    default: v = 0.8f; break;                                  // flat
                }
            }
            v = 1.0f - p.velDepth * (1.0f - v);
            v *= 1.0f - p.velRand * 0.5f * rVel;
            v *= 1.0f - p.humanize * 0.15f * rHum;

            GrooveHit h;
            h.beat = t * scale;
            h.vel = juce::jlimit (0.05f, 1.0f, v);
            h.semis = rolled ? p.pitchRamp * pos : 0.0f;
            if (p.panMode == 1) h.pan = ((hitIndex & 1) ? 1.0f : -1.0f) * p.panWidth;
            if (p.panMode == 2) h.pan = (float) std::sin (juce::MathConstants<double>::twoPi * t / 4.0) * p.panWidth;
            h.beat = juce::jmax (0.0, h.beat);
            h.slot = sl.k; h.beatIndex = sl.b;
            out[count] = h;
            ++count;
            ++hitIndex;
        }
    }
    return count;
}

juce::MidiFile RollEngine::renderGrooveMidi (const RollParams& p, int rootNote) const
{
    constexpr int tpq = 960;
    std::vector<GrooveHit> one ((size_t) maxGrooveHits);
    const int n = realizeGroove (one.data(), maxGrooveHits, p);
    const double loop = grooveLength (p), total = exportLength (p);

    // repeat the loop until the export is whole bars long
    std::vector<GrooveHit> hits;
    for (double start = 0.0; start < total - 1.0e-9; start += loop)
        for (int i = 0; i < n; ++i)
        {
            auto h = one[(size_t) i];
            h.beat += start;
            if (h.beat < total - 1.0e-9) hits.push_back (h);
        }

    juce::MidiMessageSequence seq;
    for (size_t i = 0; i < hits.size(); ++i)
    {
        const double beat = hits[i].beat;
        const double next = i + 1 < hits.size() ? hits[i + 1].beat : total;
        const double dur = juce::jlimit (1.0 / 128.0, 0.25, (next - beat) * 0.85);
        const int note = juce::jlimit (0, 127, rootNote + (int) std::round (hits[i].semis));
        const int vel = juce::jlimit (1, 127, (int) std::round (127.0f * hits[i].vel));
        seq.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) vel), beat * tpq);
        seq.addEvent (juce::MidiMessage::noteOff (1, note), (beat + dur) * tpq);
    }
    seq.updateMatchedPairs();
    juce::MidiFile file;
    file.setTicksPerQuarterNote (tpq);
    file.addTrack (seq);
    return file;
}

void RollEngine::renderGroove (int start, int num, double b0, double b1, double origin, const RollParams& p)
{
    const double L = grooveLength (p);
    if (L <= 0.0) return;
    const double bps = (b1 - b0) / juce::jmax (1, num);
    for (int i = 0; i < grooveHitCount; ++i)
    {
        const auto& h = grooveHits[(size_t) i];
        double k = std::ceil ((b0 - origin - h.beat) / L - 1.0e-9);
        for (;; k += 1.0)
        {
            const double t = origin + h.beat + k * L;
            if (t >= b1) break;
            if (t < b0) continue;
            const int off = juce::jlimit (0, num - 1, (int) ((t - b0) / bps));
            if (p.choke)
                for (auto& v : voices)
                    if (v.active && v.roll == -3 && v.chokeLeft < 0)
                        v.chokeLeft = juce::jmax (1, (int) (0.003 * sr)) + off;
            startVoice (-3, h.vel, p.tune + pitchOffset.load() + h.semis, h.pan, off, p);
            auto w = historyWrite.load();
            history[(size_t) w].store (h.vel);
            historyWrite.store ((w + 1) % historySize);
        }
    }
    juce::ignoreUnused (start);
}

Pattern RollEngine::getPattern (int pattern) const noexcept
{
    Pattern p {};
    for (int i = 0; i < 16; ++i) p[(size_t) i] = step (pattern, i);
    return p;
}

void RollEngine::setPattern (int pattern, const Pattern& p, const juce::String& name)
{
    pattern = juce::jlimit (0, 11, pattern);
    for (int i = 0; i < 16; ++i) setStep (pattern, i, p[(size_t) i]);
    patNames[(size_t) pattern] = name;
}

juce::String RollEngine::patternName (int pattern) const { return patNames[(size_t) juce::jlimit (0, 11, pattern)]; }

void RollEngine::resetPattern (int pattern)
{
    setPattern (pattern, KeyMap::factoryPattern (pattern), KeyMap::factoryPatternName (pattern));
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
    if (! KeyMap::isRate (note) && ! KeyMap::isPattern (note)) return;   // single hits are handled in process()

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
            const int pi = r.note - KeyMap::firstPattern;
            const double stepLen = 0.25 / p.speed;
            const double swingShift = p.swing * stepLen * 0.5;
            long long s = (long long) std::floor ((lo - origin) / stepLen) - 1;
            if (s < 0) s = 0;
            for (;; ++s)
            {
                const double base = origin + (double) s * stepLen + ((s & 1) ? swingShift : 0.0);
                if (base >= b1) break;
                const int n = step (pi, (int) (s % 16));
                for (int j = 0; j < n; ++j)
                    fire (base + j * stepLen / n, (int) s * 8 + j);
            }
        }
    }
    displayNote.store (shown);
    if (grooveOn) renderGroove (start, num, b0, b1, grooveOrigin, p);

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
    if (lastPlaying && ! t.playing) pausedByMidi.store (false);      // transport stopped: groove is back
    if (t.playing) previewPlaying.store (false);                     // the DAW takes over from the PLAY button
    const bool preview = previewPlaying.load();
    if (preview && ! lastPreview) { grooveOrigin = engineBeat; pausedByMidi.store (false); }
    lastPreview = preview;
    if (t.playing) grooveOrigin = 0.0;                               // bar 1 of the DAW = start of the groove
    grooveOn = preview || (p.withDaw && t.playing && ! pausedByMidi.load());
    if (grooveOn) grooveHitCount = realizeGroove (grooveHits.data(), maxGrooveHits, p);
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
        if (msg.isNoteOn()) lastIncomingNote.store (msg.getNoteNumber());
        if (msg.isNoteOn() && t.playing) pausedByMidi.store (true);   // notes from the DAW take over from the groove
        // every note plays one hit, pitched like a sampler: C5 (FL) = 60 = original pitch.
        // So the MIDI from DRAG MIDI plays the same on this plugin and on any sampler.
        if (msg.isNoteOn()) hit (msg.getFloatVelocity(), (double) (msg.getNoteNumber() - 60), p);
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) reset();
    }
    renderSegment (out, pos, n - pos, p, t);

    if (grooveOn)
    {
        const double L = grooveLength (p);
        double ph = std::fmod (engineBeat - grooveOrigin, L);
        if (ph < 0) ph += L;
        const double gb = ph * p.speed;
        const int b = juce::jlimit (0, maxBeats - 1, (int) gb);
        grooveNowBeat.store (b);
        grooveNowSlot.store ((int) ((gb - b) * beatDivision (b)));
    }
    else
    {
        grooveNowBeat.store (-1);
        grooveNowSlot.store (-1);
    }
}

void RollEngine::hit (float vel, double semis, const RollParams& p)
{
    auto* s = sample.load();
    if (s == nullptr || s->audio.getNumSamples() < 2) return;
    if (p.choke)
        for (auto& v : voices)
            if (v.active && v.roll == -2 && v.chokeLeft < 0)
                v.chokeLeft = juce::jmax (1, (int) (0.003 * sr));
    vel = juce::jlimit (0.0f, 1.0f, vel);
    startVoice (-2, vel, p.tune + pitchOffset.load() + semis, 0.0f, 0, p);
    auto w = historyWrite.load();
    history[(size_t) w].store (vel);
    historyWrite.store ((w + 1) % historySize);
}

juce::MidiFile RollEngine::renderMidi (int key, const RollParams& p, int outNote) const
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
        const auto pat = getPattern (key - KeyMap::firstPattern);
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
