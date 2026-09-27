#pragma once

#include "CategoryProfile.h"
#include "Pattern.h"

#include <juce_core/juce_core.h>

#include <optional>
#include <set>

namespace rk
{

struct Preset
{
    juce::String name;
    int category = kUserCategory;
    double bpmHint = 140.0;
    juce::StringArray tags;
    Pattern pattern;

    bool isFactory = false;
    juce::String id;            // "factory/<file>" or "user/<file>" - stable key for favorites and state
    juce::File file;            // user presets only

    int bars() const noexcept { return pattern.bars; }
};

/** Loads factory presets (embedded JSON), user presets and favorites. Message thread only. */
class RollLibrary
{
public:
    RollLibrary();

    /** Parses one preset JSON document. Returns nullopt and fills 'error' on failure. */
    static std::optional<Preset> parsePreset (const juce::String& json, juce::String& error);
    static juce::String toJson (const Preset& preset);

    void loadFactoryPresets();
    void setUserDirectory (const juce::File& dir);     // loads user presets + favorites from there
    static juce::File defaultUserDirectory();

    int getNumPresets() const noexcept { return (int) presets.size(); }
    int getNumFactoryPresets() const noexcept { return numFactory; }
    const Preset& getPreset (int index) const;
    int indexOfId (const juce::String& id) const;

    std::vector<int> presetsInCategory (int category) const;
    std::vector<int> favoritePresets() const;
    /** Case-insensitive search in name, category and tags. category < 0 = all. */
    std::vector<int> search (const juce::String& query, int category = -1) const;

    bool isFavorite (int index) const;
    void setFavorite (int index, bool shouldBeFavorite);

    /** Saves the pattern as a new user preset. Returns the new index or -1. */
    int saveUserPreset (const juce::String& name, const Pattern& pattern, double bpmHint, const juce::StringArray& tags);

    juce::StringArray getLoadErrors() const { return loadErrors; }

private:
    void loadUserPresets();
    void loadFavorites();
    void saveFavorites() const;

    std::vector<Preset> presets;
    int numFactory = 0;
    std::set<juce::String> favorites;
    juce::File userDir;
    juce::StringArray loadErrors;
};

} // namespace rk
