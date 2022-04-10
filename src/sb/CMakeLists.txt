set(PROJECT_NAME sb)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "Context.h"
    "Event.h"
    "EventHandler.h"
    "Factory.h"
    "FactoryRegistry.h"
    "GlobalAddress.h"
    "Initiator.h"
    "InvokerPool.h"
    "InvokerPoolRegistry.h"
    "InvokerThread.h"
    "LocalAddress.h"
    "Message.h"
    "MsgContext.h"
    "MsgFactory.h"
    "MsgHeader.h"
    "MsgPort.h"
    "Parameter.h"
    "Protocol.h"
    "ProtocolLayer.h"
    "ProtocolRegistry.h"
    "ProtocolSM.h"
    "PsmContext.h"
    "PsmFactory.h"
    "RootServiceSM.h"
    "SbAppIds.h"
    "SbCliParms.h"
    "SbDaemons.h"
    "SbEvents.h"
    "SbExtInputHandler.h"
    "SbHandlers.h"
    "SbIncrement.h"
    "SbInputHandler.h"
    "SbInvokerPools.h"
    "SbIpBuffer.h"
    "SbLogs.h"
    "SbModule.h"
    "SbPools.h"
    "SbTrace.h"
    "SbTracer.h"
    "SbTypes.h"
    "Service.h"
    "ServiceRegistry.h"
    "ServiceSM.h"
    "Signal.h"
    "SsmContext.h"
    "SsmFactory.h"
    "State.h"
    "TextTlvMessage.h"
    "Timer.h"
    "TimerProtocol.h"
    "TimerRegistry.h"
    "TimerThread.h"
    "TlvIntParameter.h"
    "TlvMessage.h"
    "TlvParameter.h"
    "TlvProtocol.h"
    "Trigger.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "AnalyzeMsgEvent.cpp"
    "AnalyzeSapEvent.cpp"
    "AnalyzeSnpEvent.cpp"
    "Context.cpp"
    "Event.cpp"
    "EventHandler.cpp"
    "Factory.cpp"
    "FactoryRegistry.cpp"
    "ForceTransitionEvent.cpp"
    "GlobalAddress.cpp"
    "InitiationReqEvent.cpp"
    "Initiator.cpp"
    "InvokerPool.cpp"
    "InvokerPoolRegistry.cpp"
    "InvokerThread.cpp"
    "LocalAddress.cpp"
    "Message.cpp"
    "MsgContext.cpp"
    "MsgFactory.cpp"
    "MsgHeader.cpp"
    "MsgPort.cpp"
    "Parameter.cpp"
    "Protocol.cpp"
    "ProtocolLayer.cpp"
    "ProtocolRegistry.cpp"
    "ProtocolSM.cpp"
    "PsmContext.cpp"
    "PsmFactory.cpp"
    "RootServiceSM.cpp"
    "SbCliParms.cpp"
    "SbDaemons.cpp"
    "SbExtInputHandler.cpp"
    "SbHandlers.cpp"
    "SbIncrement.cpp"
    "SbInputHandler.cpp"
    "SbInvokerPools.cpp"
    "SbIpBuffer.cpp"
    "SbLogs.cpp"
    "SbModule.cpp"
    "SbPools.cpp"
    "SbTrace.cpp"
    "SbTracer.cpp"
    "SbTypes.cpp"
    "Service.cpp"
    "ServiceRegistry.cpp"
    "ServiceSM.cpp"
    "Signal.cpp"
    "SsmContext.cpp"
    "SsmFactory.cpp"
    "State.cpp"
    "TextTlvMessage.cpp"
    "Timer.cpp"
    "TimerProtocol.cpp"
    "TimerRegistry.cpp"
    "TimerThread.cpp"
    "TlvMessage.cpp"
    "TlvParameter.cpp"
    "TlvProtocol.cpp"
    "Trigger.cpp"
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

set(ROOT_NAMESPACE sb)

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
# Link with other targets.
target_link_libraries(${PROJECT_NAME} PUBLIC
    nb
    nw
)
