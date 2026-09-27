#pragma once

#include "Widgets.h"
#include "../engine/CategoryProfile.h"

class RollsKillaProcessor;

namespace rk::ui
{

/** Overlay with categories (sigils), search, preset table, favorites and a hover preview. */
class PresetBrowser : public juce::Component, private juce::TextEditor::Listener
{
public:
    explicit PresetBrowser (RollsKillaProcessor&);
    ~PresetBrowser() override;

    void open (int category);
    void close();

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

    std::function<void()> onPresetLoaded;

    static constexpr int kFavorites = kUserCategory + 1;

private:
    class CategoryList;
    class Table;

    void textEditorTextChanged (juce::TextEditor&) override;
    void selectCategory (int category);
    void rebuildRows();
    void load (int presetIndex);
    void promptSaveUserPreset();
    juce::Rectangle<int> panelBounds() const;

    RollsKillaProcessor& processor;
    std::unique_ptr<CategoryList> categories;
    std::unique_ptr<Table> table;
    juce::Viewport viewport;
    juce::TextEditor search;
    IconButton closeButton { "Close", Icon::close };
    IconButton saveButton { "Save user preset", Icon::folder };
    int category = 0;
    std::vector<int> rows;
    std::unique_ptr<juce::AlertWindow> saveDialog;
};

} // namespace rk::ui
