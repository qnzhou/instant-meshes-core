if(TARGET pss::pss)
    return()
endif()

message(STATUS "Third-party (external): creating target 'pss::pss'")

include(CPM)
CPMAddPackage(
    NAME pss
    GITHUB_REPOSITORY wjakob/pss
    GIT_TAG a91da33ea2e22f90d1babfb99c4882c485467af4
    DOWNLOAD_ONLY
)

add_library(pss INTERFACE)
target_include_directories(pss SYSTEM INTERFACE ${pss_SOURCE_DIR})
add_library(pss::pss ALIAS pss)
