function(vdx7_resolve_display_version output_variable project_version release_build candidate_label)
    if(NOT candidate_label STREQUAL "")
        if(release_build)
            message(FATAL_ERROR
                "VDX7_RELEASE_CANDIDATE and VDX7_RELEASE_BUILD are mutually exclusive")
        endif()

        if(NOT candidate_label MATCHES "^rc[1-9][0-9]*$")
            message(FATAL_ERROR
                "VDX7_RELEASE_CANDIDATE must be empty or use rcN notation (for example rc1)")
        endif()

        set(display_version "${project_version}-${candidate_label}")
    elseif(release_build)
        set(display_version "${project_version}")
    else()
        set(display_version "${project_version}-dev")
    endif()

    set("${output_variable}" "${display_version}" PARENT_SCOPE)
endfunction()
