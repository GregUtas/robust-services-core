//==============================================================================
//
//  NbSignals.cpp
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
#include "NbSignals.h"
#include "PosixSignal.h"
#include "Debug.h"
#include "Singleton.h"
#include "SysSignals.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Signals defined for use within NodeBase.  They cannot be used with the
//  functions signal() and raise(), but their SIG... constants can be used
//  with SignalException.
//
class SigWrite : public PosixSignal
{
   friend class Singleton< SigWrite >;

   SigWrite();
   ~SigWrite() = default;
};

class SigClose : public PosixSignal
{
   friend class Singleton< SigClose >;

   SigClose();
   ~SigClose() = default;
};

class SigYield : public PosixSignal
{
   friend class Singleton< SigYield >;

   SigYield();
   ~SigYield() = default;
};

class SigStack1 : public PosixSignal
{
   friend class Singleton< SigStack1 >;

   SigStack1();
   ~SigStack1() = default;
};

class SigStack2 : public PosixSignal
{
   friend class Singleton< SigStack2 >;

   SigStack2();
   ~SigStack2() = default;
};

class SigPurge : public PosixSignal
{
   friend class Singleton< SigPurge >;

   SigPurge();
   ~SigPurge() = default;
};

class SigDeleted : public PosixSignal
{
   friend class Singleton< SigDeleted >;

   SigDeleted();
   ~SigDeleted() = default;
};

//------------------------------------------------------------------------------

SigWrite::SigWrite() : PosixSignal(SIGWRITE, "SIGWRITE",
   "Write to Protected Memory", 0, NoFlags) { }

SigClose::SigClose() : PosixSignal(SIGCLOSE, "SIGCLOSE",
   "Non-Error Shutdown", 12,
      PS_Interrupt() | PS_Final() | PS_NoLog() | PS_NoError()) { }

SigYield::SigYield() : PosixSignal(SIGYIELD, "SIGYIELD",
   "Running Unpreemptably Too Long", 4, NoFlags) { }

SigStack1::SigStack1() : PosixSignal(SIGSTACK1, "SIGSTACK1",
   "Stack Overflow: Attempt Recovery", 0, NoFlags) { }

SigStack2::SigStack2() : PosixSignal(SIGSTACK2, "SIGSTACK2",
   "Stack Overflow: Exit Thread", 0, PS_Final()) { }

SigPurge::SigPurge() : PosixSignal(SIGPURGE, "SIGPURGE",
   "Suicided [errval = 0] or Killed [errval > 0]", 16,
      PS_Interrupt() | PS_Final()) { }

SigDeleted::SigDeleted() : PosixSignal(SIGDELETED, "SIGDELETED",
   "Thread Deleted", 0, PS_Final()) { }

//------------------------------------------------------------------------------

void CreatePosixSignals()
{
   Debug::ft("NodeBase.CreatePosixSignals");

   //  Create <cstdint> signals, this platform's native signals, and then
   //  our proprietary signals.
   //
   SysSignals::CreateStandardSignals();
   SysSignals::CreateNativeSignals();

   Singleton< SigWrite >::Instance();
   Singleton< SigClose >::Instance();
   Singleton< SigYield >::Instance();
   Singleton< SigStack1 >::Instance();
   Singleton< SigStack2 >::Instance();
   Singleton< SigPurge >::Instance();
   Singleton< SigDeleted >::Instance();
}
}
