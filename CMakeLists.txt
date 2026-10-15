cmake_minimum_required(VERSION 3.22)

# Version lives here only
project(HAT_GOBLIN VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# --- Identity ---------------------------------------------------------------
# NEVER change the CODES after release, hosts identify saved projects by them.
set(HG_COMPANY_NAME      "TrapWin")
set(HG_MANUFACTURER_CODE "Trwn")
set(HG_PLUGIN_CODE       "Hgob")

# --- JUCE -------------------------------------------------------------------
set(HG_JUCE_DIR "" CACHE PATH "Local JUCE checkout (optional)")
if(HG_JUCE_DIR)
    add_subdirectory(${HG_JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

option(HG_HEADLESS "Linux CI/test build without ALSA/JACK" OFF)

juce_add_plugin(HatGoblin
    COMPANY_NAME "${HG_COMPANY_NAME}"
    COMPANY_WEBSITE "https://trapwin.com"
    BUNDLE_ID com.trapwin.hatgoblin
    PLUGIN_MANUFACTURER_CODE ${HG_MANUFACTURER_CODE}
    PLUGIN_CODE ${HG_PLUGIN_CODE}
    PRODUCT_NAME "HAT GOBLIN"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Instrument Drum
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    FORMATS VST3 AU Standalone
    COPY_PLUGIN_AFTER_BUILD FALSE)

file(GLOB HG_HAT_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/hats/*.wav)
list(SORT HG_HAT_FILES)
juce_add_binary_data(HatGoblinData SOURCES
    ${HG_HAT_FILES}
    assets/trapwin_logo.png
    assets/fonts/Anton-Regular.ttf
    assets/fonts/IBMPlexMono-Bold.ttf
    assets/fonts/UnifrakturCook-Bold.ttf
    assets/fonts/VT323-Regular.ttf)

set(HG_SOURCES
    Source/HatLibrary.cpp
    Source/RollEngine.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

set(HG_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_USE_FLAC=1
    JUCE_DISPLAY_SPLASH_SCREEN=0
    HG_VERSION_STRING="${PROJECT_VERSION}")
if(HG_HEADLESS)
    list(APPEND HG_DEFS JUCE_ALSA=0 JUCE_JACK=0)
endif()

target_sources(HatGoblin PRIVATE ${HG_SOURCES})
target_compile_definitions(HatGoblin PUBLIC ${HG_DEFS})
target_link_libraries(HatGoblin
    PRIVATE
        HatGoblinData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# --- Offline render test ----------------------------------------------------
option(HG_BUILD_TESTS "Build render test" ON)
if(HG_BUILD_TESTS)
    juce_add_console_app(HatGoblinTests PRODUCT_NAME "HatGoblinTests")
    target_sources(HatGoblinTests PRIVATE tests/RenderTest.cpp ${HG_SOURCES})
    target_compile_definitions(HatGoblinTests PRIVATE ${HG_DEFS} JucePlugin_Name="HAT GOBLIN" HG_TEST_BUILD=1)
    target_link_libraries(HatGoblinTests PRIVATE
        HatGoblinData
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()
