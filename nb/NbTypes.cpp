//==============================================================================
//
//  NbTypes.cpp
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
#include "NbTypes.h"
#include <sstream>

using std::ostream;

//------------------------------------------------------------------------------

namespace NodeBase
{
const Flags Vb_Mask = Flags(1 << DispVerbose);

//------------------------------------------------------------------------------

fixed_string BlockingReasonStrings[BlockingReason_N + 1] =
{
   "rdy",
   "slp",
   "net",
   "con",
   "d/b",
   "???"
};

char NodeBase::BlockingReasonChar(BlockingReason reason)
{
   std::ostringstream stream;
   stream << reason;
   return stream.str().front();
}

ostream& operator<<(std::ostream& stream, BlockingReason reason)
{
   if((reason >= 0) && (reason < BlockingReason_N))
      stream << BlockingReasonStrings[reason];
   else
      stream << BlockingReasonStrings[BlockingReason_N];
   return stream;
}

//------------------------------------------------------------------------------

fixed_string FactionStrings[Faction_N + 1] =
{
   "Idle",
   "Audit",
   "Background",
   "Operations",
   "Maintenance",
   "Payload",
   "System",
   "Watchdog",
   ERROR_STR
};

char NodeBase::FactionChar(Faction faction)
{
   std::ostringstream stream;
   stream << faction;
   return stream.str().front();
}

ostream& operator<<(std::ostream& stream, Faction faction)
{
   if((faction >= 0) && (faction < Faction_N))
      stream << FactionStrings[faction];
   else
      stream << FactionStrings[Faction_N];
   return stream;
}
}