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
#include "PosixSignal.h"
#include <bitset>
#include <csignal>
#include "Debug.h"
#include "Singleton.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
class SigAbort : public PosixSignal
{
   friend class Singleton< SigAbort >;
private:
   SigAbort();
};

class SigFpe : public PosixSignal
{
   friend class Singleton< SigFpe >;
private:
   SigFpe();
};

class SigIll : public PosixSignal
{
   friend class Singleton< SigIll >;
private:
   SigIll();
};

class SigInt : public PosixSignal
{
   friend class Singleton< SigInt >;
private:
   SigInt();
};

class SigSegv : public PosixSignal
{
   friend class Singleton< SigSegv >;
private:
   SigSegv();
};

class SigTerm : public PosixSignal
{
   friend class Singleton< SigTerm >;
private:
   SigTerm();
};

//------------------------------------------------------------------------------

SigAbort::SigAbort() : PosixSignal(SIGABRT, "SIGABRT",
"Abort Request", 0, PS_Native()) { }

//------------------------------------------------------------------------------

SigFpe::SigFpe() : PosixSignal(SIGFPE, "SIGFPE",
   "Erroneous Arithmetic Operation", 0, PS_Native()) { }

//------------------------------------------------------------------------------

SigIll::SigIll() : PosixSignal(SIGILL, "SIGILL",
   "Illegal Instruction", 0, PS_Native()) { }

//------------------------------------------------------------------------------

SigInt::SigInt() : PosixSignal(SIGINT, "SIGINT",
   "Terminal Interrupt", 8, PS_Native() | PS_Break()) { }

//------------------------------------------------------------------------------

SigSegv::SigSegv() : PosixSignal(SIGSEGV, "SIGSEGV",
   "Invalid Memory Reference", 0, PS_Native()) { }

//------------------------------------------------------------------------------

SigTerm::SigTerm() : PosixSignal(SIGTERM, "SIGTERM",
   "Termination Request", 0, PS_Native()) { }

//------------------------------------------------------------------------------

fn_name SysSignals_CreateStandardSignals = "SysSignals.CreateStandardSignals";

void SysSignals::CreateStandardSignals()
{
   Debug::ft(SysSignals_CreateStandardSignals);

   Singleton< SigAbort >::Instance();
   Singleton< SigFpe >::Instance();
   Singleton< SigIll >::Instance();
   Singleton< SigInt >::Instance();
   Singleton< SigSegv >::Instance();
   Singleton< SigTerm >::Instance();
}
}
