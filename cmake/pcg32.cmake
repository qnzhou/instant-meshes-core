if(TARGET pcg32::pcg32)
    return()
endif()

message(STATUS "Third-party (external): using bundled pcg32")

set(pcg32_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../ext/pcg32")
if(NOT EXISTS "${pcg32_SOURCE_DIR}/pcg32.h")
    message(FATAL_ERROR "Bundled pcg32 not found at ${pcg32_SOURCE_DIR}")
endif()

add_library(pcg32 INTERFACE)
target_include_directories(pcg32 SYSTEM INTERFACE ${pcg32_SOURCE_DIR})
add_library(pcg32::pcg32 ALIAS pcg32)

install(TARGETS pcg32 EXPORT InstantMeshesCoreTargets)
install(FILES
    "${pcg32_SOURCE_DIR}/pcg32.h"
    "${pcg32_SOURCE_DIR}/pcg32_8.h"
    DESTINATION include)
