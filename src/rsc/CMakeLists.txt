set(PROJECT_NAME rsc)

################################################################################
# Source groups
################################################################################
set(Source_Files
    "main.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Source_Files}
)

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")

set(ROOT_NAMESPACE rsc)

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
    "${CMAKE_CURRENT_SOURCE_DIR}/../nt;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ct;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../nw;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../sb;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../st;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../mb;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../cb;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../pb;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../on;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../cn;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../rn;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../an;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../sn;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../dip"
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
        "_CONSOLE;"
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
        "_CONSOLE;"
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
        "_CONSOLE;"
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
        /DEBUG;
        /SUBSYSTEM:CONSOLE
    )
endif()

################################################################################
# Startup project
################################################################################
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT rsc)

################################################################################
# Command line parameters
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "Prot_kBs=8192")

################################################################################
# Dependencies
################################################################################
# Link with other targets.
target_link_libraries(${PROJECT_NAME} PRIVATE
    an
    cb
    cn
    ct
    dip
    mb
    nb
    nt
    nw
    on
    pb
    rn
    sb
    sn
    st
)
