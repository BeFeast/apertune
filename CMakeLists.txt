cmake_minimum_required(VERSION 3.22)

project(Apertune
    VERSION 0.1.0
    DESCRIPTION "Apertune chromatic tuner plugin"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.13
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

FetchContent_Declare(
    clap_juce_extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG 51a9359315298de632cf44e9d7524940868441e6
    GIT_SUBMODULES_RECURSE TRUE)
FetchContent_MakeAvailable(clap_juce_extensions)

add_library(apertune_core
    src/PitchMath.cpp)
set_target_properties(apertune_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(apertune_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(APERTUNE_PLUGIN_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND APERTUNE_PLUGIN_FORMATS AU)
endif()

juce_add_plugin(Apertune
    COMPANY_NAME "BeFeast"
    BUNDLE_ID "com.befeast.apertune"
    PLUGIN_MANUFACTURER_CODE Bfst
    PLUGIN_CODE Aptn
    PRODUCT_NAME "Apertune"
    DESCRIPTION "Chromatic tuner for guitar and bass"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS ${APERTUNE_PLUGIN_FORMATS})

target_sources(Apertune
    PRIVATE
        src/PluginEditor.cpp
        src/PluginProcessor.cpp)

target_compile_definitions(Apertune
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Apertune
    PRIVATE
        apertune_core
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

clap_juce_extensions_plugin(TARGET Apertune
    CLAP_ID "com.befeast.apertune"
    CLAP_FEATURES audio-effect analyzer tuner)

enable_testing()

add_executable(apertune_pitch_math_tests
    tests/PitchMathTests.cpp)
target_link_libraries(apertune_pitch_math_tests
    PRIVATE
        apertune_core)

add_test(NAME apertune_pitch_math COMMAND apertune_pitch_math_tests)
