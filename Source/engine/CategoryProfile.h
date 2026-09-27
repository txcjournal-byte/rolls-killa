#pragma once

#include "Pattern.h"

#include <array>
#include <vector>

namespace rk
{

constexpr int kNumFactoryCategories = 12;
constexpr int kUserCategory = kNumFactoryCategories;   // index of the "User" pseudo category

/** Musical profile of a category (section 3.3 of the spec). Drives validation, KILL and Density. */
struct CategoryProfile
{
    const char* name;               // display name, also the "category" field in preset JSON
    int bpmMin, bpmMax;
    std::array<float, numRollRates> rateWeights;   // which roll rates are idiomatic
    std::vector<double> rollLengths;               // typical roll lengths in beats
    bool allowSkipStarts;           // rolls may start off the 1/8 grid
    bool mixedRates;                // rolls may change speed half-way
    int velMin, velMax;
    float rampChance;               // chance a new roll gets a velocity ramp
    std::vector<int> pitchSteps;    // intervals used for pitched rolls
    float pitchChance;              // chance a new roll is pitched
    float rollsPerBar;              // target roll density
    int maxRollsPerTwoBars;         // hard limit, 0 = none
};

const CategoryProfile& getCategoryProfile (int categoryIndex);
int findCategoryIndex (const char* name);

} // namespace rk
