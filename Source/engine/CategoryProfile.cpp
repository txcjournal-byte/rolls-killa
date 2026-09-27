#include "CategoryProfile.h"

#include <cstring>

namespace rk
{

//                                              1/24   1/32   1/48   1/64   1/96
static const CategoryProfile kProfiles[kNumFactoryCategories + 1] {
    { "PRIMITIVUS",   130, 160, {{ 1.0f,  1.0f,  0.0f,  0.0f,  0.0f  }}, { 0.25, 0.5 },               false, false, 110, 127, 0.0f,  {},                 0.0f,  0.35f, 1 },
    { "LIBER TRAP",   130, 150, {{ 0.5f,  0.35f, 0.0f,  0.15f, 0.0f  }}, { 2.0 / 3.0, 0.5, 0.375 },   false, false,  85, 105, 0.2f,  { 12 },             0.3f,  0.8f,  0 },
    { "TRINITAS",     135, 155, {{ 0.7f,  0.0f,  0.3f,  0.0f,  0.0f  }}, { 2.0 / 3.0, 1.0 / 3.0, 1.0 },false, false,  90, 105, 0.1f,  { 3, 5 },           0.3f,  0.9f,  0 },
    { "VOMITORIUM",   150, 165, {{ 0.0f,  1.0f,  0.0f,  0.1f,  0.0f  }}, { 0.25, 0.375, 0.5 },        false, false, 108, 122, 0.05f, { 2, 3, 5 },        0.3f,  0.45f, 0 },
    { "BLAST RITUAL", 140, 165, {{ 0.0f,  0.0f,  0.1f,  0.5f,  0.4f  }}, { 1.0, 1.5, 0.5 },           false, false,  90, 127, 0.7f,  { 12 },             0.5f,  1.1f,  0 },
    { "AURA FARM",    140, 160, {{ 0.7f,  0.3f,  0.0f,  0.0f,  0.0f  }}, { 2.0 / 3.0, 0.5 },          false, false,  70,  95, 0.15f, { 3, 5, 7 },        0.6f,  0.5f,  0 },
    { "DELIRIUM",     140, 155, {{ 0.1f,  0.0f,  0.2f,  0.4f,  0.3f  }}, { 0.5, 0.75, 1.0 },          false, false,  50,  80, 0.4f,  { 7, 12, 19, 24 },  0.7f,  1.0f,  0 },
    { "NECRODRILL",   140, 145, {{ 1.0f,  0.0f,  0.0f,  0.0f,  0.0f  }}, { 0.5 },                     true,  false,  75, 120, 0.0f,  { 2 },              0.1f,  0.9f,  0 },
    { "MOTOR MORTIS", 130, 145, {{ 0.0f,  0.8f,  0.0f,  0.2f,  0.0f  }}, { 0.25, 0.375, 0.5 },        true,  false,  70, 115, 0.2f,  { 2, 4, 7 },        0.4f,  0.8f,  0 },
    { "SPASMUS",      140, 155, {{ 0.0f,  1.0f,  0.0f,  0.0f,  0.0f  }}, { 0.25, 0.375 },             true,  false,  80, 105, 0.0f,  {},                 0.0f,  0.7f,  0 },
    { "REQUIEM",       60,  80, {{ 0.5f,  0.5f,  0.0f,  0.0f,  0.0f  }}, { 0.5, 2.0 / 3.0, 1.0 },     false, false,  90, 100, 0.0f,  { 12, -12 },        0.4f,  0.7f,  0 },
    { "MANIA",        140, 170, {{ 0.3f,  0.1f,  0.0f,  0.3f,  0.3f  }}, { 1.0, 0.75 },               true,  true,   60, 127, 0.6f,  { 12, -12, -7 },    0.6f,  1.0f,  0 },
    // User presets: neutral trap profile
    { "USER",         60,  180, {{ 0.45f, 0.25f, 0.1f,  0.1f,  0.1f  }}, { 2.0 / 3.0, 0.5 },          true,  false,  80, 110, 0.2f,  { 3, 5, 7, 12 },    0.4f,  0.75f, 0 },
};

const CategoryProfile& getCategoryProfile (int categoryIndex)
{
    if (categoryIndex < 0 || categoryIndex > kUserCategory)
        categoryIndex = kUserCategory;

    return kProfiles[categoryIndex];
}

int findCategoryIndex (const char* name)
{
    for (int i = 0; i <= kUserCategory; ++i)
        if (std::strcmp (kProfiles[i].name, name) == 0)
            return i;

    return -1;
}

} // namespace rk
