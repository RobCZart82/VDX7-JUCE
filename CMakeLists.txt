cmake_minimum_required(VERSION 3.22)

project(VDX7_JUCE
    VERSION 0.1.0
    DESCRIPTION "Minimal JUCE VST3/AU wrapper for the VDX7 Yamaha DX7 Mk I emulation core"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# JUCE is used only as the cross-platform plugin/GUI wrapper.
# JUCE 9 is available under its current AGPLv3/commercial licensing terms;
# review those terms before distributing binaries.
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# Pull the maintained portable VDX7 port from Retromulator.
# We deliberately compile ONLY its dx7Lib core files, not Retromulator itself.
FetchContent_Declare(
    retromulator
    GIT_REPOSITORY https://github.com/reales/retromulator.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    # Populate the repository, but do not configure Retromulator's own project.
    SOURCE_SUBDIR _vdx7_wrapper_no_cmakelists_here
)
FetchContent_MakeAvailable(retromulator)

set(VDX7_CORE_DIR "${retromulator_SOURCE_DIR}/source/dx7Lib")

add_library(vdx7_core STATIC
    "${VDX7_CORE_DIR}/HD6303R.cpp"
    "${VDX7_CORE_DIR}/HD6303R_inst.cpp"
    "${VDX7_CORE_DIR}/dx7.cpp"
)

target_include_directories(vdx7_core PUBLIC "${VDX7_CORE_DIR}")
target_compile_features(vdx7_core PUBLIC cxx_std_20)
set_target_properties(vdx7_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    target_compile_options(vdx7_core PRIVATE /W3)
else()
    target_compile_options(vdx7_core PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

set(VDX7_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND VDX7_FORMATS AU)
endif()

juce_add_plugin(VDX7
    COMPANY_NAME "VDX7 Open Source"
    BUNDLE_ID "org.vdx7.prototype"
    PLUGIN_MANUFACTURER_CODE VdxP
    PLUGIN_CODE VdX7
    FORMATS ${VDX7_FORMATS}
    PRODUCT_NAME "VDX7"
    DESCRIPTION "Yamaha DX7 Mk I VDX7 hardware-emulation prototype"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
)

juce_generate_juce_header(VDX7)

target_sources(VDX7 PRIVATE
    Source/VDX7Engine.h
    Source/VDX7Engine.cpp
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
)

target_include_directories(VDX7 PRIVATE
    Source
    "${VDX7_CORE_DIR}"
)

target_compile_definitions(VDX7 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(VDX7 PRIVATE
    vdx7_core
    juce::juce_audio_utils
    juce::juce_gui_extra
)

if(APPLE)
    # Ad-hoc signing is sufficient for a local REAPER test build.
    set_target_properties(VDX7 PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    )
endif()
