//==============================================================================
//
//  SysSignals.win.cpp
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
#ifdef OS_WIN
#include <bitset>
#include "SysSignals.h"
#include "PosixSignal.h"
#include <csignal>
#include "Debug.h"
#include "Singleton.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
class SigBreak : public PosixSignal
{
   friend class Singleton< SigBreak >;
private:
   SigBreak();
};

SigBreak::SigBreak() : PosixSignal(SIGBREAK, "SIGBREAK",
   "Ctrl-Break", 8, PS_Native() | PS_Break()) { }

//------------------------------------------------------------------------------

fn_name SysSignals_CreateNativeSignals = "SysSignals.CreateNativeSignals";

void SysSignals::CreateNativeSignals()
{
   Debug::ft(SysSignals_CreateNativeSignals);

   Singleton< SigBreak >::Instance();
}
}
#endif
