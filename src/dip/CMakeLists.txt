set(PROJECT_NAME dip)

################################################################################
# Source groups
################################################################################
set(no_group_source_files
    "Adjudicator.cpp"
    "BaseBot.cpp"
    "BaseBot.h"
    "BotThread.cpp"
    "BotThread.h"
    "BotTrace.cpp"
    "BotTrace.h"
    "BotTracer.cpp"
    "BotTracer.h"
    "BotType.h"
    "ConvoySubversion.cpp"
    "ConvoySubversion.h"
    "DipModule.cpp"
    "DipModule.h"
    "DipProtocol.cpp"
    "DipProtocol.h"
    "DipTypes.h"
    "Location.cpp"
    "Location.h"
    "MapAndUnits.cpp"
    "MapAndUnits.h"
    "Province.cpp"
    "Province.h"
    "StartupParameters.cpp"
    "StartupParameters.h"
    "Token.cpp"
    "Token.h"
    "TokenMessage.cpp"
    "TokenMessage.h"
    "TokenTextMap.cpp"
    "TokenTextMap.h"
    "UnitOrder.cpp"
    "UnitOrder.h"
    "WinterOrders.cpp"
    "WinterOrders.h"
)
source_group("" FILES ${no_group_source_files})

set(ALL_FILES
    ${no_group_source_files}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")

set(ROOT_NAMESPACE dip)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)

if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
)
endif()

################################################################################
# Include directories
################################################################################
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../nb;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../nw"
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
    nb
    nw
)
