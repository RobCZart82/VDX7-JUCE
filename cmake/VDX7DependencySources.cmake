# Observe resolved sources after FetchContent/third_party selection. This file
# also serves as CMAKE_PROJECT_VDX7_JUCE_INCLUDE for older product checkouts in
# the exact approval/workflow checkout for older product/frozen packager pairs.
# Never execute the output file.
get_property(_vdx7_sources_scheduled GLOBAL PROPERTY VDX7_DEPENDENCY_SOURCES_SCHEDULED)
if(NOT _vdx7_sources_scheduled)
    set_property(GLOBAL PROPERTY VDX7_DEPENDENCY_SOURCES_SCHEDULED TRUE)
    function(vdx7_record_dependency_sources)
        if(NOT juce_SOURCE_DIR OR NOT VDX7_CORE_DIR)
            message(FATAL_ERROR "Missing resolved VDX7 dependency source directories")
        endif()
        file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/VDX7DependencySources.cmake" CONTENT
"set(VDX7_SOURCE_DIR \"${CMAKE_CURRENT_SOURCE_DIR}\")
set(VDX7_JUCE_SOURCE_DIR \"${juce_SOURCE_DIR}\")
set(VDX7_CORE_SOURCE_DIR \"${VDX7_CORE_DIR}\")
")
    endfunction()
    cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL vdx7_record_dependency_sources)
endif()
