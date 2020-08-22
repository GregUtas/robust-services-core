//==============================================================================
//
//  TraceDump.h
//
//  Copyright (C) 2013-2020  Greg Utas
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
   constexpr col_t TabWidth = 2;
   constexpr col_t TodWidth = 9;    // time of day field
   constexpr col_t TidWidth = 3;    // thread identifier field
   constexpr col_t EvtWidth = 5;    // event description field
   constexpr col_t TotWidth = 9;    // total time field
   constexpr col_t NetWidth = 8;    // net time field
   constexpr col_t IdRcWidth = 10;  // id or return code field

   constexpr col_t StartToEvt = TodWidth + TidWidth + (2 * TabWidth);
   constexpr col_t EvtToObj = TotWidth + TabWidth;
   constexpr col_t ObjToDesc = IdRcWidth + TabWidth;

   //  Returns a string containing TabWidth spaces.
   //
   const std::string& Tab();

   //  Displays the records in the trace buffer.
   //
   TraceRc Generate(std::ostream& stream, const std::string& opts);
}
}
#endif
