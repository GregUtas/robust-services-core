set(PROJECT_NAME ct)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "CodeCoverage.h"
    "CodeDir.h"
    "CodeDirSet.h"
    "CodeFile.h"
    "CodeFileSet.h"
    "CodeItemSet.h"
    "CodeSet.h"
    "CodeTypes.h"
    "CodeWarning.h"
    "CtIncrement.h"
    "CtModule.h"
    "Cxx.h"
    "CxxArea.h"
    "CxxCharLiteral.h"
    "CxxDirective.h"
    "CxxExecute.h"
    "CxxFwd.h"
    "CxxLocation.h"
    "CxxNamed.h"
    "CxxRoot.h"
    "CxxScope.h"
    "CxxScoped.h"
    "CxxStatement.h"
    "CxxString.h"
    "CxxStrLiteral.h"
    "CxxSymbols.h"
    "CxxToken.h"
    "CxxVector.h"
    "Editor.h"
    "Interpreter.h"
    "Lexer.h"
    "Library.h"
    "LibraryErrSet.h"
    "LibraryItem.h"
    "LibrarySet.h"
    "LibraryTypes.h"
    "LibraryVarSet.h"
    "Parser.h"
    "SetOperations.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "CodeCoverage.cpp"
    "CodeDir.cpp"
    "CodeDirSet.cpp"
    "CodeFile.cpp"
    "CodeFileSet.cpp"
    "CodeItemSet.cpp"
    "CodeSet.cpp"
    "CodeTypes.cpp"
    "CodeWarning.cpp"
    "CtIncrement.cpp"
    "CtModule.cpp"
    "Cxx.cpp"
    "CxxArea.cpp"
    "CxxDirective.cpp"
    "CxxExecute.cpp"
    "CxxLocation.cpp"
    "CxxNamed.cpp"
    "CxxRoot.cpp"
    "CxxScope.cpp"
    "CxxScoped.cpp"
    "CxxStatement.cpp"
    "CxxString.cpp"
    "CxxSymbols.cpp"
    "CxxToken.cpp"
    "Editor.cpp"
    "Interpreter.cpp"
    "Lexer.cpp"
    "Library.cpp"
    "LibraryErrSet.cpp"
    "LibraryItem.cpp"
    "LibrarySet.cpp"
    "LibraryTypes.cpp"
    "LibraryVarSet.cpp"
    "Parser.cpp"
    "SetOperations.cpp"
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

set(ROOT_NAMESPACE ct)

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
    "${CMAKE_CURRENT_SOURCE_DIR}/../nt"
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

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
     source_file_compile_options(Cxx.cpp -Wno-char-subscripts)
     source_file_compile_options(CxxArea.cpp -Wno-char-subscripts)
     source_file_compile_options(CxxString.cpp -Wno-char-subscripts)
     source_file_compile_options(Lexer.cpp -Wno-char-subscripts)
endif()

################################################################################
# Dependencies
################################################################################
add_dependencies(${PROJECT_NAME}
    nt
)

# Link with other targets.
target_link_libraries(${PROJECT_NAME} PUBLIC
    nb
)
