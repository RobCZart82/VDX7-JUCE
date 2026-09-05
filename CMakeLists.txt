cmake_minimum_required(VERSION 3.22)

project(VDX7_JUCE
    VERSION 0.6.6
    DESCRIPTION "Minimal JUCE VST3/AU wrapper for the VDX7 Yamaha DX7 Mk I emulation core"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# JUCE is used only as the cross-platform plugin/GUI wrapper.
# This public release uses JUCE under AGPLv3.
# Corresponding-source archives include offline dependency sources.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/JUCE/CMakeLists.txt")
    set(FETCHCONTENT_SOURCE_DIR_JUCE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/JUCE")
endif()
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(JUCE)

# Pull the maintained portable VDX7 port from Retromulator.
# We deliberately compile ONLY its dx7Lib core files, not Retromulator itself.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/dx7Lib/dx7.cpp")
    set(VDX7_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/dx7Lib")
else()
FetchContent_Declare(
    retromulator
    GIT_REPOSITORY https://github.com/reales/retromulator.git
    GIT_TAG d5473776a0449d60a997b91bdc888598a33265ac
    GIT_SHALLOW FALSE
    # Populate the repository, but do not configure Retromulator's own project.
    SOURCE_SUBDIR _vdx7_wrapper_no_cmakelists_here
)
FetchContent_MakeAvailable(retromulator)

set(VDX7_CORE_DIR "${retromulator_SOURCE_DIR}/source/dx7Lib")
endif()

add_library(vdx7_core STATIC
    "${VDX7_CORE_DIR}/HD6303R.cpp"
    "${VDX7_CORE_DIR}/HD6303R_inst.cpp"
    "${VDX7_CORE_DIR}/dx7.cpp"
)

add_library(vdx7_voice_data STATIC
    Source/VDX7VoiceData.h
    Source/VDX7VoiceData.cpp
    Source/VDX7Sysex.cpp
)
target_include_directories(vdx7_voice_data PUBLIC Source)
target_compile_features(vdx7_voice_data PUBLIC cxx_std_20)
set_target_properties(vdx7_voice_data PROPERTIES POSITION_INDEPENDENT_CODE ON)

enable_testing()
add_executable(vdx7_voice_data_tests Tests/VDX7VoiceDataTests.cpp)
target_link_libraries(vdx7_voice_data_tests PRIVATE vdx7_voice_data)
add_test(NAME vdx7_voice_data COMMAND vdx7_voice_data_tests)
add_executable(vdx7_algorithm_tests Tests/VDX7AlgorithmsTests.cpp)
target_include_directories(vdx7_algorithm_tests PRIVATE Source)
add_test(NAME vdx7_algorithms COMMAND vdx7_algorithm_tests "${VDX7_CORE_DIR}/OPS.h")

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

# The GUI asset pack is compiled into the plug-in so that every format is
# self-contained and does not depend on loose PNG files at runtime.
file(GLOB_RECURSE VDX7_GUI_ASSETS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/GUI/1x/*.png"
)

juce_add_binary_data(VDX7Assets
    NAMESPACE VDX7Assets
    SOURCES ${VDX7_GUI_ASSETS}
)

target_sources(VDX7 PRIVATE
    Source/VDX7Engine.h
    Source/VDX7Engine.cpp
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
    Source/VDX7Algorithms.h
    Source/VDX7AlgorithmView.h
    Source/VDX7MechanicalDrawing.h
    Source/VDX7LookAndFeel.h
    Source/VDX7LookAndFeel.cpp
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
    vdx7_voice_data
    VDX7Assets
    juce::juce_audio_utils
    juce::juce_gui_extra
)

# Opt-in integration runner: uses the user's ROM locally, never bundles it.
add_executable(vdx7_processor_tests EXCLUDE_FROM_ALL Tests/VDX7ProcessorTests.cpp)
target_link_libraries(vdx7_processor_tests PRIVATE VDX7)
target_include_directories(vdx7_processor_tests PRIVATE
    $<TARGET_PROPERTY:VDX7,INCLUDE_DIRECTORIES>)
target_compile_definitions(vdx7_processor_tests PRIVATE
    $<TARGET_PROPERTY:VDX7,COMPILE_DEFINITIONS>)

if(APPLE)
    # Ad-hoc signing is sufficient for a local REAPER test build.
    set_target_properties(VDX7 PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    )
endif()
