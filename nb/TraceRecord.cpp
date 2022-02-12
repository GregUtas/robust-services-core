//==============================================================================
//
//  TraceRecord.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "TraceRecord.h"
#include <ostream>
#include "Formatters.h"
#include "TraceDump.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const uint32_t TraceRecord::InvalidSlot = UINT32_MAX;

//  identifies a record that has not been fully constructed.  Assigned
//  as the initial value for rid_.
//
constexpr TraceRecord::Id InvalidId = UINT8_MAX;

//------------------------------------------------------------------------------

TraceRecord::TraceRecord(FlagId owner) :
   slot_(InvalidSlot),
   owner_(owner),
   rid_(InvalidId)
{
}

//------------------------------------------------------------------------------

bool TraceRecord::Display(ostream& stream, const string& opts)
{
   stream << spaces(TraceDump::StartToEvt) << EventString() << TraceDump::Tab();
   return true;
}

//------------------------------------------------------------------------------

c_string TraceRecord::EventString() const
{
   static const string BlankEventStr(TraceDump::EvtWidth, SPACE);

   return BlankEventStr.c_str();
}
}
