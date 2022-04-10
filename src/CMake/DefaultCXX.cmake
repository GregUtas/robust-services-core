include("${CMAKE_CURRENT_LIST_DIR}/Default.cmake")

set_config_specific_property("OUTPUT_DIRECTORY" "${CMAKE_SOURCE_DIR}$<$<NOT:$<STREQUAL:${CMAKE_VS_PLATFORM_NAME},Win32>>:/${CMAKE_VS_PLATFORM_NAME}>/${PROPS_CONFIG}")

if(MSVC)
    set_target_properties("${PROPS_TARGET}" PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()
