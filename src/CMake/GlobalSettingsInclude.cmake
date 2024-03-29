# Options common to all RSC builds
#
# - if(MSVC) is true for both MSVC and clang builds
# - clang is currently used only to target Windows (x64)
# - GCC is used only to target Linux (x64)
# - x86 uses "Visual Studio 17 2022" generator instead of Ninja
#
set(CMAKE_CXX_STANDARD 17)

if(MSVC)
    message("** Reading global settings shared by MSVC and CLang")

    # Enable Windows targets (*.win.cpp files)
    add_compile_definitions(OS_WIN)

    # Use async exception handling to catch structured exceptions
    add_compile_options(/EHa)

    # Disable optimizations to improve debugging
    add_compile_options(/Od)

    # Keep frame pointers
    add_compile_options(/Oy-)

    # Include security checks
    add_compile_options(/GS)

    # Enable all compiler warnings
    add_compile_options(/Wall)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message("** Reading global settings for MSVC")

    # For definitions in subs/cstddef
    add_compile_definitions(MSVC_COMPILER)

    # Include run-time checks
    add_compile_options(/RTC1)

    # Provide full debugging information
    add_compile_options(/Z7)

    # Disable Just My Code debugging
    add_compile_options(/JMC-)

    # Use standard C++
    add_compile_options(/permissive-)

    # Treat compiler security warnings as errors
    add_compile_options(/sdl)

    # Compiler warnings to treat as errors
    add_compile_options(/we4715)

    # Compiler warnings to suppress
    add_compile_options(/wd4061 /wd4062 /wd4100 /wd4242 /wd4244 /wd4267)
    add_compile_options(/wd4365 /wd4514 /wd4582 /wd4623 /wd4625 /wd4626)
    add_compile_options(/wd4668 /wd4710 /wd4711 /wd4820 /wd5026 /wd5027)
    add_compile_options(/wd5045 /wd5219 /wd26812)

    # Include debugging information
    add_link_options(/DEBUG:FULL)

    # Do not merge identical functions or data
    add_link_options(/OPT:NOICF)

    # Support incremental linking for patching
    add_link_options(/INCREMENTAL)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message("** Reading global settings for Clang")

    # Provide full debugging information
    add_compile_options(/Zi)

    # Disable specific compiler warnings
    add_compile_options(-Wno-c++98-compat)
    add_compile_options(-Wno-c++98-compat-pedantic)
    add_compile_options(-Wno-cast-align)
    add_compile_options(-Wno-cast-qual)
    add_compile_options(-Wno-covered-switch-default)
    add_compile_options(-Wno-exit-time-destructors)
    add_compile_options(-Wno-global-constructors)
    add_compile_options(-Wno-header-hygiene)
    add_compile_options(-Wno-implicit-int-conversion)
    add_compile_options(-Wno-implicit-int-float-conversion)
    add_compile_options(-Wno-inconsistent-missing-destructor-override)
    add_compile_options(-Wno-missing-noreturn)
    add_compile_options(-Wno-old-style-cast)
    add_compile_options(-Wno-shadow)
    add_compile_options(-Wno-shadow-field-in-constructor)
    add_compile_options(-Wno-shorten-64-to-32)
    add_compile_options(-Wno-sign-compare)
    add_compile_options(-Wno-sign-conversion)
    add_compile_options(-Wno-suggest-destructor-override)
    add_compile_options(-Wno-switch)
    add_compile_options(-Wno-switch-enum)
    add_compile_options(-Wno-tautological-type-limit-compare)
    add_compile_options(-Wno-tautological-undefined-compare)
    add_compile_options(-Wno-unused-parameter)
    add_compile_options(-Wno-unreachable-code-break)
    add_compile_options(-Wno-unreachable-code-return)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message("** Reading global settings for GCC")

    # Enable Linux targets (*.linux.cpp files)
    add_compile_definitions(OS_LINUX)

    # Increase reliability of stack traces
    add_compile_options(-fasynchronous-unwind-tables)

    # Enable exceptions
    add_compile_options(-fexceptions)

    # Allow signal handler to throw a C++ exception
    add_compile_options(-fnon-call-exceptions)

    # Support function names in stack traces
    add_link_options(-fno-pie)

    # Allow typedefs that hide underlying types in subs/chrono to compile
    add_compile_options(-fpermissive)

    # Generate debugging information
    add_compile_options(-g)

    # Disable optimizations
    add_compile_options(-O0)

    # Enable POSIX threads
    add_compile_options(-pthread)

    # Enable function names in stack traces
    add_compile_options(-rdynamic)

    # Enable all compiler warnings
    add_compile_options(-Wall)

    # Disable specific compiler warnings
    add_compile_options(-Wno-address)
    add_compile_options(-Wno-nonnull-compare)
    add_compile_options(-Wno-switch)

    # Support function names in stack traces
    add_link_options(-ldl)
    add_link_options(-no-pie)

    # Enable POSIX threads
    add_link_options(-pthread)

    # Enable function names in stack traces
    add_link_options(-rdynamic)
else()
    message(FATAL_ERROR "** ${CMAKE_CXX_COMPILER_ID} compiler is not supported")
endif()
