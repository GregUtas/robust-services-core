//==============================================================================
//
//  Restart.cpp
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
#include "Restart.h"
#include <ostream>
#include "Debug.h"
#include "ElementException.h"

using std::ostream;

//------------------------------------------------------------------------------

namespace NodeBase
{
RestartStage Restart::Stage_ = Launching;
RestartLevel Restart::Level_ = RestartReboot;

//------------------------------------------------------------------------------

bool Restart::ClearsMemory(MemoryType type)
{
   switch(type)
   {
   case MemImmutable:
   case MemPermanent:
      return (Level_ >= RestartReboot);
   case MemProtected:
   case MemPersistent:
      return (Level_ >= RestartReload);
   case MemSlab:
   case MemDynamic:
      return (Level_ >= RestartCold);
   case MemTemporary:
      return (Level_ >= RestartWarm);
   }

   return false;
}

//------------------------------------------------------------------------------

RestartLevel Restart::GetLevel()
{
   Debug::ft("Restart.GetLevel");

   return (Stage_ == Running ? RestartNone: Level_);
}

//------------------------------------------------------------------------------

void Restart::Initiate
   (RestartLevel level, RestartReason reason, debug64_t errval)
{
   Debug::ft("Restart.Initiate");

   throw ElementException(level, reason, errval);
}

//------------------------------------------------------------------------------

RestartLevel Restart::LevelToClear(MemoryType type)
{
   switch(type)
   {
   case MemTemporary:
      return RestartWarm;
   case MemDynamic:
   case MemSlab:
      return RestartCold;
   case MemPersistent:
   case MemProtected:
      return RestartReload;
   case MemPermanent:
   case MemImmutable:
      return RestartReboot;
   }

   return RestartNone;
}

//------------------------------------------------------------------------------

fixed_string RestartReasonStrings[RestartReason_N + 1] =
{
   "manual restart",
   "mutex creation failed",
   "heap creation failed",
   "object pool creation failed",
   "death of eritical thread",
   "heap protection failed",
   "heap corruption",
   "work queue corruption",
   "timer queue corruption",
   ERROR_STR
};

ostream& operator<<(ostream& stream, RestartReason reason)
{
   if((reason >= 0) && (reason < RestartReason_N))
      stream << RestartReasonStrings[reason];
   else
      stream << RestartReasonStrings[RestartReason_N];
   return stream;
}
}
