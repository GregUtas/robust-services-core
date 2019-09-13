//==============================================================================
//
//  SysSignals.cpp
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
#include "SysSignals.h"
#include <bitset>
#include <csignal>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
SysSignals::SigAbort::SigAbort() : PosixSignal(SIGABRT, "SIGABRT",
   "Abort Request", 0, PS_Native()) { }

//------------------------------------------------------------------------------

#ifndef SIGBREAK
const signal_t SIGBREAK = 21;
#endif

SysSignals::SigBreak::SigBreak() : PosixSignal(SIGBREAK, "SIGBREAK",
   "Ctrl-Break", 8, PS_Native() | PS_Break()) { }

//------------------------------------------------------------------------------

#ifndef SIGBUS
const signal_t SIGBUS = 10;
#endif

SysSignals::SigBus::SigBus() : PosixSignal(SIGBUS, "SIGBUS",
   "Invalid Memory Reference", 0, PS_Native()) { }

//------------------------------------------------------------------------------

SysSignals::SigFpe::SigFpe() : PosixSignal(SIGFPE, "SIGFPE",
   "Erroneous Arithmetic Operation", 0, PS_Native()) { }

//------------------------------------------------------------------------------

SysSignals::SigIll::SigIll() : PosixSignal(SIGILL, "SIGILL",
   "Illegal Instruction", 0, PS_Native()) { }

//------------------------------------------------------------------------------

SysSignals::SigInt::SigInt() : PosixSignal(SIGINT, "SIGINT",
   "Terminal Interrupt", 8, PS_Native() | PS_Break()) { }

//------------------------------------------------------------------------------

SysSignals::SigSegv::SigSegv() : PosixSignal(SIGSEGV, "SIGSEGV",
   "Invalid Memory Reference", 0, PS_Native()) { }

//------------------------------------------------------------------------------

SysSignals::SigTerm::SigTerm() : PosixSignal(SIGTERM, "SIGTERM",
   "Termination Request", 0, PS_Native()) { }
}
