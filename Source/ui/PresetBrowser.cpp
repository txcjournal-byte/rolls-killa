#include "PresetBrowser.h"

#include "../PluginProcessor.h"
#include "Sigils.h"

namespace rk::ui
{

namespace
{
    juce::String categoryName (int c)
    {
        if (c == PresetBrowser::kFavorites) return "FAVORITES";
        return getCategoryProfile (c).name;
    }

    /** Tiny pattern preview (same look as the visualizer). */
    void drawMiniPattern (juce::Graphics& g, const Pattern& p, juce::Rectangle<float> area)
    {
        g.setColour (colours::background);
        g.fillRect (area);
        const auto len = juce::jmax (1.0, p.lengthBeats());
        for (int bar = 1; bar < p.bars; ++bar)
        {
            g.setColour (colours::outline);
            g.drawVerticalLine (juce::roundToInt (area.getX() + area.getWidth() * (float) (bar * kBeatsPerBar / len)), area.getY(), area.getBottom());
        }
        for (const auto& n : p.notes)
        {
            const auto x = area.getX() + area.getWidth() * (float) (n.beat / len);
            const auto h = area.getHeight() * 0.92f * (float) n.vel / 127.0f;
            g.setColour (pitchColour (n.pitch));
            g.fillRect (juce::Rectangle<float> (x, area.getBottom() - h, 1.6f, h));
        }
    }
}

//==============================================================================
class PresetBrowser::CategoryList : public juce::Component
{
public:
    explicit CategoryList (PresetBrowser& b) : owner (b) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    static constexpr int kRowH = 28;

    int rowAt (int y) const
    {
        const auto i = y / kRowH;
        if (i < kNumFactoryCategories) return i;
        if (y < kNumFactoryCategories * kRowH + 12) return -1;
        const auto j = (y - kNumFactoryCategories * kRowH - 12) / kRowH;
        return j == 0 ? kUserCategory : j == 1 ? kFavorites : -1;
    }

    juce::Rectangle<int> rowBounds (int c) const
    {
        const auto i = c < kNumFactoryCategories ? c : kNumFactoryCategories + (c - kUserCategory);
        return { 0, i * kRowH + (c >= kUserCategory ? 12 : 0), getWidth(), kRowH };
    }

    void paint (juce::Graphics& g) override
    {
        for (int c = 0; c <= kFavorites; ++c)
        {
            const auto r = rowBounds (c).reduced (4, 2).toFloat();
            const auto selected = owner.category == c && owner.search.isEmpty();
            if (selected)
            {
                drawGlow (g, r, 4.0f, colours::accent, 0.9f, 3);
                g.setColour (colours::accent.withAlpha (0.18f));
                g.fillRoundedRectangle (r, 4.0f);
                g.setColour (colours::accent);
                g.drawRoundedRectangle (r, 4.0f, 1.0f);
            }
            else if (hover == c)
            {
                g.setColour (colours::panelLight.brighter (0.06f));
                g.fillRoundedRectangle (r, 4.0f);
            }

            drawSigil (g, c, r.withWidth (r.getHeight()).reduced (4.0f).translated (6.0f, 0.0f),
                       colours::accent.withAlpha (selected || hover == c ? 1.0f : 0.85f));
            g.setColour (selected ? colours::accent : colours::text);
            g.setFont (labelFont (12.5f));
            g.drawText (categoryName (c), r.withTrimmedLeft (r.getHeight() + 16.0f), juce::Justification::centredLeft, false);
        }

        g.setColour (colours::outline);
        g.drawHorizontalLine (kNumFactoryCategories * kRowH + 6, 10.0f, (float) getWidth() - 10.0f);
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const auto h = rowAt (e.y);
        if (h != hover) { hover = h; repaint(); }
    }

    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }

    void mouseUp (const juce::MouseEvent& e) override
    {
        const auto c = rowAt (e.y);
        if (c >= 0)
            owner.selectCategory (c);
    }

private:
    PresetBrowser& owner;
    int hover = -1;
};

//==============================================================================
class PresetBrowser::Table : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit Table (PresetBrowser& b) : owner (b) {}

    static constexpr int kRowH = 40;
    int selected = -1;          // row index
    int hover = -1;

    // column layout
    juce::Rectangle<int> starBounds (int row) const { return { getWidth() - 56, row * kRowH, 44, kRowH }; }

    void paint (juce::Graphics& g) override
    {
        auto& lib = owner.processor.getLibrary();
        const auto loaded = owner.processor.getModel().getSettings().presetIndex;

        for (int row = 0; row < (int) owner.rows.size(); ++row)
        {
            const auto idx = owner.rows[(size_t) row];
            const auto& p = lib.getPreset (idx);
            auto r = juce::Rectangle<int> (0, row * kRowH, getWidth(), kRowH);
            const auto isLoaded = idx == loaded;

            if (isLoaded)
            {
                g.setColour (colours::accent.withAlpha (0.10f));
                g.fillRect (r);
            }
            else if (row == hover || row == selected)
            {
                g.setColour (colours::panelLight.brighter (0.05f));
                g.fillRect (r);
            }
            g.setColour (colours::outline.withAlpha (0.6f));
            g.drawHorizontalLine (r.getBottom() - 1, 8.0f, (float) getWidth() - 8.0f);

            auto rr = r.reduced (8, 0);
            drawIcon (g, Icon::play, rr.removeFromLeft (26).withSizeKeepingCentre (12, 12).toFloat(),
                      isLoaded ? colours::accent : colours::textDim);
            rr.removeFromLeft (10);

            const auto textCol = isLoaded ? colours::accent : colours::text;
            g.setColour (textCol);
            g.setFont (uiFont (16.0f, false));
            g.drawText (p.name, rr.removeFromLeft (200), juce::Justification::centredLeft, true);

            if (owner.search.getText().isNotEmpty() || owner.category == kFavorites)
            {
                g.setColour (colours::textDark);
                g.setFont (labelFont (9.5f));
                g.drawText (getCategoryProfile (p.category).name, juce::Rectangle<int> (rr.getX() - 90, r.getY(), 86, kRowH),
                            juce::Justification::centredRight, false);
            }

            g.setColour (textCol);
            g.setFont (uiFont (15.0f, false));
            g.drawText (juce::String (juce::roundToInt (p.bpmHint)), rr.removeFromLeft (60), juce::Justification::centred, false);
            g.drawText (juce::String (p.bars()), rr.removeFromLeft (56), juce::Justification::centred, false);
            rr.removeFromLeft (12);

            // tag pills
            g.setFont (uiFont (11.5f, false));
            auto tags = rr.withTrimmedRight (50);
            for (const auto& tag : p.tags)
            {
                const auto w = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), tag) + 14;
                if (w > tags.getWidth())
                    break;
                const auto pill = tags.removeFromLeft (w).withSizeKeepingCentre (w, 22).toFloat();
                tags.removeFromLeft (6);
                g.setColour (isLoaded ? colours::accent.withAlpha (0.15f) : colours::panelLight);
                g.fillRoundedRectangle (pill, 4.0f);
                g.setColour (isLoaded ? colours::accent : colours::outlineLight);
                g.drawRoundedRectangle (pill, 4.0f, 1.0f);
                g.setColour (isLoaded ? colours::accent : colours::textDim);
                g.drawText (tag, pill, juce::Justification::centred, false);
            }

            const auto fav = lib.isFavorite (idx);
            drawIcon (g, fav ? Icon::starFilled : Icon::star, starBounds (row).withSizeKeepingCentre (18, 18).toFloat(),
                      fav ? colours::accent : colours::textDim);
        }

        if (owner.rows.empty())
        {
            g.setColour (colours::textDim);
            g.setFont (uiFont (14.0f, false));
            const auto msg = owner.category == kFavorites ? "No favorites yet - click a star to add one."
                           : owner.category == kUserCategory && owner.search.isEmpty() ? "No user presets yet - use SAVE to store the current pattern."
                           : "No presets found.";
            g.drawText (msg, getLocalBounds().withHeight (80), juce::Justification::centred, false);
        }
    }

    int rowAt (int y) const
    {
        const auto r = y / kRowH;
        return r >= 0 && r < (int) owner.rows.size() ? r : -1;
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const auto r = rowAt (e.y);
        if (r != hover)
        {
            hover = r;
            repaint();
            owner.repaint();   // preview card
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        hover = -1;
        repaint();
        owner.repaint();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        const auto row = rowAt (e.y);
        if (row < 0)
            return;

        const auto idx = owner.rows[(size_t) row];
        if (starBounds (row).contains (e.getPosition()))
        {
            auto& lib = owner.processor.getLibrary();
            lib.setFavorite (idx, ! lib.isFavorite (idx));
            if (owner.category == kFavorites)
                owner.rebuildRows();
            repaint();
            return;
        }

        selected = row;
        owner.load (idx);
    }

private:
    PresetBrowser& owner;
};

//==============================================================================
PresetBrowser::PresetBrowser (RollsKillaProcessor& p) : processor (p)
{
    categories = std::make_unique<CategoryList> (*this);
    table = std::make_unique<Table> (*this);
    addAndMakeVisible (*categories);
    addAndMakeVisible (viewport);
    viewport.setViewedComponent (table.get(), false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (8);

    search.setTextToShowWhenEmpty ("Search presets...", colours::textDark);
    search.setFont (uiFont (15.0f, false));
    search.setIndents (34, 8);
    search.addListener (this);
    search.setEscapeAndReturnKeysConsumed (false);
    addAndMakeVisible (search);

    addAndMakeVisible (closeButton);
    closeButton.onClick = [this] { close(); };

    saveButton.label = "SAVE";
    saveButton.setTooltip ("Save the current pattern (with your tweaks) as a user preset");
    saveButton.onClick = [this] { promptSaveUserPreset(); };
    addAndMakeVisible (saveButton);

    setWantsKeyboardFocus (true);
}

PresetBrowser::~PresetBrowser()
{
    viewport.setViewedComponent (nullptr, false);
}

juce::Rectangle<int> PresetBrowser::panelBounds() const
{
    return getLocalBounds().reduced (36, 24);
}

void PresetBrowser::open (int newCategory)
{
    search.clear();
    setVisible (true);
    toFront (true);
    selectCategory (juce::jlimit (0, kFavorites, newCategory));

    const auto loaded = processor.getModel().getSettings().presetIndex;
    for (size_t i = 0; i < rows.size(); ++i)
        if (rows[i] == loaded)
        {
            table->selected = (int) i;
            viewport.setViewPosition (0, juce::jmax (0, (int) i * Table::kRowH - Table::kRowH * 2));
        }

    grabKeyboardFocus();
}

void PresetBrowser::close()
{
    if (saveDialog != nullptr)
        return;
    setVisible (false);
    if (auto* parent = getParentComponent())
        parent->grabKeyboardFocus();
}

void PresetBrowser::selectCategory (int c)
{
    category = c;
    if (search.getText().isNotEmpty())
        search.clear();
    rebuildRows();
    categories->repaint();
}

void PresetBrowser::rebuildRows()
{
    auto& lib = processor.getLibrary();
    const auto query = search.getText().trim();

    if (query.isNotEmpty())
        rows = lib.search (query);
    else if (category == kFavorites)
        rows = lib.favoritePresets();
    else
        rows = lib.presetsInCategory (category);

    table->selected = -1;
    table->hover = -1;
    table->setSize (viewport.getMaximumVisibleWidth(), juce::jmax (viewport.getHeight(), (int) rows.size() * Table::kRowH));
    table->repaint();
    repaint();
}

void PresetBrowser::textEditorTextChanged (juce::TextEditor&)
{
    rebuildRows();
    categories->repaint();
}

void PresetBrowser::load (int presetIndex)
{
    processor.loadPreset (presetIndex);
    if (! processor.isHostPlaying() && ! processor.isPreviewEnabled())
        processor.setPreviewEnabled (true);          // click = load and play
    table->repaint();
    if (onPresetLoaded != nullptr)
        onPresetLoaded();
}

void PresetBrowser::promptSaveUserPreset()
{
    saveDialog = std::make_unique<juce::AlertWindow> ("Save user preset", "Name of the new preset:", juce::MessageBoxIconType::NoIcon, this);
    saveDialog->addTextEditor ("name", processor.getModel().getPreset().name + " (mine)");
    saveDialog->addButton ("SAVE", 1, juce::KeyPress (juce::KeyPress::returnKey));
    saveDialog->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    saveDialog->setLookAndFeel (&getLookAndFeel());

    saveDialog->enterModalState (true, juce::ModalCallbackFunction::create ([safe = juce::Component::SafePointer<PresetBrowser> (this)] (int result)
    {
        if (safe == nullptr || safe->saveDialog == nullptr)
            return;
        const auto name = safe->saveDialog->getTextEditorContents ("name");
        safe->saveDialog.reset();
        if (result == 1 && name.trim().isNotEmpty())
        {
            const auto idx = safe->processor.saveUserPreset (name);
            if (idx >= 0)
            {
                safe->selectCategory (kUserCategory);
                if (safe->onPresetLoaded != nullptr)
                    safe->onPresetLoaded();
            }
        }
        safe->grabKeyboardFocus();
    }), false);
}

void PresetBrowser::resized()
{
    auto panel = panelBounds().reduced (18);
    auto top = panel.removeFromTop (44);
    top.removeFromLeft (220);
    closeButton.setBounds (top.removeFromRight (40).withSizeKeepingCentre (38, 38));
    top.removeFromRight (16);
    search.setBounds (top.withSizeKeepingCentre (top.getWidth(), 38));
    panel.removeFromTop (12);

    auto left = panel.removeFromLeft (206);
    categories->setBounds (left.removeFromTop (kNumFactoryCategories * CategoryList::kRowH + 12 + 2 * CategoryList::kRowH).reduced (0, 2));
    panel.removeFromLeft (14);

    auto footer = panel.removeFromBottom (40);
    saveButton.setBounds (footer.removeFromRight (110).reduced (0, 5));
    panel.removeFromBottom (6);
    panel.removeFromTop (34);   // column headers
    viewport.setBounds (panel);
    rebuildRows();
}

void PresetBrowser::paint (juce::Graphics& g)
{
    // dim + blur-ish backdrop
    g.fillAll (juce::Colours::black.withAlpha (0.72f));

    const auto panel = panelBounds().toFloat();
    drawGlow (g, panel, 10.0f, colours::accent, 1.2f, 6);
    g.setColour (colours::background.withAlpha (0.97f));
    g.fillRoundedRectangle (panel, 10.0f);
    g.setColour (colours::accent.withAlpha (0.8f));
    g.drawRoundedRectangle (panel, 10.0f, 1.2f);

    auto inner = panelBounds().reduced (18);
    g.setColour (colours::text);
    g.setFont (labelFont (26.0f));
    g.drawText ("PRESETS", inner.removeFromTop (44).removeFromLeft (220), juce::Justification::centredLeft, false);

    // side + table frames
    const auto catFrame = categories->getBounds().toFloat().expanded (4.0f, 6.0f);
    drawPanel (g, catFrame, 6.0f);
    auto tableFrame = viewport.getBounds().toFloat().withTop ((float) viewport.getY() - 34.0f).expanded (4.0f, 4.0f);
    drawPanel (g, tableFrame, 6.0f);

    // column headers
    auto header = viewport.getBounds().withY (viewport.getY() - 34).withHeight (34).reduced (8, 0);
    g.setColour (colours::text);
    g.setFont (labelFont (11.0f));
    header.removeFromLeft (36);
    const auto title = search.getText().isNotEmpty() ? "RESULTS" : juce::String ("NAME");
    g.drawText (title, header.removeFromLeft (200), juce::Justification::centredLeft, false);
    g.drawText ("BPM", header.removeFromLeft (60), juce::Justification::centred, false);
    g.drawText ("BARS", header.removeFromLeft (56), juce::Justification::centred, false);
    header.removeFromLeft (12);
    g.drawText ("TAGS", header.removeFromLeft (200), juce::Justification::centredLeft, false);
    drawIcon (g, Icon::starFilled, juce::Rectangle<float> ((float) viewport.getRight() - 42.0f, (float) viewport.getY() - 24.0f, 14.0f, 14.0f), colours::text);
    g.setColour (colours::outline);
    g.drawHorizontalLine (viewport.getY() - 1, (float) viewport.getX(), (float) viewport.getRight());

    // footer
    const auto footer = juce::Rectangle<int> (viewport.getX(), viewport.getBottom() + 6, viewport.getWidth() - 120, 40);
    g.setColour (colours::textDim);
    g.setFont (uiFont (13.0f, false));
    g.drawText (juce::CharPointer_UTF8 ("\xe2\x86\x91\xe2\x86\x93 browse  \xc2\xb7  Enter load  \xc2\xb7  Esc close"), footer.withTrimmedLeft (10),
                juce::Justification::centredLeft, false);
    g.drawText (juce::String (processor.getLibrary().getNumPresets()) + " presets", footer, juce::Justification::centredRight, false);

    // hover preview card (placed so it never covers the star column)
    if (table->hover >= 0 && table->hover < (int) rows.size())
    {
        const auto& p = processor.getLibrary().getPreset (rows[(size_t) table->hover]);
        const auto rowY = viewport.getY() + table->hover * Table::kRowH - viewport.getViewPositionY();
        auto card = juce::Rectangle<float> (300.0f, 96.0f);
        const auto x = (float) viewport.getRight() - 300.0f - 64.0f;
        auto y = (float) rowY + Table::kRowH + 2.0f;
        if (y + card.getHeight() > (float) viewport.getBottom())
            y = (float) rowY - card.getHeight() - 2.0f;
        card.setPosition (x, y);

        drawGlow (g, card, 6.0f, colours::accent, 1.0f, 4);
        g.setColour (colours::background);
        g.fillRoundedRectangle (card, 6.0f);
        g.setColour (colours::accent);
        g.drawRoundedRectangle (card, 6.0f, 1.0f);
        auto c = card.reduced (10.0f, 8.0f);
        auto top = c.removeFromTop (16.0f);
        g.setColour (colours::text);
        g.setFont (labelFont (12.0f));
        g.drawText (p.name, top, juce::Justification::centredLeft, true);
        g.setColour (colours::textDim);
        g.setFont (uiFont (11.0f, false));
        g.drawText (juce::String (juce::roundToInt (p.bpmHint)) + " BPM  " + juce::String (p.bars()) + (p.bars() == 1 ? " BAR" : " BARS"),
                    top, juce::Justification::centredRight, false);
        c.removeFromTop (6.0f);
        drawMiniPattern (g, p.pattern, c);
    }
}

void PresetBrowser::paintOverChildren (juce::Graphics& g)
{
    drawIcon (g, Icon::search, search.getBounds().toFloat().removeFromLeft (34).withSizeKeepingCentre (16, 16), colours::textDim);
}

void PresetBrowser::mouseDown (const juce::MouseEvent& e)
{
    if (! panelBounds().contains (e.getPosition()))
        close();
}

bool PresetBrowser::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        close();
        return true;
    }

    if (rows.empty())
        return false;

    auto move = [this] (int delta)
    {
        table->selected = juce::jlimit (0, (int) rows.size() - 1, table->selected < 0 ? 0 : table->selected + delta);
        const auto y = table->selected * Table::kRowH;
        if (y < viewport.getViewPositionY())
            viewport.setViewPosition (0, y);
        else if (y + Table::kRowH > viewport.getViewPositionY() + viewport.getHeight())
            viewport.setViewPosition (0, y + Table::kRowH - viewport.getHeight());
        load (rows[(size_t) table->selected]);
    };

    if (key == juce::KeyPress::downKey) { move (1); return true; }
    if (key == juce::KeyPress::upKey)   { move (-1); return true; }
    if (key == juce::KeyPress::returnKey && table->selected >= 0)
    {
        load (rows[(size_t) table->selected]);
        close();
        return true;
    }
    return false;
}

} // namespace rk::ui
