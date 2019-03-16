//==============================================================================
//
//  ToolTypes.cpp
//
//  Copyright (C) 2017  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later
//  version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "ToolTypes.h"
#include <ostream>
#include "NbCliParms.h"

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

ostream& operator<<(ostream& stream, TraceStatus status)
{
   if((status >= 0) && (status < TraceStatus_N))
      stream << StatusStrings[status];
   else
      stream << StatusStrings[TraceStatus_N];
   return stream;
}
}
