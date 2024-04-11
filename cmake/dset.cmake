if(TARGET dset::dset)
    return()
endif()

message(STATUS "Third-party (external): creating target 'dset::dset'")

include(CPM)
CPMAddPackage(
    NAME dset
    GITHUB_REPOSITORY wjakob/dset
    GIT_TAG 7967ef0e6041cd9d73b9c7f614ab8ae92e9e587a
    DOWNLOAD_ONLY
)

add_library(dset INTERFACE)
target_include_directories(dset SYSTEM INTERFACE ${dset_SOURCE_DIR})
add_library(dset::dset ALIAS dset)
