#include "DrumKit.h"

namespace rk
{

namespace
{
    void writeShape (juce::ValueTree& v, const DrumShape& s)
    {
        v.setProperty ("tune", s.tune, nullptr);
        v.setProperty ("decay", s.decay, nullptr);
        v.setProperty ("punch", s.punch, nullptr);
        v.setProperty ("drive", s.drive, nullptr);
        v.setProperty ("tone", s.tone, nullptr);
        v.setProperty ("body", s.body, nullptr);
    }

    DrumShape readShape (const juce::ValueTree& v)
    {
        DrumShape s;
        s.tune = juce::jlimit (-12.0f, 12.0f, (float) v.getProperty ("tune", 0.0f));
        s.decay = juce::jlimit (0.0f, 1.0f, (float) v.getProperty ("decay", 0.5f));
        s.punch = juce::jlimit (0.0f, 1.0f, (float) v.getProperty ("punch", 0.5f));
        s.drive = juce::jlimit (0.0f, 1.0f, (float) v.getProperty ("drive", 0.0f));
        s.tone = juce::jlimit (0.0f, 1.0f, (float) v.getProperty ("tone", 0.5f));
        s.body = juce::jlimit (0.0f, 1.0f, (float) v.getProperty ("body", 0.5f));
        return s;
    }

    juce::uint32 readSeed (const juce::ValueTree& v, const char* id, juce::uint32 fallback)
    {
        return (juce::uint32) (juce::int64) v.getProperty (id, (juce::int64) fallback);
    }

    bool sameShape (const DrumShape& a, const DrumShape& b)
    {
        return juce::approximatelyEqual (a.tune, b.tune) && juce::approximatelyEqual (a.decay, b.decay)
            && juce::approximatelyEqual (a.punch, b.punch) && juce::approximatelyEqual (a.drive, b.drive)
            && juce::approximatelyEqual (a.tone, b.tone) && juce::approximatelyEqual (a.body, b.body);
    }
}

DrumKit::DrumKit()
{
    for (int i = 0; i < kNumDrumTypes; ++i)
    {
        slots[(size_t) i].seed = (juce::uint32) (101 * i + 7);
        slots[(size_t) i].patternSeed = (juce::uint32) (37 * i + 3);
    }
    // mix-ready defaults (every one-shot is normalised to -0.5 dBFS; together they must not clip)
    const float volumes[] { -8.0f, -7.0f, -10.0f, -10.0f, -13.0f, -15.0f, -13.0f, -13.0f };
    for (int i = 0; i < kNumDrumTypes; ++i)
        slots[(size_t) i].volumeDb = volumes[i];
}

DrumSoundPtr DrumKit::renderSlot (DrumType t, double sampleRate, juce::String& error) const
{
    const auto& s = slot (t);
    if (s.file.isNotEmpty())
    {
        if (auto snd = loadDrumFile (t, juce::File (s.file), s.shape, sampleRate, error))
            return snd;
        // the file is gone (other PC?) - fall back to the synth so the kit still plays
    }
    return synthesizeDrum (t, s.seed, s.mood, s.shape, sampleRate);
}

DrumSoundPtr DrumKit::renderKept (const KeptSound& k, double sampleRate, juce::String& error)
{
    if (k.file.isNotEmpty())
        if (auto snd = loadDrumFile (k.type, juce::File (k.file), k.shape, sampleRate, error))
            return snd;
    return synthesizeDrum (k.type, k.seed, k.mood, k.shape, sampleRate);
}

std::vector<DrumHit> DrumKit::pattern (DrumType t) const
{
    if (t == DrumType::hat)
        return {};
    const auto& s = slot (t);
    return generateDrumPattern (t, s.patternSeed, s.patternStyle, s.patternBars, s.patternDensity);
}

bool DrumKit::keep (DrumType t, const juce::String& soundName)
{
    const auto& s = slot (t);
    for (const auto& k : kept)
        if (k.type == t && k.seed == s.seed && k.mood == s.mood && k.file == s.file && sameShape (k.shape, s.shape))
            return false;
    kept.push_back ({ t, s.seed, s.mood, s.shape, s.file, soundName });
    return true;
}

int DrumKit::numKept (DrumType t) const
{
    int n = 0;
    for (const auto& k : kept)
        n += k.type == t ? 1 : 0;
    return n;
}

juce::ValueTree DrumKit::toValueTree() const
{
    juce::ValueTree v (kTreeType);
    v.setProperty ("name", name, nullptr);
    for (int i = 0; i < kNumDrumTypes; ++i)
    {
        const auto& s = slots[(size_t) i];
        juce::ValueTree c ("SLOT");
        c.setProperty ("type", i, nullptr);
        c.setProperty ("seed", (juce::int64) s.seed, nullptr);
        c.setProperty ("mood", s.mood, nullptr);
        writeShape (c, s.shape);
        c.setProperty ("file", s.file, nullptr);
        c.setProperty ("volume", s.volumeDb, nullptr);
        c.setProperty ("muted", s.muted, nullptr);
        c.setProperty ("patOn", s.patternOn, nullptr);
        c.setProperty ("patSeed", (juce::int64) s.patternSeed, nullptr);
        c.setProperty ("patStyle", s.patternStyle, nullptr);
        c.setProperty ("patBars", s.patternBars, nullptr);
        c.setProperty ("patDensity", s.patternDensity, nullptr);
        v.appendChild (c, nullptr);
    }
    for (const auto& k : kept)
    {
        juce::ValueTree c ("KEPT");
        c.setProperty ("type", (int) k.type, nullptr);
        c.setProperty ("seed", (juce::int64) k.seed, nullptr);
        c.setProperty ("mood", k.mood, nullptr);
        writeShape (c, k.shape);
        c.setProperty ("file", k.file, nullptr);
        c.setProperty ("name", k.name, nullptr);
        v.appendChild (c, nullptr);
    }
    return v;
}

void DrumKit::fromValueTree (const juce::ValueTree& v)
{
    *this = DrumKit();
    if (! v.hasType (kTreeType))
        return;
    name = v.getProperty ("name", name).toString();
    for (const auto& c : v)
    {
        const auto type = drumTypeFromIndex ((int) c.getProperty ("type", 0));
        if (c.hasType ("SLOT"))
        {
            auto& s = slot (type);
            s.seed = readSeed (c, "seed", s.seed);
            s.mood = juce::jlimit (0, 2, (int) c.getProperty ("mood", 1));
            s.shape = readShape (c);
            s.file = c.getProperty ("file").toString();
            s.volumeDb = juce::jlimit (-48.0f, 6.0f, (float) c.getProperty ("volume", 0.0f));
            s.muted = (bool) c.getProperty ("muted", false);
            s.patternOn = (bool) c.getProperty ("patOn", false);
            s.patternSeed = readSeed (c, "patSeed", s.patternSeed);
            s.patternStyle = juce::jlimit (0, 3, (int) c.getProperty ("patStyle", 0));
            s.patternBars = juce::jlimit (1, 8, (int) c.getProperty ("patBars", 2));
            s.patternDensity = juce::jlimit (0.0f, 1.0f, (float) c.getProperty ("patDensity", 0.5f));
        }
        else if (c.hasType ("KEPT"))
        {
            KeptSound k;
            k.type = type;
            k.seed = readSeed (c, "seed", 1);
            k.mood = juce::jlimit (0, 2, (int) c.getProperty ("mood", 1));
            k.shape = readShape (c);
            k.file = c.getProperty ("file").toString();
            k.name = c.getProperty ("name").toString();
            kept.push_back (k);
        }
    }
}

} // namespace rk
