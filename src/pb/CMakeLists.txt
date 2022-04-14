set(PROJECT_NAME pb)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "PbModule.h"
    "PotsBicFeature.h"
    "PotsBocFeature.h"
    "PotsCfbFeature.h"
    "PotsCfnFeature.h"
    "PotsCfuFeature.h"
    "PotsCircuit.h"
    "PotsCliParms.h"
    "PotsCwtFeature.h"
    "PotsCxfFeature.h"
    "PotsFeature.h"
    "PotsFeatureProfile.h"
    "PotsFeatureRegistry.h"
    "PotsFeatures.h"
    "PotsHtlFeature.h"
    "PotsIncrement.h"
    "PotsLogs.h"
    "PotsProfile.h"
    "PotsProfileRegistry.h"
    "PotsProtocol.h"
    "PotsSusFeature.h"
    "PotsTwcFeature.h"
    "PotsWmlFeature.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "PbModule.cpp"
    "PotsBicFeature.cpp"
    "PotsBocFeature.cpp"
    "PotsCfbFeature.cpp"
    "PotsCfnFeature.cpp"
    "PotsCfuFeature.cpp"
    "PotsCircuit.cpp"
    "PotsCliParms.cpp"
    "PotsCwtFeature.cpp"
    "PotsCxfFeature.cpp"
    "PotsFeature.cpp"
    "PotsFeatureProfile.cpp"
    "PotsFeatureRegistry.cpp"
    "PotsFeatures.cpp"
    "PotsHtlFeature.cpp"
    "PotsIncrement.cpp"
    "PotsLogs.cpp"
    "PotsMessages.cpp"
    "PotsProfile.cpp"
    "PotsProfileRegistry.cpp"
    "PotsProtocol.cpp"
    "PotsSusFeature.cpp"
    "PotsTwcFeature.cpp"
    "PotsWmlFeature.cpp"
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
################################################################################
# Includes for CMake from *.props
################################################################################

set(ROOT_NAMESPACE pb)

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
    "${CMAKE_CURRENT_SOURCE_DIR}/../cb"
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
# Link with other targets.
target_link_libraries(${PROJECT_NAME} PUBLIC
    cb
    mb
    nb
    nt
    nw
    sb
    st
)
