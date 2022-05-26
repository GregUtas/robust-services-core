//==============================================================================
//
//  SysSignals.cpp
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
#include "SysSignals.h"
#include "PosixSignal.h"
#include <csignal>
#include "Debug.h"
#include "Singleton.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
class SigAbort : public PosixSignal
{
   friend class Singleton<SigAbort>;

   SigAbort();
   ~SigAbort() = default;
};

class SigFpe : public PosixSignal
{
   friend class Singleton<SigFpe>;

   SigFpe();
   ~SigFpe() = default;
};

class SigIll : public PosixSignal
{
   friend class Singleton<SigIll>;

   SigIll();
   ~SigIll() = default;
};

class SigInt : public PosixSignal
{
   friend class Singleton<SigInt>;

   SigInt();
   ~SigInt() = default;
};

class SigSegv : public PosixSignal
{
   friend class Singleton<SigSegv>;

   SigSegv();
   ~SigSegv() = default;
};

class SigTerm : public PosixSignal
{
   friend class Singleton<SigTerm>;

   SigTerm();
   ~SigTerm() = default;
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
   "Terminal Interrupt", 8, PS_Native() | PS_Break() | PS_Interrupt()) { }

//------------------------------------------------------------------------------

SigSegv::SigSegv() : PosixSignal(SIGSEGV, "SIGSEGV",
   "Illegal Memory Access", 0, PS_Native()) { }

//------------------------------------------------------------------------------

SigTerm::SigTerm() : PosixSignal(SIGTERM, "SIGTERM",
   "Termination Request", 0, PS_Native()) { }

//------------------------------------------------------------------------------

void SysSignals::CreateStandardSignals()
{
   Debug::ft("SysSignals.CreateStandardSignals");

   Singleton<SigAbort>::Instance();
   Singleton<SigFpe>::Instance();
   Singleton<SigIll>::Instance();
   Singleton<SigInt>::Instance();
   Singleton<SigSegv>::Instance();
   Singleton<SigTerm>::Instance();
}
}
