#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace rk::params
{

// Parameter IDs (never rename - they are stored in DAW projects)
inline constexpr const char* preset    = "preset";
inline constexpr const char* rollSpeed = "rollSpeed";
inline constexpr const char* density   = "density";
inline constexpr const char* velMode   = "velMode";
inline constexpr const char* groove    = "groove";
inline constexpr const char* pitchRamp = "pitchRamp";
inline constexpr const char* swing     = "swing";
inline constexpr const char* variation = "variation";
inline constexpr const char* bars      = "bars";
inline constexpr const char* seed      = "seed";
inline constexpr const char* hat       = "hat";
inline constexpr const char* tune      = "tune";
inline constexpr const char* decay     = "decay";
inline constexpr const char* choke     = "choke";
inline constexpr const char* volume    = "volume";
inline constexpr const char* playMode  = "playMode";
inline constexpr const char* puff      = "puff";      // blunt smoke FX (Rolls Killa Mini)

inline constexpr int kMaxPresetIndex = 511;
inline constexpr int kMaxSeed = 99999;

inline const juce::StringArray rollSpeedChoices { "Slower", "Original", "Faster" };
inline const juce::StringArray velModeChoices { "Original", "Flat", "Ramp Up", "Ramp Down" };
inline const juce::StringArray barsChoices { "1", "2", "4", "8" };
// MIDI = plays only while a note is held on the channel (host-synced); Host = plays whenever the host plays
inline const juce::StringArray playModeChoices { "MIDI", "Host" };
inline constexpr int kPlayModeMidi = 0;

inline int barsFromChoice (int choice) { return 1 << juce::jlimit (0, 3, choice); }
inline int choiceFromBars (int numBars) { return numBars >= 8 ? 3 : numBars >= 4 ? 2 : numBars >= 2 ? 1 : 0; }

juce::AudioProcessorValueTreeState::ParameterLayout createLayout (const juce::StringArray& hatNames);

/** IDs of the parameters that change the pattern (as opposed to the sampler). */
const juce::StringArray& patternParameterIds();

} // namespace rk::params
