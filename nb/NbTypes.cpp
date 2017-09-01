//==============================================================================
//
//  NbTypes.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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