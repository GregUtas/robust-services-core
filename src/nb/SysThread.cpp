//==============================================================================
//
//  SysThread.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "SysThread.h"
#include <condition_variable>
#include <csignal>
#include <exception>
#include <ostream>
#include <string>
#include <thread>
#include "Debug.h"
#include "Formatters.h"
#include "NbSignals.h"
#include "SignalException.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  The following is registered to intercept calls to std::terminate.
//
static void HandleTerminate()
{
   Debug::ft("NodeBase.HandleTerminate");

   throw SignalException(SIGTERM, 0);
}

//------------------------------------------------------------------------------
//
//  Invoked to wait on GATE until TIMEOUT.  If TIMEOUT is TIMEOUT_NEVER,
//  the thread will only resume after GATE is signalled.
//
static DelayRc Suspend(Gate& gate, const msecs_t& timeout)
{
   Debug::ft("NodeBase.Suspend");

   auto result = gate.WaitFor(timeout);

   return (result == std::cv_status::timeout ?
      DelayCompleted : DelayInterrupted);
}

//------------------------------------------------------------------------------

SysThread::SysThread(Thread* client, Priority prio, size_t size) :
   nid_(NIL_ID),
   nthread_(0),
   priority_(Priority_N),
   signal_(SIGNIL)
{
   Debug::ft("SysThread.ctor");

   //  This must be done on a per-thread basis in Windows, so it might as
   //  well be done that way on all platforms.
   //
   std::set_terminate(HandleTerminate);

   Debug::Assert(Create(client, size));
   Debug::Assert(nid_ != NIL_ID);
   Debug::Assert(nthread_ != 0);
   Debug::Assert(SetPriority(prio));
}

//------------------------------------------------------------------------------

SysThread::~SysThread()
{
   Debug::ftnt("SysThread.dtor");

   Delete();
}

//------------------------------------------------------------------------------

DelayRc SysThread::Delay(const msecs_t& timeout)
{
   Debug::ft("SysThread.Delay");

   return Suspend(clock_, timeout);
}

//------------------------------------------------------------------------------

void SysThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Permanent::Display(stream, prefix, options);

   stream << prefix << "nid      : " << strHex(nid_) << CRLF;
   stream << prefix << "nthread  : " << nthread_ << CRLF;
   stream << prefix << "status   : " << status_.to_string() << CRLF;
   stream << prefix << "priority : " << priority_ << CRLF;
   stream << prefix << "signal   : " << signal_ << CRLF;
}

//------------------------------------------------------------------------------

bool SysThread::Interrupt()
{
   Debug::ft("SysThread.Interrupt");

   clock_.Notify();
   return true;
}

//------------------------------------------------------------------------------

void SysThread::Pause(const msecs_t& msecs)
{
   Debug::ft("SysThread.Pause");

   std::this_thread::sleep_for(msecs);
}

//------------------------------------------------------------------------------

bool SysThread::Proceed()
{
   Debug::ft("SysThread.Proceed");

   sched_.Notify();
   return true;
}

//------------------------------------------------------------------------------

bool SysThread::ReportError(fn_name function, fixed_string expl, int error)
{
   Debug::SwLog(function, expl, error);
   return false;
}

//------------------------------------------------------------------------------

DelayRc SysThread::Wait()
{
   Debug::ft("SysThread.Wait");

   return Suspend(sched_, TIMEOUT_NEVER);
}
}
