if(TARGET pcg32::pcg32)
    return()
endif()

message(STATUS "Third-party (external): creating target 'pcg32::pcg32'")

include(CPM)
CPMAddPackage(
    NAME pcg32
    GITHUB_REPOSITORY wjakob/pcg32
    GIT_TAG 70099eadb86d3999c38cf69d2c55f8adc1f7fe34
    DOWNLOAD_ONLY
)

add_library(pcg32 INTERFACE)
target_include_directories(pcg32 SYSTEM INTERFACE ${pcg32_SOURCE_DIR})
add_library(pcg32::pcg32 ALIAS pcg32)
