#pragma once

#include "Widgets.h"

class RollsKillaProcessor;

namespace rk::ui
{

/** PRESETS [<][>] | CATEGORY | PRESET * - click opens the browser. */
class PresetBar : public juce::Component
{
public:
    explicit PresetBar (RollsKillaProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    void refresh();

    std::function<void (int category)> onOpenBrowser;

private:
    juce::Rectangle<int> categoryArea() const;
    juce::Rectangle<int> presetArea() const;

    RollsKillaProcessor& processor;
    IconButton prev { "Previous preset", Icon::prev }, next { "Next preset", Icon::next };
    IconButton star { "Favorite", Icon::star, false };
    int hover = 0;   // 1 = category, 2 = preset
};

} // namespace rk::ui
