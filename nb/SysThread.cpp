//==============================================================================
//
//  SysThread.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include <ostream>
#include <string>
#include "Debug.h"
#include "Duration.h"
#include "Formatters.h"
#include "NbSignals.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
SysThread::SysThread
   (Thread* client, ThreadEntry entry, Priority prio, size_t size) :
   nthread_(nullptr),
   nid_(NIL_ID),
   event_(CreateSentry()),
   guard_(CreateSentry()),
   priority_(Priority_N),
   signal_(SIGNIL)
{
   Debug::ft("SysThread.ctor");

   Debug::Assert(event_ != nullptr);
   Debug::Assert(guard_ != nullptr);

   //  Create the thread and set its priority.
   //
   nthread_ = Create(entry, client, size, nid_);
   Debug::Assert(nthread_ != nullptr);

   Debug::Assert(SetPriority(prio));
}

//------------------------------------------------------------------------------

SysThread::SysThread() :
   nthread_(nullptr),
   nid_(RunningThreadId()),
   event_(CreateSentry()),
   guard_(CreateSentry()),
   priority_(Priority_N),
   signal_(SIGNIL)
{
   Debug::ft("SysThread.ctor(wrap)");

   Debug::Assert(event_ != nullptr);
   Debug::Assert(guard_ != nullptr);

   //  Wrap the thread and set its priority.
   //
   nthread_ = Wrap();
   Debug::Assert(nthread_ != nullptr);

   Debug::Assert(SetPriority(WatchdogPriority));
}

//------------------------------------------------------------------------------

SysThread::~SysThread()
{
   Debug::ftnt("SysThread.dtor");

   DeleteSentry(event_);
   DeleteSentry(guard_);
   Delete(nthread_);
}

//------------------------------------------------------------------------------

DelayRc SysThread::Delay(const Duration& timeout)
{
   Debug::ft("SysThread.Delay");

   return Suspend(event_, timeout);
}

//------------------------------------------------------------------------------

void SysThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Permanent::Display(stream, prefix, options);

   stream << prefix << "nthread  : " << nthread_ << CRLF;
   stream << prefix << "nid      : " << strHex(nid_, 4, false) << CRLF;
   stream << prefix << "status   : " << status_.to_string() << CRLF;
   stream << prefix << "event    : " << event_ << CRLF;
   stream << prefix << "guard    : " << guard_ << CRLF;
   stream << prefix << "priority : " << priority_ << CRLF;
   stream << prefix << "signal   : " << signal_ << CRLF;
}

//------------------------------------------------------------------------------

bool SysThread::Interrupt()
{
   Debug::ft("SysThread.Interrupt");

   return Resume(event_);
}

//------------------------------------------------------------------------------

bool SysThread::Proceed()
{
   Debug::ft("SysThread.Proceed");

   return Resume(guard_);
}

//------------------------------------------------------------------------------

DelayRc SysThread::Wait()
{
   Debug::ft("SysThread.Wait");

   return Suspend(guard_, TIMEOUT_NEVER);
}
}
