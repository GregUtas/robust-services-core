//==============================================================================
//
//  SysThread.cpp
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
#include "SysThread.h"
#include <condition_variable>
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "NbSignals.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
SysThread::SysThread(Thread* client, Priority prio, size_t size) :
   nthread_(nullptr),
   nid_(NIL_ID),
   priority_(Priority_N),
   signal_(SIGNIL)
{
   Debug::ft("SysThread.ctor");

   //  Create the thread and set its priority.
   //
   Debug::Assert(Create(client, size, nid_, nthread_));
   Debug::Assert(SetPriority(prio));
}

//------------------------------------------------------------------------------

SysThread::~SysThread()
{
   Debug::ftnt("SysThread.dtor");

   Delete(nthread_);
}

//------------------------------------------------------------------------------

DelayRc SysThread::Delay(const msecs_t& timeout)
{
   Debug::ft("SysThread.Delay");

   return Suspend(alarm_, timeout);
}

//------------------------------------------------------------------------------

void SysThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Permanent::Display(stream, prefix, options);

   stream << prefix << "nthread  : " << nthread_ << CRLF;
   stream << prefix << "nid      : " << strHex(nid_, 8, false) << CRLF;
   stream << prefix << "status   : " << status_.to_string() << CRLF;
   stream << prefix << "priority : " << priority_ << CRLF;
   stream << prefix << "signal   : " << signal_ << CRLF;
}

//------------------------------------------------------------------------------

bool SysThread::Interrupt()
{
   Debug::ft("SysThread.Interrupt");

   alarm_.Notify();
   return true;
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

fn_name SysThread_Suspend = "SysThread.Suspend";

DelayRc SysThread::Suspend(Gate& gate, const msecs_t& timeout)
{
   Debug::ft(SysThread_Suspend);

   //  This operation can only be applied to the running thread.
   //
   if(RunningThreadId() != nid_)
   {
      Debug::SwLog(SysThread_Suspend, "not running thread", nid_);
      return DelayError;
   }

   auto result = gate.WaitFor(timeout);

   return (result == std::cv_status::timeout ?
      DelayCompleted : DelayInterrupted);
}

//------------------------------------------------------------------------------

DelayRc SysThread::Wait()
{
   Debug::ft("SysThread.Wait");

   return Suspend(sched_, TIMEOUT_NEVER);
}
}
