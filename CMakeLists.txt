cmake_minimum_required(VERSION 3.22)
project(AtelierDebussy VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")

option(AD_COPY "Copy plugin into your system plugin folder after build" OFF)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

if(APPLE)
    set(AD_FORMATS VST3 AU Standalone)
else()
    set(AD_FORMATS VST3 Standalone)
endif()

juce_add_plugin(AtelierDebussy
    COMPANY_NAME "Atelier"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${AD_COPY}
    PLUGIN_MANUFACTURER_CODE Atlr
    PLUGIN_CODE Dbsy
    FORMATS ${AD_FORMATS}
    PRODUCT_NAME "Atelier Debussy")

juce_generate_juce_header(AtelierDebussy)

target_sources(AtelierDebussy PRIVATE
    Source/Generator.h
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp)

target_compile_definitions(AtelierDebussy PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(AtelierDebussy
    PRIVATE juce::juce_audio_utils
    PUBLIC  juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

if(MSVC)
    target_compile_options(AtelierDebussy PRIVATE /utf-8)
endif()
