#include "PresetBar.h"

#include "../PluginProcessor.h"

namespace rk::ui
{

PresetBar::PresetBar (RollsKillaProcessor& p) : processor (p)
{
    addAndMakeVisible (prev);
    addAndMakeVisible (next);
    addAndMakeVisible (star);
    prev.onClick = [this] { processor.stepPreset (-1); refresh(); };
    next.onClick = [this] { processor.stepPreset (1); refresh(); };
    star.onClick = [this]
    {
        auto& lib = processor.getLibrary();
        const auto idx = processor.getModel().getSettings().presetIndex;
        lib.setFavorite (idx, ! lib.isFavorite (idx));
        refresh();
    };
    prev.setTooltip ("Previous preset");
    next.setTooltip ("Next preset");
    star.setTooltip ("Add to favorites");
    refresh();
}

void PresetBar::refresh()
{
    const auto idx = processor.getModel().getSettings().presetIndex;
    const auto fav = processor.getLibrary().isFavorite (idx);
    star.setIcon (fav ? Icon::starFilled : Icon::star);
    star.iconColour = fav ? colours::accent : colours::textDim;
    repaint();
}

juce::Rectangle<int> PresetBar::categoryArea() const
{
    if (compact)
        return { 92, 0, (getWidth() - 92) * 2 / 5, getHeight() };
    return { 206, 0, 184, getHeight() };
}

juce::Rectangle<int> PresetBar::presetArea() const
{
    const auto x = categoryArea().getRight() + 10;
    return { x, 0, getWidth() - x, getHeight() };
}

void PresetBar::resized()
{
    const auto h = getHeight();
    const auto x0 = compact ? 4 : 110;
    prev.setBounds (x0, (h - 32) / 2, 38, 32);
    next.setBounds (x0 + 46, (h - 32) / 2, 38, 32);
    const auto pa = presetArea();
    star.setBounds (pa.getRight() - 58, (h - 24) / 2, 24, 24);
}

void PresetBar::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat(), 6.0f);
    const auto& preset = processor.getModel().getPreset();

    if (! compact)
    {
        g.setColour (colours::text);
        g.setFont (labelFont (15.0f));
        g.drawText ("PRESETS", juce::Rectangle<int> (18, 0, 90, getHeight()), juce::Justification::centredLeft, false);
    }

    g.setColour (colours::outline);
    g.drawVerticalLine (categoryArea().getX() - 6, 8.0f, (float) getHeight() - 8.0f);
    g.drawVerticalLine (presetArea().getX() - 6, 8.0f, (float) getHeight() - 8.0f);

    auto drawField = [&] (juce::Rectangle<int> area, const juce::String& caption, const juce::String& value, bool hot, int rightInset)
    {
        if (hot)
        {
            g.setColour (colours::panelLight.brighter (0.05f));
            g.fillRoundedRectangle (area.reduced (0, 5).toFloat(), 4.0f);
        }
        auto r = area.reduced (10, 6);
        g.setColour (colours::textDim);
        g.setFont (labelFont (10.0f));
        g.drawText (caption, r.removeFromTop (12), juce::Justification::centredLeft, false);
        g.setColour (colours::text);
        g.setFont (uiFont (16.5f, false));
        g.drawText (value, r.withTrimmedRight (rightInset), juce::Justification::centredLeft, true);
        drawIcon (g, Icon::chevronDown, area.removeFromRight (30).withSizeKeepingCentre (14, 14).toFloat(), colours::text);
    };

    const auto category = getCategoryProfile (preset.category).name;
    drawField (categoryArea(), "CATEGORY", juce::String (category), hover == 1, 30);
    drawField (presetArea(), "PRESET", preset.name, hover == 2, 64);
}

void PresetBar::mouseMove (const juce::MouseEvent& e)
{
    const auto h = categoryArea().contains (e.getPosition()) ? 1 : presetArea().contains (e.getPosition()) ? 2 : 0;
    if (h != hover)
    {
        hover = h;
        setMouseCursor (h != 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void PresetBar::mouseExit (const juce::MouseEvent&)
{
    hover = 0;
    repaint();
}

void PresetBar::mouseUp (const juce::MouseEvent& e)
{
    if ((categoryArea().contains (e.getPosition()) || presetArea().contains (e.getPosition())) && onOpenBrowser != nullptr)
        onOpenBrowser (processor.getModel().getPreset().category);
}

} // namespace rk::ui
