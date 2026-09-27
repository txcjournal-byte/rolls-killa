#include "engine/PatternShaper.h"
#include "engine/PresetValidator.h"
#include "engine/RollModel.h"
#include "engine/UndoHistory.h"
#include "engine/VariationEngine.h"

namespace rk
{

static bool samePattern (const Pattern& a, const Pattern& b)
{
    if (a.bars != b.bars || a.notes.size() != b.notes.size())
        return false;
    for (size_t i = 0; i < a.notes.size(); ++i)
    {
        const auto& x = a.notes[i];
        const auto& y = b.notes[i];
        if (std::abs (x.beat - y.beat) > 1.0e-9 || x.vel != y.vel || x.pitch != y.pitch || x.muted != y.muted)
            return false;
    }
    return true;
}

static std::vector<const Note*> notesInBar (const Pattern& p, int bar)
{
    std::vector<const Note*> out;
    for (const auto& n : p.notes)
        if ((int) (n.beat / kBeatsPerBar) == bar)
            out.push_back (&n);
    return out;
}

class ShaperTests : public juce::UnitTest
{
public:
    ShaperTests() : juce::UnitTest ("Pattern shaper / KILL / model", "RollsKilla") {}

    void runTest() override
    {
        RollLibrary lib;
        const auto numPresets = lib.getNumFactoryPresets();

        beginTest ("default knobs = exact original preset");
        for (int i = 0; i < numPresets; ++i)
        {
            const auto& preset = lib.getPreset (i);
            const auto shaped = shapePattern (preset.pattern, ShapeParams {}, getCategoryProfile (preset.category), 1234);
            expect (samePattern (shaped, preset.pattern), preset.name);

            ModelSettings s;
            s.presetIndex = i;
            s.bars = preset.bars();
            s.seed = 0;
            expect (samePattern (RollModel::build (lib, s), preset.pattern), preset.name + " through the model");
        }

        beginTest ("shaper output keeps the invariants for every knob combination");
        {
            juce::Random r (42);
            int checked = 0;
            for (int i = 0; i < numPresets; ++i)
            {
                const auto& preset = lib.getPreset (i);
                for (int k = 0; k < 12; ++k)
                {
                    ShapeParams sp;
                    sp.speedShift = r.nextInt (3) - 1;
                    sp.density = r.nextInt (201);
                    sp.velMode = (VelocityMode) r.nextInt (4);
                    sp.groove = r.nextInt (101);
                    sp.pitchRamp = r.nextInt (25) - 12;
                    sp.swing = r.nextInt (61);
                    const auto out = shapePattern (preset.pattern, sp, getCategoryProfile (preset.category), (uint32_t) k);
                    const auto errors = validatePatternInvariants (out);
                    expect (errors.isEmpty(), preset.name + ": " + errors.joinIntoString ("; "));
                    ++checked;
                }
            }
            logMessage ("checked " + juce::String (checked) + " shaped patterns");
        }

        beginTest ("shaper is deterministic");
        {
            const auto& preset = lib.getPreset (5);
            ShapeParams sp;
            sp.density = 170;
            sp.groove = 60;
            sp.speedShift = 1;
            const auto& prof = getCategoryProfile (preset.category);
            expect (samePattern (shapePattern (preset.pattern, sp, prof, 77), shapePattern (preset.pattern, sp, prof, 77)));
        }

        beginTest ("roll speed changes the roll rate, density changes the number of rolls");
        {
            const auto& preset = lib.getPreset (lib.search ("Codex Trappus").front());
            const auto& prof = getCategoryProfile (preset.category);
            const auto original = findRolls (preset.pattern);

            ShapeParams faster;
            faster.speedShift = 1;
            const auto fastRolls = findRolls (shapePattern (preset.pattern, faster, prof, 1));
            expectEquals ((int) fastRolls.size(), (int) original.size());
            for (size_t i = 0; i < std::min (fastRolls.size(), original.size()); ++i)
            {
                expectLessThan (fastRolls[i].step, original[i].step);
                expectWithinAbsoluteError (fastRolls[i].length, original[i].length, original[i].step);
            }

            ShapeParams none;
            none.density = 0;
            expectEquals ((int) findRolls (shapePattern (preset.pattern, none, prof, 1)).size(), 0);

            ShapeParams more;
            more.density = 200;
            expectGreaterThan ((int) findRolls (shapePattern (preset.pattern, more, prof, 1)).size(), (int) original.size());
        }

        beginTest ("velocity modes and pitch ramp");
        {
            const auto& preset = lib.getPreset (lib.search ("Codex Trappus").front());
            const auto& prof = getCategoryProfile (preset.category);

            ShapeParams up;
            up.velMode = VelocityMode::rampUp;
            up.pitchRamp = 7;
            const auto p = shapePattern (preset.pattern, up, prof, 1);
            for (const auto& r : findRolls (p))
            {
                expect (classifyVelocity (p, r) == VelocityShape::rampUp);
                expectEquals (p.notes[(size_t) r.last()].pitch - p.notes[(size_t) r.first].pitch, 7);
            }
        }

        beginTest ("KILL: deterministic, respects category rules");
        {
            int different = 0;
            for (int i = 0; i < numPresets; ++i)
            {
                const auto& preset = lib.getPreset (i);
                const auto& prof = getCategoryProfile (preset.category);

                expect (samePattern (varyPattern (preset.pattern, prof, 0, 1.0, 0), preset.pattern), "seed 0 = original");
                expect (samePattern (varyPattern (preset.pattern, prof, 55, 0.0, 0), preset.pattern), "amount 0 = original");

                for (uint32_t seed = 1; seed <= 25; ++seed)
                {
                    const auto a = varyPattern (preset.pattern, prof, seed, 1.0, 0);
                    const auto b = varyPattern (preset.pattern, prof, seed, 1.0, 0);
                    expect (samePattern (a, b), "deterministic");
                    different += samePattern (a, preset.pattern) ? 0 : 1;

                    Preset varied = preset;
                    varied.pattern = a;
                    const auto errors = validatePreset (varied);
                    expect (errors.isEmpty(), preset.name + " seed " + juce::String ((int) seed) + ": " + errors.joinIntoString ("; "));
                }
            }
            expectGreaterThan (different, numPresets * 25 * 3 / 4, "KILL actually changes most patterns");
        }

        beginTest ("KILL: locked bars never change");
        for (int i = 0; i < numPresets; ++i)
        {
            const auto& preset = lib.getPreset (i);
            const auto tiled = tileToBars (preset.pattern, 4);
            for (uint32_t seed = 1; seed <= 10; ++seed)
            {
                const uint32_t locks = 0b0101;   // bars 1 and 3
                const auto v = varyPattern (tiled, getCategoryProfile (preset.category), seed, 1.0, locks);
                for (int bar : { 0, 2 })
                {
                    const auto a = notesInBar (tiled, bar);
                    const auto b = notesInBar (v, bar);
                    bool same = a.size() == b.size();
                    for (size_t k = 0; same && k < a.size(); ++k)
                        same = std::abs (a[k]->beat - b[k]->beat) < 1.0e-9 && a[k]->vel == b[k]->vel && a[k]->pitch == b[k]->pitch;
                    expect (same, preset.name + " bar " + juce::String (bar + 1) + " seed " + juce::String ((int) seed));
                }
            }
        }

        beginTest ("model: edits mute notes, change velocity and roll rate");
        {
            const auto index = lib.search ("Codex Trappus").front();
            ModelSettings s;
            s.presetIndex = index;
            s.bars = 2;
            Pattern editable;
            const auto base = RollModel::build (lib, s, &editable);
            const auto rolls = findRolls (editable);
            expect (! rolls.empty());

            const auto plainTick = beatToTick (0.0);
            const auto rollTick = beatToTick (rolls[0].start);
            s.edits[plainTick] = { true, -1, -1 };
            s.edits[beatToTick (0.5)] = { false, 30, -1 };
            s.edits[rollTick] = { false, -1, rate64 };

            const auto edited = RollModel::build (lib, s);
            const auto& first = edited.notes.front();
            expect (first.muted);
            const auto second = std::find_if (edited.notes.begin(), edited.notes.end(), [] (const Note& n) { return std::abs (n.beat - 0.5) < 1.0e-9; });
            expect (second != edited.notes.end() && second->vel == 30);

            const auto newRolls = findRolls (edited);
            const auto r = std::find_if (newRolls.begin(), newRolls.end(), [&] (const Roll& x) { return std::abs (x.start - rolls[0].start) < 1.0e-9; });
            expect (r != newRolls.end() && rateIndexForStep (r->step) == rate64);

            const auto playback = PlaybackPattern::fromPattern (edited, 60, 140.0);
            expectEquals ((int) playback.events.size(), (int) edited.notes.size() - 1, "muted note does not play");
            juce::ignoreUnused (base);
        }

        beginTest ("model: delete a note, delete a whole roll, delete a re-timed roll note");
        {
            const auto index = lib.search ("Codex Trappus").front();
            ModelSettings s;
            s.presetIndex = index;
            s.bars = 2;
            Pattern editable;
            const auto base = RollModel::build (lib, s, &editable);
            const auto rolls = findRolls (base);
            expect (! rolls.empty());

            // delete one plain note
            auto del = s;
            del.edits[beatToTick (0.5)].deleted = true;
            const auto withoutNote = RollModel::build (lib, del);
            expectEquals ((int) withoutNote.notes.size(), (int) base.notes.size() - 1);
            expect (std::none_of (withoutNote.notes.begin(), withoutNote.notes.end(), [] (const Note& n) { return std::abs (n.beat - 0.5) < 1.0e-9; }));

            // delete the first roll: fewer rolls, the groove keeps going (grid refilled)
            auto noRoll = s;
            noRoll.edits[beatToTick (rolls.front().start)].removeRoll = true;
            const auto withoutRoll = RollModel::build (lib, noRoll);
            expectEquals ((int) findRolls (withoutRoll).size(), (int) rolls.size() - 1);
            expect (validatePatternInvariants (withoutRoll).isEmpty());
            const auto hitsInRollSpan = std::count_if (withoutRoll.notes.begin(), withoutRoll.notes.end(), [&] (const Note& n)
                                                       { return n.beat >= rolls.front().start - 1.0e-9 && n.beat < rolls.front().start + rolls.front().length; });
            expectGreaterThan ((int) hitsInRollSpan, 0, "plain hits fill the removed roll");

            // with Roll Speed = Faster the roll notes are regenerated; they can still be deleted by position
            auto fast = s;
            fast.shape.speedShift = 1;
            const auto fastPattern = RollModel::build (lib, fast);
            const auto fastRolls = findRolls (fastPattern);
            const auto& generated = fastPattern.notes[(size_t) fastRolls.front().first + 1];
            expectLessThan (generated.srcTick, 0, "note re-timed by the shaper");
            fast.edits[RollModel::editKeyOf (generated)].deleted = true;
            const auto fastDeleted = RollModel::build (lib, fast);
            expectEquals ((int) fastDeleted.notes.size(), (int) fastPattern.notes.size() - 1);
            expect (validatePatternInvariants (fastDeleted).isEmpty());

            // a generated roll can be removed as a whole too
            auto fastNoRoll = s;
            fastNoRoll.shape.speedShift = 1;
            fastNoRoll.edits[RollModel::editKeyOf (fastPattern.notes[(size_t) fastRolls.front().first])].removeRoll = true;
            expectEquals ((int) findRolls (RollModel::build (lib, fastNoRoll)).size(), (int) fastRolls.size() - 1);
        }

        beginTest ("undo history keeps the last 20 changes");
        {
            UndoHistory h;
            juce::ValueTree t ("S");
            t.setProperty ("v", 0, nullptr);
            h.reset (t);
            expect (! h.canUndo());
            expect (! h.push (t), "identical state is not recorded");

            for (int i = 1; i <= 30; ++i)
            {
                t.setProperty ("v", i, nullptr);
                expect (h.push (t));
            }
            expectEquals (h.getNumUndoSteps(), 20);
            int undone = 0;
            while (h.canUndo())
            {
                h.undo();
                ++undone;
            }
            expectEquals (undone, 20);
            expectEquals ((int) h.current().getProperty ("v"), 10);
            expect (h.canRedo());
            expectEquals ((int) h.redo().getProperty ("v"), 11);

            t.setProperty ("v", 99, nullptr);
            h.push (t);
            expect (! h.canRedo(), "a new change clears redo");
        }
    }
};

static ShaperTests shaperTests;

} // namespace rk
