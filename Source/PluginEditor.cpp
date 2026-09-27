#include "PluginEditor.h"

RollsKillaEditor::RollsKillaEditor (RollsKillaProcessor& p)
    : AudioProcessorEditor (&p)
{
    setSize (900, 560);
}

void RollsKillaEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0e0e10));
    g.setColour (juce::Colour (0xffff2e3e));
    g.setFont (juce::FontOptions (48.0f, juce::Font::bold));
    g.drawText ("ROLLS KILLA", getLocalBounds(), juce::Justification::centred);
}
