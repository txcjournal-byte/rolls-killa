#include "PluginProcessor.h"
#include "PluginEditor.h"

RollsKillaProcessor::RollsKillaProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void RollsKillaProcessor::prepareToPlay (double, int) {}

bool RollsKillaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void RollsKillaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
}

juce::AudioProcessorEditor* RollsKillaProcessor::createEditor()
{
    return new RollsKillaEditor (*this);
}

void RollsKillaProcessor::getStateInformation (juce::MemoryBlock&) {}
void RollsKillaProcessor::setStateInformation (const void*, int) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RollsKillaProcessor();
}
