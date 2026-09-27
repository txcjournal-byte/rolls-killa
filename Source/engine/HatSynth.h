#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace rk
{

/** Synthesizes the placeholder closed hats used until the Killa Drum Kit samples are added.
    Deterministic: the same variant always produces the same sound. */
juce::AudioBuffer<float> synthesizeClosedHat (int variant, double sampleRate = 44100.0);

} // namespace rk
