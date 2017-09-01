//==============================================================================
//
//  SysSignals.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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

#ifndef SIGALRM
const signal_t SIGALRM = 14;
#endif

SysSignals::SigAlrm::SigAlrm() : PosixSignal(SIGALRM, "SIGALRM",
   "Alarm Clock", 0, PS_Native()) { }

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

#ifndef SIGQUIT
const signal_t SIGQUIT = 3;
#endif

SysSignals::SigQuit::SigQuit() : PosixSignal(SIGQUIT, "SIGQUIT",
   "Terminal Quit", 8, PS_Native() | PS_Break()) { }

//------------------------------------------------------------------------------

SysSignals::SigSegv::SigSegv() : PosixSignal(SIGSEGV, "SIGSEGV",
   "Invalid Memory Reference", 0, PS_Native()) { }

//------------------------------------------------------------------------------

#ifndef SIGSYS
const signal_t SIGSYS = 12;
#endif

SysSignals::SigSys::SigSys() : PosixSignal(SIGSYS, "SIGSYS",
   "Bad System Call", 0, PS_Native()) { }

//------------------------------------------------------------------------------

SysSignals::SigTerm::SigTerm() : PosixSignal(SIGTERM, "SIGTERM",
   "Termination Request", 0, PS_Native()) { }

//------------------------------------------------------------------------------

#ifndef SIGVTALRM
const signal_t SIGVTALRM = 28;
#endif

SysSignals::SigVtAlrm::SigVtAlrm() : PosixSignal(SIGVTALRM, "SIGVTALRM",
   "Virtual Timer Expired", 0, PS_Native()) { }
}
