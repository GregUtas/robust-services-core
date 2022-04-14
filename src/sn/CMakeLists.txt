set(PROJECT_NAME sn)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "PotsBcHandlers.h"
    "PotsBicService.h"
    "PotsBocService.h"
    "PotsCcwService.h"
    "PotsCfbService.h"
    "PotsCfnService.h"
    "PotsCfuService.h"
    "PotsCfxService.h"
    "PotsCwtService.h"
    "PotsHtlService.h"
    "PotsMultiplexer.h"
    "PotsProxyHandlers.h"
    "PotsProxySessions.h"
    "PotsSessions.h"
    "PotsStatistics.h"
    "PotsSusService.h"
    "PotsTreatmentRegistry.h"
    "PotsTreatments.h"
    "PotsWmlService.h"
    "SnIncrement.h"
    "SnModule.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "PotsBcHandlers.cpp"
    "PotsBcService.cpp"
    "PotsBcSsm.cpp"
    "PotsBcStates.cpp"
    "PotsBcTriggers.cpp"
    "PotsBicService.cpp"
    "PotsBocService.cpp"
    "PotsCallFactory.cpp"
    "PotsCallHandler.cpp"
    "PotsCallIpService.cpp"
    "PotsCallPsm.cpp"
    "PotsCcwService.cpp"
    "PotsCfbService.cpp"
    "PotsCfnService.cpp"
    "PotsCfuService.cpp"
    "PotsCfxService.cpp"
    "PotsCwmService.cpp"
    "PotsCwtService.cpp"
    "PotsDiscService.cpp"
    "PotsHtlService.cpp"
    "PotsMultiplexer.cpp"
    "PotsProxyHandlers.cpp"
    "PotsProxySessions.cpp"
    "PotsStatistics.cpp"
    "PotsSusService.cpp"
    "PotsTreatmentRegistry.cpp"
    "PotsTreatments.cpp"
    "PotsWmlService.cpp"
    "SnIncrement.cpp"
    "SnModule.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Header_Files}
    ${Source_Files}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")

set(ROOT_NAMESPACE sn)

if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_GLOBAL_KEYWORD "Win32Proj"
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    )
endif()

################################################################################
# Include directories
################################################################################
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../nb;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../nt;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../nw;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../sb;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../st;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../mb;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../cb;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../pb"
)

################################################################################
# Compile definitions
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "WIN32;"
        "_LIB;"
        "UNICODE;"
        "_UNICODE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    if(MSVC)
        target_compile_definitions(${PROJECT_NAME} PRIVATE
            "$<$<CONFIG:Debug>:"
                "_DEBUG"
            ">"
            "$<$<CONFIG:Release>:"
                "NDEBUG"
            ">"
            "_LIB;"
            "UNICODE;"
            "_UNICODE"
        )
    endif()
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "WIN32;"
        "_LIB;"
        "UNICODE;"
        "_UNICODE"
    )
endif()

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:
            /Oi;
            /Gy
        >
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:
            /OPT:REF;
            /OPT:NOICF
        >
        /SUBSYSTEM:WINDOWS
    )
endif()

################################################################################
# Dependencies
################################################################################
add_dependencies(${PROJECT_NAME}
    cb
    mb
    nb
    nw
    pb
    sb
)
