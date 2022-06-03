set(PROJECT_NAME subs)

################################################################################
# Source groups
################################################################################
set(Headers
    "algorithm"
    "atomic"
    "bitset"
    "cctype"
    "chrono"
    "cmath"
    "csignal"
    "cstddef"
    "cstdint"
    "cstdio"
    "cstdlib"
    "cstring"
    "ctime"
    "cxxabi.h"
    "dbghelp.h"
    "endian.h"
    "errno.h"
    "exception"
    "execinfo.h"
    "filesystem"
    "fstream"
    "functional"
    "in.h"
    "intsafe.h"
    "ioctl.h"
    "iomanip"
    "ios"
    "iosfwd"
    "iostream"
    "istream"
    "iterator"
    "list"
    "malloc.h"
    "map"
    "mcheck.h"
    "mman.h"
    "memory"
    "mutex"
    "netdb.h"
    "new"
    "ostream"
    "poll.h"
    "process.h"
    "pthread.h"
    "queue"
    "ratio"
    "resource.h"
    "sched.h"
    "set"
    "socket.h"
    "spawn.h"
    "sstream"
    "stack"
    "stdlib.h"
    "string"
    "system_error"
    "typeinfo"
    "unordered_map"
    "unistd.h"
    "utility"
    "vector"
    "wait.h"
    "windows.h"
    "winerror.h"
    "winsock2.h"
    "ws2tcpip.h"
)
source_group("Headers" FILES ${Headers})

set(Sources
    "subs.cpp"
)
source_group("Sources" FILES ${Sources})

set(ALL_FILES
    ${Headers}
    ${Sources}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE subs)

if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_GLOBAL_KEYWORD "Win32Proj"
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    )
endif()

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
        "_CONSOLE;"
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
        /SUBSYSTEM:WINDOWS
    )
endif()
