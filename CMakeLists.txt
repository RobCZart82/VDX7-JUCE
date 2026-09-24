cmake_minimum_required(VERSION 3.22)

project(VDX7_JUCE
    VERSION 1.0.0
    DESCRIPTION "VDX7 Yamaha DX7 Mk I hardware-emulation instrument"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VDX7_RELEASE_BUILD "Build an accepted final release (not a development build)" OFF)
set(VDX7_DISPLAY_VERSION "${PROJECT_VERSION}-dev")
if(VDX7_RELEASE_BUILD)
    set(VDX7_DISPLAY_VERSION "${PROJECT_VERSION}")
endif()

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

# USER-bank persistence can be exercised on CI without firmware or GUI startup.
add_executable(vdx7_user_bank_tests Tests/VDX7UserBankTests.cpp Source/VDX7UserBank.cpp)
target_include_directories(vdx7_user_bank_tests PRIVATE Source)
target_link_libraries(vdx7_user_bank_tests PRIVATE juce::juce_core vdx7_voice_data)
target_compile_definitions(vdx7_user_bank_tests PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)
add_test(NAME vdx7_user_bank COMMAND vdx7_user_bank_tests)
set_tests_properties(vdx7_user_bank PROPERTIES TIMEOUT 20)
add_executable(vdx7_deferred_midi_tests Tests/VDX7DeferredMidiTests.cpp)
target_include_directories(vdx7_deferred_midi_tests PRIVATE Source)
add_test(NAME vdx7_deferred_midi COMMAND vdx7_deferred_midi_tests)
set_tests_properties(vdx7_deferred_midi PROPERTIES TIMEOUT 60)
add_executable(vdx7_latest_display_tests Tests/VDX7LatestDisplayTests.cpp)
target_include_directories(vdx7_latest_display_tests PRIVATE Source)
add_test(NAME vdx7_latest_display COMMAND vdx7_latest_display_tests)
set_tests_properties(vdx7_latest_display PROPERTIES TIMEOUT 60)
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
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/GUI/Vector/*.svg"
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
    Source/VDX7UserBank.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
    Source/VDX7AboutPanel.h
    Source/VDX7AboutPanel.cpp
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
    VDX7_DISPLAY_VERSION="${VDX7_DISPLAY_VERSION}"
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
add_executable(vdx7_midi_range_tests EXCLUDE_FROM_ALL
    Tests/VDX7MidiRangeTests.cpp Source/VDX7Engine.cpp)
target_include_directories(vdx7_midi_range_tests PRIVATE Source)
target_link_libraries(vdx7_midi_range_tests PRIVATE vdx7_core vdx7_voice_data)

add_executable(vdx7_timing_tests EXCLUDE_FROM_ALL
    Tests/VDX7TimingTests.cpp Source/VDX7Engine.cpp)
target_include_directories(vdx7_timing_tests PRIVATE Source)
target_link_libraries(vdx7_timing_tests PRIVATE vdx7_core vdx7_voice_data)

add_executable(vdx7_resampling_tests Tests/VDX7ResamplingTests.cpp Source/VDX7Engine.cpp)
target_include_directories(vdx7_resampling_tests PRIVATE Source)
target_link_libraries(vdx7_resampling_tests PRIVATE vdx7_core vdx7_voice_data)
add_test(NAME vdx7_resampling COMMAND vdx7_resampling_tests)
set_tests_properties(vdx7_resampling PROPERTIES TIMEOUT 60)

add_executable(vdx7_processor_tests EXCLUDE_FROM_ALL Tests/VDX7ProcessorTests.cpp)
target_link_libraries(vdx7_processor_tests PRIVATE VDX7)
target_include_directories(vdx7_processor_tests PRIVATE
    $<TARGET_PROPERTY:VDX7,INCLUDE_DIRECTORIES>)
target_compile_definitions(vdx7_processor_tests PRIVATE
    $<TARGET_PROPERTY:VDX7,COMPILE_DEFINITIONS>)

add_executable(vdx7_stability_tests EXCLUDE_FROM_ALL Tests/VDX7StabilityTests.cpp)
target_link_libraries(vdx7_stability_tests PRIVATE VDX7)
target_include_directories(vdx7_stability_tests PRIVATE
    $<TARGET_PROPERTY:VDX7,INCLUDE_DIRECTORIES>)
target_compile_definitions(vdx7_stability_tests PRIVATE
    $<TARGET_PROPERTY:VDX7,COMPILE_DEFINITIONS>)

option(VDX7_ENABLE_ROM_TESTS "Register local integration tests using your own ROM" OFF)
add_executable(vdx7_stress_tests EXCLUDE_FROM_ALL Tests/VDX7StressTests.cpp)
target_link_libraries(vdx7_stress_tests PRIVATE VDX7)
target_include_directories(vdx7_stress_tests PRIVATE $<TARGET_PROPERTY:VDX7,INCLUDE_DIRECTORIES>)
target_compile_definitions(vdx7_stress_tests PRIVATE $<TARGET_PROPERTY:VDX7,COMPILE_DEFINITIONS>)
set(VDX7_TEST_ROM_FILE "" CACHE FILEPATH "Local user-supplied ROM; never packaged")
if(VDX7_ENABLE_ROM_TESTS)
    if(NOT EXISTS "${VDX7_TEST_ROM_FILE}")
        message(FATAL_ERROR "VDX7_TEST_ROM_FILE must point to your local compatible ROM")
    endif()
    add_test(NAME vdx7_stability COMMAND vdx7_stability_tests "${VDX7_TEST_ROM_FILE}")
    add_test(NAME vdx7_midi_range COMMAND vdx7_midi_range_tests "${VDX7_TEST_ROM_FILE}")
    add_test(NAME vdx7_timing COMMAND vdx7_timing_tests "${VDX7_TEST_ROM_FILE}")
    add_test(NAME vdx7_processor COMMAND vdx7_processor_tests)
    add_test(NAME vdx7_stress COMMAND vdx7_stress_tests "${VDX7_TEST_ROM_FILE}")
    set_tests_properties(vdx7_stress PROPERTIES TIMEOUT 120)
    set_tests_properties(vdx7_processor PROPERTIES
        ENVIRONMENT "VDX7_TEST_ROM_PATH=${VDX7_TEST_ROM_FILE}")
    set_tests_properties(vdx7_stability vdx7_midi_range vdx7_timing vdx7_processor vdx7_stress
        PROPERTIES SKIP_RETURN_CODE 77 LABELS "local-rom")
endif()

# Shared by normal CI and exact-commit candidates. Every integration runner
# must compile even without a ROM; execution stays opt-in and local only.
add_custom_target(vdx7_ci_checks DEPENDS
    vdx7_user_bank_tests vdx7_deferred_midi_tests vdx7_latest_display_tests vdx7_voice_data_tests
    vdx7_algorithm_tests vdx7_resampling_tests vdx7_stress_tests
    vdx7_processor_tests vdx7_stability_tests vdx7_midi_range_tests vdx7_timing_tests)
add_custom_target(vdx7_all_tests DEPENDS vdx7_ci_checks)

if(APPLE)
    # Ad-hoc signing is sufficient for a local REAPER test build.
    set_target_properties(VDX7 PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    )
endif()

# N1 regression: compile on ROM-free CI, execute the firmware test only when
# the existing local-ROM suite is explicitly enabled.
add_executable(vdx7_host_reset_tests EXCLUDE_FROM_ALL Tests/VDX7HostResetTests.cpp)
target_link_libraries(vdx7_host_reset_tests PRIVATE VDX7)
target_include_directories(vdx7_host_reset_tests PRIVATE
    $<TARGET_PROPERTY:VDX7,INCLUDE_DIRECTORIES>)
target_compile_definitions(vdx7_host_reset_tests PRIVATE
    $<TARGET_PROPERTY:VDX7,COMPILE_DEFINITIONS>)
add_dependencies(vdx7_ci_checks vdx7_host_reset_tests)
# Compile the isolated counterfactual experiment on public CI; execute only
# locally with the verified private firmware. Never link it into VDX7.
add_executable(vdx7_mono_candidate_tests EXCLUDE_FROM_ALL Tests/VDX7MonoCandidateTests.cpp)
target_link_libraries(vdx7_mono_candidate_tests PRIVATE vdx7_core vdx7_voice_data)
add_dependencies(vdx7_ci_checks vdx7_mono_candidate_tests)
add_executable(vdx7_mono_correction_tests Tests/VDX7MonoCorrectionTests.cpp)
target_include_directories(vdx7_mono_correction_tests PRIVATE Source)
add_test(NAME vdx7_mono_correction COMMAND vdx7_mono_correction_tests)
set_tests_properties(vdx7_mono_correction PROPERTIES TIMEOUT 20 LABELS "rom-free;correction-policy-not-product-fix")
add_dependencies(vdx7_ci_checks vdx7_mono_correction_tests)
if(VDX7_ENABLE_ROM_TESTS)
    # These regressions inspect the locally validated v1.8 ownership map.
    # A wrong image is a failed precondition, never silent SKIP or success.
    add_test(NAME vdx7_v18_profile
        COMMAND vdx7_host_reset_tests "${VDX7_TEST_ROM_FILE}" --profile-check-only)
    set_tests_properties(vdx7_v18_profile PROPERTIES
        FIXTURES_SETUP vdx7_v18_rom TIMEOUT 20 LABELS "local-rom;firmware-v1_8")
    add_test(NAME vdx7_host_reset
        COMMAND vdx7_host_reset_tests "${VDX7_TEST_ROM_FILE}")
    set_tests_properties(vdx7_host_reset PROPERTIES
        TIMEOUT 180 LABELS "local-rom")
    # Keep lifecycle and history-cost diagnostics independently observable.
    add_test(NAME vdx7_reactivation
        COMMAND vdx7_host_reset_tests "${VDX7_TEST_ROM_FILE}" --reactivation-only)
    set_tests_properties(vdx7_reactivation PROPERTIES TIMEOUT 180 LABELS "local-rom")
    add_test(NAME vdx7_reset_history_pair
        COMMAND vdx7_host_reset_tests "${VDX7_TEST_ROM_FILE}" --history-pair-only)
    set_tests_properties(vdx7_reset_history_pair PROPERTIES TIMEOUT 180 LABELS "local-rom")
    add_test(NAME vdx7_firmware_ownership
        COMMAND vdx7_host_reset_tests "${VDX7_TEST_ROM_FILE}" --ownership-only)
    set_tests_properties(vdx7_firmware_ownership PROPERTIES TIMEOUT 180 LABELS "local-rom")
    add_test(NAME vdx7_history_retirement
        COMMAND vdx7_host_reset_tests "${VDX7_TEST_ROM_FILE}" --retirement-only)
    set_tests_properties(vdx7_history_retirement PROPERTIES TIMEOUT 180 LABELS "local-rom")
    add_test(NAME vdx7_overlap_retirement
        COMMAND vdx7_host_reset_tests "${VDX7_TEST_ROM_FILE}" --overlap-only)
    set_tests_properties(vdx7_overlap_retirement PROPERTIES TIMEOUT 180 LABELS "local-rom")
    add_test(NAME vdx7_reset_gate_overflow
        COMMAND vdx7_host_reset_tests "${VDX7_TEST_ROM_FILE}" --gate-overflow-only)
    set_tests_properties(vdx7_reset_gate_overflow PROPERTIES TIMEOUT 180 LABELS "local-rom")
    add_test(NAME vdx7_expanded_lifecycle
        COMMAND vdx7_host_reset_tests "${VDX7_TEST_ROM_FILE}" --expanded-lifecycle-only)
    set_tests_properties(vdx7_expanded_lifecycle PROPERTIES TIMEOUT 180 LABELS "local-rom")
    add_test(NAME vdx7_mono_boundary_characterization
        COMMAND vdx7_host_reset_tests "${VDX7_TEST_ROM_FILE}" --mono-boundary-only)
    set_tests_properties(vdx7_mono_boundary_characterization PROPERTIES TIMEOUT 180)
    add_test(NAME vdx7_mono_trace_characterization
        COMMAND vdx7_host_reset_tests "${VDX7_TEST_ROM_FILE}" --mono-trace-only)
    set_tests_properties(vdx7_mono_trace_characterization PROPERTIES TIMEOUT 180)
    add_test(NAME vdx7_deferred_partition
        COMMAND vdx7_host_reset_tests "${VDX7_TEST_ROM_FILE}" --deferred-partition-only)
    set_tests_properties(vdx7_deferred_partition PROPERTIES TIMEOUT 180)
    add_test(NAME vdx7_portamento
        COMMAND vdx7_stress_tests "${VDX7_TEST_ROM_FILE}" --portamento-only)
    set_tests_properties(vdx7_portamento PROPERTIES TIMEOUT 120)
    set_tests_properties(vdx7_host_reset vdx7_reactivation vdx7_reset_history_pair
        vdx7_firmware_ownership vdx7_history_retirement vdx7_overlap_retirement
        vdx7_reset_gate_overflow vdx7_expanded_lifecycle vdx7_mono_boundary_characterization
        vdx7_mono_trace_characterization vdx7_deferred_partition vdx7_portamento
        PROPERTIES FIXTURES_REQUIRED vdx7_v18_rom LABELS "local-rom;firmware-v1_8")
    set_property(TEST vdx7_mono_boundary_characterization vdx7_mono_trace_characterization APPEND PROPERTY
        LABELS "known-firmware-characterization")
    add_test(NAME vdx7_mono_candidate_experiment
        COMMAND vdx7_mono_candidate_tests "${VDX7_TEST_ROM_FILE}")
    set_tests_properties(vdx7_mono_candidate_experiment PROPERTIES
        TIMEOUT 180 FIXTURES_REQUIRED vdx7_v18_rom LABELS "local-rom;firmware-v1_8;experiment-not-product-fix")
    # This intentionally exposes the unresolved production P1 in the combined
    # local result. Do NOT use WILL_FAIL, SKIP or a characterization expectation.
    add_test(NAME vdx7_mono_note_zero_acceptance
        COMMAND vdx7_host_reset_tests "${VDX7_TEST_ROM_FILE}" --mono-note-zero-only)
    set_tests_properties(vdx7_mono_note_zero_acceptance PROPERTIES
        TIMEOUT 60 FIXTURES_REQUIRED vdx7_v18_rom LABELS "local-rom;firmware-v1_8;release-blocker")
endif()
