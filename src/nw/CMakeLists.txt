set(PROJECT_NAME nw)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "ByteBuffer.h"
    "InputHandler.h"
    "IoThread.h"
    "IpBuffer.h"
    "IpPort.h"
    "IpPortRegistry.h"
    "IpService.h"
    "IpServiceCfg.h"
    "IpServiceRegistry.h"
    "LocalAddrTest.h"
    "NwCliParms.h"
    "NwDaemons.h"
    "NwIncrement.h"
    "NwLogs.h"
    "NwModule.h"
    "NwPools.h"
    "NwTrace.h"
    "NwTracer.h"
    "NwTypes.h"
    "SysIpL2Addr.h"
    "SysIpL3Addr.h"
    "SysSocket.h"
    "SysTcpSocket.h"
    "SysUdpSocket.h"
    "TcpIoThread.h"
    "TcpIpPort.h"
    "TcpIpService.h"
    "UdpIoThread.h"
    "UdpIpPort.h"
    "UdpIpService.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "ByteBuffer.cpp"
    "InputHandler.cpp"
    "IoThread.cpp"
    "IpBuffer.cpp"
    "IpPort.cpp"
    "IpPortRegistry.cpp"
    "IpService.cpp"
    "IpServiceCfg.cpp"
    "IpServiceRegistry.cpp"
    "LocalAddrTest.cpp"
    "NwCliParms.cpp"
    "NwDaemons.cpp"
    "NwIncrement.cpp"
    "NwLogs.cpp"
    "NwModule.cpp"
    "NwPools.cpp"
    "NwTrace.cpp"
    "NwTracer.cpp"
    "NwTypes.cpp"
    "SysIpL2Addr.cpp"
    "SysIpL2Addr.linux.cpp"
    "SysIpL2Addr.win.cpp"
    "SysIpL3Addr.cpp"
    "SysIpL3Addr.linux.cpp"
    "SysIpL3Addr.win.cpp"
    "SysSocket.cpp"
    "SysSocket.linux.cpp"
    "SysSocket.win.cpp"
    "SysTcpSocket.cpp"
    "SysTcpSocket.linux.cpp"
    "SysTcpSocket.win.cpp"
    "SysUdpSocket.cpp"
    "SysUdpSocket.linux.cpp"
    "SysUdpSocket.win.cpp"
    "TcpIoThread.cpp"
    "TcpIpPort.cpp"
    "TcpIpService.cpp"
    "UdpIoThread.cpp"
    "UdpIpPort.cpp"
    "UdpIpService.cpp"
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

set(ROOT_NAMESPACE nw)

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
    "${CMAKE_CURRENT_SOURCE_DIR}/../nb"
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
    nb
)

if(MSVC)
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "ws2_32"
    )
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC "${ADDITIONAL_LIBRARY_DEPENDENCIES}")
