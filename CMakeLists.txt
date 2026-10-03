cmake_minimum_required(VERSION 3.22)

project(RollsKilla VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(ROLLSKILLA_BUILD_TESTS "Build the unit test runner" ON)

if(MSVC)
    # Static runtime: the .vst3 works without the VC++ redistributable.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ---------------------------------------------------------------------------
# JUCE (pinned tag). Set FETCHCONTENT_SOURCE_DIR_JUCE to use a local checkout.
# ---------------------------------------------------------------------------
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.9
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------
# Embedded resources
# ---------------------------------------------------------------------------
file(GLOB ROLLSKILLA_PRESET_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Presets/Factory/*.json")
file(GLOB ROLLSKILLA_SAMPLE_FILES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Samples/*.wav")

juce_add_binary_data(RollsKillaPresetData
    HEADER_NAME PresetData.h
    NAMESPACE PresetData
    SOURCES ${ROLLSKILLA_PRESET_FILES} Resources/Presets/README.txt)

juce_add_binary_data(RollsKillaSampleData
    HEADER_NAME SampleData.h
    NAMESPACE SampleData
    SOURCES ${ROLLSKILLA_SAMPLE_FILES} Resources/Samples/README.txt)

# Rolls Killa Mini skin (made by tools/mini_skin/make_skin.py from docs/design/mini_metal.webp)
juce_add_binary_data(RollsKillaMiniSkin
    HEADER_NAME MiniSkin.h
    NAMESPACE MiniSkin
    SOURCES
        Resources/MiniSkin/plate.jpg
        Resources/MiniSkin/ring.png
        Resources/MiniSkin/lamp_on.png
        Resources/MiniSkin/lamp_off.png
        Resources/MiniSkin/dot_on.png
        Resources/MiniSkin/dot_off.png
        Resources/MiniSkin/mood_on.png
        Resources/MiniSkin/mood_off.png
        Resources/MiniSkin/toggle_midi.png
        Resources/MiniSkin/toggle_host.png)

# ---------------------------------------------------------------------------
# Engine sources (no GUI) - shared by the plugin and the test runner
# ---------------------------------------------------------------------------
set(ROLLSKILLA_ENGINE_SOURCES
    Source/engine/Pattern.cpp
    Source/engine/CategoryProfile.cpp
    Source/engine/RollLibrary.cpp
    Source/engine/PresetValidator.cpp
    Source/engine/HatSynth.cpp
    Source/engine/HatSampler.cpp
    Source/engine/PatternPlayer.cpp
    Source/engine/RollModel.cpp
    Source/engine/PatternShaper.cpp
    Source/engine/VariationEngine.cpp
    Source/engine/MidiExport.cpp
    Source/engine/SmokeFx.cpp
    Source/engine/DrumSynth.cpp
    Source/engine/DrumPatterns.cpp
    Source/engine/DrumKit.cpp
    Source/engine/KitSampler.cpp
    Source/engine/KitExport.cpp
)

set(ROLLSKILLA_COMMON_DEFINITIONS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

set(ROLLSKILLA_ENGINE_MODULES
    juce::juce_core
    juce::juce_data_structures
    juce::juce_audio_basics
    juce::juce_audio_formats
)

# ---------------------------------------------------------------------------
# Plugin
# ---------------------------------------------------------------------------
set(ROLLSKILLA_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND ROLLSKILLA_FORMATS AU)
endif()

# Both plugins share the processor and the engine; the Mini one has its own small editor.
set(ROLLSKILLA_PLUGIN_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/MiniEditor.cpp
    Source/Parameters.cpp
    Source/ui/KillaLookAndFeel.cpp
    Source/ui/RollVisualizer.cpp
    Source/ui/PresetBar.cpp
    Source/ui/PresetBrowser.cpp
    Source/ui/Sigils.cpp
    Source/ui/Widgets.cpp
    Source/ui/MetalWidgets.cpp
    Source/ui/SkinWidgets.cpp
    ${ROLLSKILLA_ENGINE_SOURCES}
)

function(rollskilla_add_plugin target code name bundle description mini)
    juce_add_plugin(${target}
        COMPANY_NAME                "Killa"
        COMPANY_WEBSITE             ""
        BUNDLE_ID                   ${bundle}
        PLUGIN_MANUFACTURER_CODE    Klla
        PLUGIN_CODE                 ${code}
        PRODUCT_NAME                "${name}"
        DESCRIPTION                 "${description}"
        FORMATS                     ${ROLLSKILLA_FORMATS}
        IS_SYNTH                    TRUE
        NEEDS_MIDI_INPUT            TRUE
        NEEDS_MIDI_OUTPUT           TRUE
        IS_MIDI_EFFECT              FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COPY_PLUGIN_AFTER_BUILD     FALSE
        VST3_CATEGORIES             Instrument Drum Generator
        AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    )

    target_sources(${target} PRIVATE ${ROLLSKILLA_PLUGIN_SOURCES})
    target_include_directories(${target} PRIVATE Source)
    target_compile_definitions(${target} PUBLIC ${ROLLSKILLA_COMMON_DEFINITIONS} ROLLSKILLA_MINI=${mini})

    target_link_libraries(${target}
        PRIVATE
            RollsKillaPresetData
            RollsKillaSampleData
            RollsKillaMiniSkin
            ${ROLLSKILLA_ENGINE_MODULES}
            juce::juce_audio_utils
            juce::juce_audio_processors
            juce::juce_gui_basics
            juce::juce_gui_extra
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )
endfunction()

rollskilla_add_plugin(RollsKilla     RlKl "Rolls Killa"      com.killa.rollskilla     "Hi-Hat Roll Presets"            0)
rollskilla_add_plugin(RollsKillaMini RlKm "Rolls Killa Mini" com.killa.rollskillamini "Hi-Hat Roll Presets - one KILL" 1)

# ---------------------------------------------------------------------------
# Tests (JUCE UnitTest, no GUI)
# ---------------------------------------------------------------------------
if(ROLLSKILLA_BUILD_TESTS)
    enable_testing()

    juce_add_console_app(RollsKillaTests PRODUCT_NAME "RollsKillaTests")

    target_sources(RollsKillaTests PRIVATE
        Tests/TestMain.cpp
        Tests/LibraryTests.cpp
        Tests/SamplerTests.cpp
        Tests/PlayerTests.cpp
        Tests/ShaperTests.cpp
        Tests/MidiExportTests.cpp
        Tests/SmokeFxTests.cpp
        Tests/DrumSynthTests.cpp
        Tests/DrumPatternTests.cpp
        Tests/KitTests.cpp
        ${ROLLSKILLA_ENGINE_SOURCES}
    )

    target_include_directories(RollsKillaTests PRIVATE Source)

    target_compile_definitions(RollsKillaTests PRIVATE
        ${ROLLSKILLA_COMMON_DEFINITIONS}
        ROLLSKILLA_PRESET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Resources/Presets/Factory"
    )

    target_link_libraries(RollsKillaTests
        PRIVATE
            RollsKillaPresetData
            RollsKillaSampleData
            ${ROLLSKILLA_ENGINE_MODULES}
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )

    add_test(NAME RollsKillaTests COMMAND RollsKillaTests)
endif()

# ---------------------------------------------------------------------------
# Offline renderer (listening tests + processor smoke test)
# ---------------------------------------------------------------------------
if(ROLLSKILLA_BUILD_TESTS AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.27)
    juce_add_console_app(RollsKillaRender PRODUCT_NAME "RollsKillaRender")
    target_sources(RollsKillaRender PRIVATE tools/render/RenderMain.cpp)
    # Links the plugin's shared code directly (JUCE modules are already compiled into it).
    target_include_directories(RollsKillaRender PRIVATE Source "${juce_SOURCE_DIR}/modules"
        "$<TARGET_PROPERTY:RollsKilla,INCLUDE_DIRECTORIES>")
    target_compile_definitions(RollsKillaRender PRIVATE
        "$<LIST:FILTER,$<TARGET_PROPERTY:RollsKilla,COMPILE_DEFINITIONS>,EXCLUDE,^JUCE_STANDALONE_APPLICATION>")
    target_link_libraries(RollsKillaRender PRIVATE RollsKilla juce::juce_recommended_config_flags)
    add_test(NAME RollsKillaRenderCheck COMMAND RollsKillaRender render-check --check)
endif()
