//==============================================================================
//
//  Restart.cpp
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
#include "Restart.h"
#include "Debug.h"
#include "ElementException.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
RestartStatus Restart::Status_ = Launching;
RestartLevel Restart::Level_ = RestartReboot;

//------------------------------------------------------------------------------

bool Restart::ClearsMemory(MemoryType type)
{
   switch(type)
   {
   case MemProtected:
   case MemPersistent:
      return (Level_ >= RestartReload);
   case MemDynamic:
      return (Level_ >= RestartCold);
   case MemTemporary:
      return (Level_ >= RestartWarm);
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Restart_Initiate = "Restart.Initiate";

void Restart::Initiate(reinit_t reason, debug32_t errval)
{
   Debug::ft(Restart_Initiate);

   throw ElementException(reason, errval);
}
}