#include "engine/PresetValidator.h"
#include "engine/RollLibrary.h"

namespace rk
{

class LibraryTests : public juce::UnitTest
{
public:
    LibraryTests() : juce::UnitTest ("Roll library", "RollsKilla") {}

    void runTest() override
    {
        RollLibrary lib;

        beginTest ("factory presets load without errors");
        expect (lib.getLoadErrors().isEmpty(), lib.getLoadErrors().joinIntoString ("\n"));
        expectGreaterThan (lib.getNumFactoryPresets(), 0);

        beginTest ("every category has the same number of presets");
        {
            const auto perCategory = (int) lib.presetsInCategory (0).size();
            expectGreaterOrEqual (perCategory, 2);
            for (int c = 0; c < kNumFactoryCategories; ++c)
                expectEquals ((int) lib.presetsInCategory (c).size(), perCategory, getCategoryProfile (c).name);
            expectEquals (lib.getNumFactoryPresets(), perCategory * kNumFactoryCategories);
           #if ROLLSKILLA_FULL_LIBRARY
            expectEquals (lib.getNumFactoryPresets(), 96);
            expectEquals (perCategory, 8);
           #endif
        }

        beginTest ("all factory presets pass validation");
        for (int i = 0; i < lib.getNumFactoryPresets(); ++i)
        {
            const auto& p = lib.getPreset (i);
            const auto errors = validatePreset (p);
            expect (errors.isEmpty(), p.name + ": " + errors.joinIntoString ("; "));
        }

        beginTest ("preset names are unique and bars match the pattern");
        {
            juce::StringArray names;
            for (int i = 0; i < lib.getNumFactoryPresets(); ++i)
            {
                const auto& p = lib.getPreset (i);
                expect (! names.contains (p.name), "duplicate name " + p.name);
                names.add (p.name);
                expect (p.bars() == 1 || p.bars() == 2 || p.bars() == 4);
                expect (p.pattern.notes.back().beat < p.pattern.lengthBeats());
            }
        }

        beginTest ("library statistics match the analysed trap numbers");
        {
            std::vector<const Preset*> all;
            for (int i = 0; i < lib.getNumFactoryPresets(); ++i)
                all.push_back (&lib.getPreset (i));

            const auto stats = computeStats (all);
            logMessage (stats.toString());

            expectWithinAbsoluteError (stats.rollsPerBar(), 0.8, 0.4);
            expectGreaterOrEqual (stats.flatVelocityShare(), 0.6);
           #if ROLLSKILLA_FULL_LIBRARY
            expectGreaterOrEqual (stats.tripletRollShare(), 0.30);
            expectLessOrEqual (stats.tripletRollShare(), 0.55);
           #else
            expectGreaterOrEqual (stats.tripletRollShare(), 0.25);
           #endif
        }

        beginTest ("json round trip");
        {
            const auto& p = lib.getPreset (0);
            juce::String error;
            const auto copy = RollLibrary::parsePreset (RollLibrary::toJson (p), error);
            expect (copy.has_value(), error);
            expectEquals (copy->name, p.name);
            expectEquals (copy->category, p.category);
            expectEquals ((int) copy->pattern.notes.size(), (int) p.pattern.notes.size());
            for (size_t i = 0; i < p.pattern.notes.size(); ++i)
            {
                expectWithinAbsoluteError (copy->pattern.notes[i].beat, p.pattern.notes[i].beat, 1.0e-6);
                expectEquals (copy->pattern.notes[i].vel, p.pattern.notes[i].vel);
                expectEquals (copy->pattern.notes[i].pitch, p.pattern.notes[i].pitch);
            }
        }

        beginTest ("invalid presets are rejected");
        {
            juce::String error;
            expect (! RollLibrary::parsePreset ("{ nonsense", error).has_value());
            expect (! RollLibrary::parsePreset (R"({"name":"x","category":"POLKA","bars":1,"notes":[]})", error).has_value());

            auto p = lib.getPreset (0);
            p.pattern.notes[0].vel = 200;
            expect (! validatePreset (p).isEmpty());

            auto q = lib.getPreset (0);
            Note bad;            // roll step of 0.1 beat is not an allowed rate
            bad.beat = q.pattern.notes[0].beat + 0.1;
            bad.len = 0.05;
            q.pattern.notes.push_back (bad);
            q.pattern.sort();
            expect (! validatePreset (q).isEmpty());
        }

        beginTest ("search, categories and favorites");
        {
            juce::TemporaryFile tmp;
            const auto dir = tmp.getFile();
            dir.createDirectory();
            lib.setUserDirectory (dir);

            expect (! lib.search ("codex").empty());
            expect (lib.search ("zzzz-no-such-preset").empty());
            expectEquals ((int) lib.search ("", 2).size(), (int) lib.presetsInCategory (2).size());

            expect (! lib.isFavorite (3));
            lib.setFavorite (3, true);
            expect (lib.isFavorite (3));

            RollLibrary other;
            other.setUserDirectory (dir);
            expect (other.isFavorite (3), "favorites persist");

            const auto idx = lib.saveUserPreset ("My Roll", lib.getPreset (1).pattern, 150.0, { "mine" });
            expectGreaterOrEqual (idx, lib.getNumFactoryPresets());
            expectEquals (lib.getPreset (idx).name, juce::String ("My Roll"));
            expectEquals (lib.getPreset (idx).category, kUserCategory);
            expectEquals ((int) lib.presetsInCategory (kUserCategory).size(), 1);

            dir.deleteRecursively();
        }

        beginTest ("tiling keeps the fill at the end of the phrase");
        {
            const auto& p = lib.getPreset (lib.search ("Bando Evangelium").front());
            expectEquals (p.bars(), 4);
            const auto rollsIn = findRolls (p.pattern);
            const auto lastRoll = rollsIn.back();

            const auto eight = tileToBars (p.pattern, 8);
            expectEquals (eight.bars, 8);
            const auto rolls8 = findRolls (eight);
            expectEquals ((int) rolls8.size(), (int) rollsIn.size() * 2);
            expectWithinAbsoluteError (rolls8.back().start, lastRoll.start + 16.0, 1.0e-6);

            const auto two = tileToBars (p.pattern, 2);
            const auto rolls2 = findRolls (two);
            expectWithinAbsoluteError (rolls2.back().start, lastRoll.start - 8.0, 1.0e-6);
            expect (validatePatternInvariants (two).isEmpty());
        }
    }
};

static LibraryTests libraryTests;

} // namespace rk
