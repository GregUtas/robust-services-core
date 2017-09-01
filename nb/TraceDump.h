//==============================================================================
//
//  TraceDump.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef TRACEDUMP_H_INCLUDED
#define TRACEDUMP_H_INCLUDED

#include <iosfwd>
#include <string>
#include "SysTypes.h"
#include "ToolTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Interface for displaying the contents of a trace buffer.
//
namespace TraceDump
{
   //  Trace output field widths (see TraceHeader2).
   //  Each field adds two trailing blanks for separation (TabWidth).
   //
   constexpr col_t TabWidth  =  2;
   constexpr col_t TodWidth  =  9;  // time of day field
   constexpr col_t TidWidth  =  3;  // thread identifier field
   constexpr col_t EvtWidth  =  5;  // event description field
   constexpr col_t TotWidth  =  9;  // total time field
   constexpr col_t NetWidth  =  8;  // net time field
   constexpr col_t IdRcWidth = 10;  // id or return code field

   constexpr col_t StartToEvt = TodWidth + TidWidth + (2 * TabWidth);
   constexpr col_t EvtToObj   = TotWidth + TabWidth;
   constexpr col_t ObjToDesc  = IdRcWidth + TabWidth;

   //  Returns a string containing TabWidth spaces.
   //
   const std::string& Tab();

   //  Displays the records in the trace buffer.
   //
   TraceRc Generate(std::ostream& stream);
}
}
#endif
