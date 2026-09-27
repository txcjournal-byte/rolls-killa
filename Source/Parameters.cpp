#include "Parameters.h"

namespace rk::params
{

juce::AudioProcessorValueTreeState::ParameterLayout createLayout (const juce::StringArray& hatNames)
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto percent = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " %"; })
                                                  .withLabel ("%");

    layout.add (std::make_unique<AudioParameterInt> (ParameterID { preset, 1 }, "Preset", 0, kMaxPresetIndex, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { rollSpeed, 1 }, "Roll Speed", rollSpeedChoices, 1));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { density, 1 }, "Density", NormalisableRange<float> (0.0f, 200.0f, 1.0f), 100.0f, percent));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { velMode, 1 }, "Velocity", velModeChoices, 0));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { groove, 1 }, "Groove", NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f, percent));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { pitchRamp, 1 }, "Pitch Ramp", -12, 12, 0,
                                                     AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return (v > 0 ? "+" : "") + String (v) + " st"; })));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { swing, 1 }, "Swing", NormalisableRange<float> (0.0f, 60.0f, 1.0f), 0.0f, percent));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { variation, 1 }, "Variation", NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f, percent));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { bars, 1 }, "Bars", barsChoices, 2));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { seed, 1 }, "Seed", 0, kMaxSeed, 0,
                                                     AudioParameterIntAttributes().withAutomatable (false)));

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { hat, 1 }, "Hat", hatNames, 2));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { tune, 1 }, "Tune", NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " st"; })));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { decay, 1 }, "Decay", NormalisableRange<float> (20.0f, 1000.0f, 1.0f, 0.5f), 1000.0f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return v >= 1000.0f ? String ("Full") : String (roundToInt (v)) + " ms"; })));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { choke, 1 }, "Choke", true));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { volume, 1 }, "Volume", NormalisableRange<float> (-48.0f, 6.0f, 0.1f, 2.0f), 0.0f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })));
    return layout;
}

const juce::StringArray& patternParameterIds()
{
    static const juce::StringArray ids { preset, rollSpeed, density, velMode, groove, pitchRamp, swing, variation, bars, seed };
    return ids;
}

} // namespace rk::params
