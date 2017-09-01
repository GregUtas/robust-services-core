//==============================================================================
//
//  ToolTypes.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "ToolTypes.h"
#include "NbCliParms.h"
#include "SysTypes.h"

using std::ostream;

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string ToolRcStrings[TraceRc_N + 1] =
{
   "OK.",
   "Tracing is already on.",
   "There is insufficient memory to allocate a buffer of that size.",
   "There is no space for that selection. Please CLEAR a selection.",
   "There is no such item.",
   "No trace buffer exists. Please SET BUFFSIZE first.",
   "Nothing is selected. Please INCLUDE something first.",
   "No trace tool is selected. Please SET a trace tool ON first.",
   "This command may not be used while tracing is in progress.",
   "The trace buffer contains entries. Please CLEAR BUFFER first.",
   "The trace buffer has nothing to display.",
   "Error: The file could not be opened.",
   "No relevant trace records found. Required tool(s) may not be on.",
   NotInFieldExpl,
   "The operation failed.",
   ERROR_STR
};

const char* strTraceRc(TraceRc rc)
{
   if((rc >= 0) && (rc < TraceRc_N)) return ToolRcStrings[rc];
   return ToolRcStrings[TraceRc_N];
}

//------------------------------------------------------------------------------

fixed_string StatusStrings[TraceStatus_N + 1] =
{
   "unspecified",
   "excluded",
   "included",
   ERROR_STR
};

ostream& operator<<(std::ostream& stream, TraceStatus status)
{
   if((status >= 0) && (status < TraceStatus_N))
      stream << StatusStrings[status];
   else
      stream << StatusStrings[TraceStatus_N];
   return stream;
}
}