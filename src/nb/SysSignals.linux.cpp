//==============================================================================
//
//  SysSignals.linux.cpp
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
#ifdef OS_LINUX

#include "SysSignals.h"
#include "PosixSignal.h"
#include <bitset>
#include <csignal>
#include "Debug.h"
#include "Singleton.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
class SigBus : public PosixSignal
{
   friend class Singleton< SigBus >;

   SigBus();
   ~SigBus() = default;
};

SigBus::SigBus() : PosixSignal(SIGBUS, "SIGBUS",
   "Non-existent Memory Access", 0,
      PS_Native() | PS_Break() | PS_Interrupt()) { }

//------------------------------------------------------------------------------

void SysSignals::CreateNativeSignals()
{
   Debug::ft("SysSignals.CreateNativeSignals");

   Singleton< SigBus >::Instance();
}
}
#endif
