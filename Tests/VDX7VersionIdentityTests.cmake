if(NOT DEFINED VDX7_VERSION_MODULE)
    message(FATAL_ERROR "VDX7_VERSION_MODULE must point to cmake/VDX7Version.cmake")
endif()

include("${VDX7_VERSION_MODULE}")

if(DEFINED VDX7_TEST_INVALID_CASE)
    if(VDX7_TEST_INVALID_CASE STREQUAL "release_and_candidate")
        vdx7_resolve_display_version(actual "1.0.0" ON "rc1")
    elseif(VDX7_TEST_INVALID_CASE STREQUAL "invalid_label")
        vdx7_resolve_display_version(actual "1.0.0" OFF "candidate")
    else()
        message(FATAL_ERROR "Unknown negative-test case: ${VDX7_TEST_INVALID_CASE}")
    endif()
    message(FATAL_ERROR "Invalid version identity was accepted")
endif()

function(assert_version expected release_build candidate_label)
    vdx7_resolve_display_version(actual "1.0.0" "${release_build}" "${candidate_label}")
    if(NOT actual STREQUAL expected)
        message(FATAL_ERROR "Expected ${expected}, got ${actual}")
    endif()
endfunction()

assert_version("1.0.0-dev" OFF "")
assert_version("1.0.0-rc1" OFF "rc1")
assert_version("1.0.0-rc12" OFF "rc12")
assert_version("1.0.0" ON "")

foreach(base_version IN ITEMS 1.0.0 1.0.1)
    foreach(mode IN ITEMS dev rc1 stable)
        if(mode STREQUAL "stable")
            vdx7_resolve_display_version(actual "${base_version}" ON "")
            set(expected "${base_version}")
        elseif(mode STREQUAL "rc1")
            vdx7_resolve_display_version(actual "${base_version}" OFF "rc1")
            set(expected "${base_version}-rc1")
        else()
            vdx7_resolve_display_version(actual "${base_version}" OFF "")
            set(expected "${base_version}-dev")
        endif()
        if(NOT actual STREQUAL expected)
            message(FATAL_ERROR "Expected ${expected}, got ${actual}")
        endif()
    endforeach()
endforeach()

foreach(invalid_case IN ITEMS release_and_candidate invalid_label)
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            "-DVDX7_VERSION_MODULE=${VDX7_VERSION_MODULE}"
            "-DVDX7_TEST_INVALID_CASE=${invalid_case}"
            -P "${CMAKE_CURRENT_LIST_FILE}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error_output)
    if(result EQUAL 0)
        message(FATAL_ERROR "Expected ${invalid_case} to be rejected")
    endif()
    if(NOT "${output}${error_output}" MATCHES "VDX7_RELEASE_CANDIDATE")
        message(FATAL_ERROR "${invalid_case} failed for an unexpected reason: ${error_output}")
    endif()
endforeach()

message(STATUS "VDX7 version identity cases passed")
