cmake_minimum_required(VERSION 3.22)
project(DarkPrince VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(DarkPrince
    COMPANY_NAME "Personal"
    PLUGIN_MANUFACTURER_CODE Pers
    PLUGIN_CODE Dkpr
    FORMATS VST3 Standalone
    PRODUCT_NAME "Dark Prince"
    VST3_CATEGORIES Fx
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(DarkPrince PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(DarkPrince PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(DarkPrince
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(MSVC)
    target_compile_options(DarkPrince PRIVATE /utf-8)
endif()
