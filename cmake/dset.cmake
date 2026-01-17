if(TARGET dset::dset)
    return()
endif()

message(STATUS "Third-party (external): using bundled dset")

set(dset_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../ext/dset")
if(NOT EXISTS "${dset_SOURCE_DIR}/dset.h")
    message(FATAL_ERROR "Bundled dset not found at ${dset_SOURCE_DIR}")
endif()

add_library(dset INTERFACE)
target_include_directories(dset SYSTEM INTERFACE ${dset_SOURCE_DIR})
add_library(dset::dset ALIAS dset)

install(TARGETS dset EXPORT InstantMeshesCoreTargets)
install(FILES "${dset_SOURCE_DIR}/dset.h" DESTINATION include)
