#include "RollLibrary.h"

#include "PresetData.h"

#include <algorithm>

namespace rk
{

RollLibrary::RollLibrary()
{
    loadFactoryPresets();
}

//==============================================================================
std::optional<Preset> RollLibrary::parsePreset (const juce::String& json, juce::String& error)
{
    juce::var root;
    const auto result = juce::JSON::parse (json, root);

    if (result.failed() || ! root.isObject())
    {
        error = "invalid JSON: " + result.getErrorMessage();
        return std::nullopt;
    }

    Preset p;
    p.name = root.getProperty ("name", {}).toString().trim();
    const auto categoryName = root.getProperty ("category", "USER").toString().toUpperCase();
    p.category = findCategoryIndex (categoryName.toRawUTF8());
    p.bpmHint = (double) root.getProperty ("bpmHint", 140.0);
    p.pattern.bars = (int) root.getProperty ("bars", 1);

    if (p.name.isEmpty())
    {
        error = "missing name";
        return std::nullopt;
    }

    if (p.category < 0)
    {
        error = "unknown category '" + categoryName + "'";
        return std::nullopt;
    }

    if (auto* tags = root.getProperty ("tags", {}).getArray())
        for (const auto& t : *tags)
            p.tags.add (t.toString());

    auto* notes = root.getProperty ("notes", {}).getArray();
    if (notes == nullptr)
    {
        error = "missing notes array";
        return std::nullopt;
    }

    for (const auto& nv : *notes)
    {
        Note n;
        n.beat = (double) nv.getProperty ("beat", 0.0);
        n.len = (double) nv.getProperty ("len", 0.1);
        n.vel = (int) nv.getProperty ("vel", 100);
        n.pitch = (int) nv.getProperty ("pitch", 0);
        p.pattern.notes.push_back (n);
    }

    p.pattern.sort();
    return p;
}

juce::String RollLibrary::toJson (const Preset& preset)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("name", preset.name);
    obj->setProperty ("category", getCategoryProfile (preset.category).name);
    obj->setProperty ("bpmHint", preset.bpmHint);
    obj->setProperty ("bars", preset.pattern.bars);

    juce::Array<juce::var> tags;
    for (const auto& t : preset.tags)
        tags.add (t);
    obj->setProperty ("tags", tags);

    juce::Array<juce::var> notes;
    for (const auto& n : preset.pattern.notes)
    {
        auto* no = new juce::DynamicObject();
        no->setProperty ("beat", std::round (n.beat * 1.0e6) / 1.0e6);
        no->setProperty ("len", std::round (n.len * 1.0e6) / 1.0e6);
        no->setProperty ("vel", n.vel);
        no->setProperty ("pitch", n.pitch);
        notes.add (juce::var (no));
    }
    obj->setProperty ("notes", notes);

    return juce::JSON::toString (juce::var (obj));
}

//==============================================================================
void RollLibrary::loadFactoryPresets()
{
    std::vector<Preset> factory;

    std::vector<int> order;
    for (int i = 0; i < PresetData::namedResourceListSize; ++i)
        if (juce::String (PresetData::originalFilenames[i]).endsWithIgnoreCase (".json"))
            order.push_back (i);

    std::sort (order.begin(), order.end(), [] (int a, int b)
               { return juce::String (PresetData::originalFilenames[a]) < juce::String (PresetData::originalFilenames[b]); });

    for (auto i : order)
    {
        int size = 0;
        const auto* data = PresetData::getNamedResource (PresetData::namedResourceList[i], size);
        const auto filename = juce::String (PresetData::originalFilenames[i]);
        juce::String error;

        if (auto p = parsePreset (juce::String::fromUTF8 (data, size), error))
        {
            p->isFactory = true;
            p->id = "factory/" + filename;
            factory.push_back (std::move (*p));
        }
        else
        {
            loadErrors.add (filename + ": " + error);
        }
    }

    // Factory presets first (category order, then file order), user presets after.
    std::stable_sort (factory.begin(), factory.end(), [] (const Preset& a, const Preset& b) { return a.category < b.category; });

    std::vector<Preset> user;
    for (auto& p : presets)
        if (! p.isFactory)
            user.push_back (std::move (p));

    presets = std::move (factory);
    numFactory = (int) presets.size();
    for (auto& p : user)
        presets.push_back (std::move (p));
}

juce::File RollLibrary::defaultUserDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Killa").getChildFile ("Rolls Killa");
}

void RollLibrary::setUserDirectory (const juce::File& dir)
{
    userDir = dir;
    loadUserPresets();
    loadFavorites();
}

void RollLibrary::loadUserPresets()
{
    presets.erase (presets.begin() + numFactory, presets.end());

    const auto presetDir = userDir.getChildFile ("User Presets");
    if (! presetDir.isDirectory())
        return;

    auto files = presetDir.findChildFiles (juce::File::findFiles, false, "*.json");
    files.sort();

    for (const auto& f : files)
    {
        juce::String error;
        if (auto p = parsePreset (f.loadFileAsString(), error))
        {
            p->category = kUserCategory;
            p->isFactory = false;
            p->file = f;
            p->id = "user/" + f.getFileName();
            presets.push_back (std::move (*p));
        }
        else
        {
            loadErrors.add (f.getFileName() + ": " + error);
        }
    }
}

void RollLibrary::loadFavorites()
{
    favorites.clear();
    const auto json = juce::JSON::parse (userDir.getChildFile ("favorites.json").loadFileAsString());

    if (auto* arr = json.getArray())
        for (const auto& v : *arr)
            favorites.insert (v.toString());
}

void RollLibrary::saveFavorites() const
{
    if (userDir == juce::File())
        return;

    juce::Array<juce::var> arr;
    for (const auto& f : favorites)
        arr.add (f);

    userDir.createDirectory();
    userDir.getChildFile ("favorites.json").replaceWithText (juce::JSON::toString (arr));
}

//==============================================================================
const Preset& RollLibrary::getPreset (int index) const
{
    jassert (! presets.empty());
    return presets[(size_t) std::clamp (index, 0, (int) presets.size() - 1)];
}

int RollLibrary::indexOfId (const juce::String& id) const
{
    for (size_t i = 0; i < presets.size(); ++i)
        if (presets[i].id == id)
            return (int) i;

    return -1;
}

std::vector<int> RollLibrary::presetsInCategory (int category) const
{
    std::vector<int> out;
    for (size_t i = 0; i < presets.size(); ++i)
        if (presets[i].category == category)
            out.push_back ((int) i);
    return out;
}

std::vector<int> RollLibrary::favoritePresets() const
{
    std::vector<int> out;
    for (size_t i = 0; i < presets.size(); ++i)
        if (favorites.count (presets[i].id) > 0)
            out.push_back ((int) i);
    return out;
}

std::vector<int> RollLibrary::search (const juce::String& query, int category) const
{
    const auto q = query.trim().toLowerCase();
    std::vector<int> out;

    for (size_t i = 0; i < presets.size(); ++i)
    {
        const auto& p = presets[i];
        if (category >= 0 && p.category != category)
            continue;

        auto haystack = p.name + " " + getCategoryProfile (p.category).name + " " + p.tags.joinIntoString (" ");
        if (q.isEmpty() || haystack.toLowerCase().contains (q))
            out.push_back ((int) i);
    }

    return out;
}

bool RollLibrary::isFavorite (int index) const
{
    return index >= 0 && index < (int) presets.size() && favorites.count (presets[(size_t) index].id) > 0;
}

void RollLibrary::setFavorite (int index, bool shouldBeFavorite)
{
    if (index < 0 || index >= (int) presets.size())
        return;

    if (shouldBeFavorite)
        favorites.insert (presets[(size_t) index].id);
    else
        favorites.erase (presets[(size_t) index].id);

    saveFavorites();
}

int RollLibrary::saveUserPreset (const juce::String& name, const Pattern& pattern, double bpmHint, const juce::StringArray& tags)
{
    if (userDir == juce::File() || name.trim().isEmpty())
        return -1;

    Preset p;
    p.name = name.trim();
    p.category = kUserCategory;
    p.bpmHint = bpmHint;
    p.tags = tags;
    p.pattern = pattern;

    for (auto& n : p.pattern.notes)
        if (n.muted)
            n.vel = 0;

    p.pattern.notes.erase (std::remove_if (p.pattern.notes.begin(), p.pattern.notes.end(), [] (const Note& n) { return n.vel <= 0; }),
                           p.pattern.notes.end());

    const auto presetDir = userDir.getChildFile ("User Presets");
    presetDir.createDirectory();
    const auto file = presetDir.getNonexistentChildFile (juce::File::createLegalFileName (p.name), ".json", false);

    if (! file.replaceWithText (toJson (p)))
        return -1;

    loadUserPresets();
    return indexOfId ("user/" + file.getFileName());
}

} // namespace rk
