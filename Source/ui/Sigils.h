#pragma once

#include "Theme.h"

namespace rk::ui
{

/** Line-art sigil of a category (0..11), 12 = USER, 13 = FAVORITES. */
void drawSigil (juce::Graphics& g, int category, juce::Rectangle<float> area, juce::Colour colour, float stroke = 1.4f);

/** "ROLLS KILLA" logo as a path (custom angular glyphs, no font files). Height of caps = 16 units. */
juce::Path createLogoPath();

/** Renders the logo with gradient + glow into an image of the given height (cached by the caller). */
juce::Image renderLogo (int height, float scale);

} // namespace rk::ui
