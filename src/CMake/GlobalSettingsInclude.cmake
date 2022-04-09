# Options common to all RSC builds
#
if(MSVC)
    message("Reading GlobalSettingsInclude.cmake")

    # Enable Windows targets (*.win.cpp files)
    add_compile_definitions(OS_WIN)

    # Use async exception handling to catch structured exceptions
    add_compile_options(/EHa)

    # Include run-time checks
    add_compile_options(/RTC1)

    # Disable optimizations to improve debugging
    add_compile_options(/Od)

    # Provide full debugging information
    add_compile_options(/Z7)

    # Disable Just My Code debugging
    add_compile_options(/JMC-)

    # Keep frame pointers
    add_compile_options(/Oy-)

    # Use standard C++
    add_compile_options(/permissive-)

    # Include security checks
    add_compile_options(/GS)

    # Enable all compiler warnings
    add_compile_options(/Wall)

    # Treat compiler security warnings as errors
    add_compile_options(/sdl)

    # Compiler warnings to treat as errors
    add_compile_options(/we4715)

    # Compiler warnings to suppress
    add_compile_options(/wd4061 /wd4062 /wd4100 /wd4242 /wd4244 /wd4267)
    add_compile_options(/wd4365 /wd4514 /wd4582 /wd4623 /wd4625 /wd4626)
    add_compile_options(/wd4668 /wd4710 /wd4711 /wd4820 /wd5026 /wd5027)
    add_compile_options(/wd5045)

    # Include debugging information
    add_link_options(/DEBUG:FULL)

    # Do not merge identical functions or data
    add_link_options(/OPT:NOICF)

    # Support incremental linking for patching
    add_link_options(/INCREMENTAL)
endif()