#pragma once

#include "PluginProcessor.h"

class RollsKillaEditor : public juce::AudioProcessorEditor
{
public:
    explicit RollsKillaEditor (RollsKillaProcessor&);

    void paint (juce::Graphics&) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RollsKillaEditor)
};
