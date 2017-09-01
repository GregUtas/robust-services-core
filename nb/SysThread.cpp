//==============================================================================
//
//  SysThread.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "SysThread.h"
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
fn_name SysThread_ctor1 = "SysThread.ctor";

SysThread::SysThread(const Thread* client,
   const ThreadEntry entry, Priority prio, size_t size) :
   nthread_(nullptr),
   nid_(NIL_ID),
   sentry_(nullptr),
   signal_(SIGNIL)
{
   Debug::ft(SysThread_ctor1);

   //  Create the thread and its sentry.  Set the thread's priority.
   //
   nthread_ = Create(entry, client, size, nid_);
   Debug::Assert(nthread_ != nullptr);

   sentry_ = CreateSentry();
   Debug::Assert(sentry_ != nullptr);

   Debug::Assert(SetPriority(prio));
}

//------------------------------------------------------------------------------

fn_name SysThread_ctor2 = "SysThread.ctor(wrap)";

SysThread::SysThread() :
   nthread_(nullptr),
   nid_(RunningThreadId()),
   sentry_(nullptr),
   signal_(SIGNIL)
{
   Debug::ft(SysThread_ctor2);

   //  Wrap the thread and create its sentry.  Set the thread's priority.
   //
   nthread_ = Wrap();
   Debug::Assert(nthread_ != nullptr);

   //  Create the event on which the thread waits and set the thread's faction.
   //
   sentry_ = CreateSentry();
   Debug::Assert(sentry_ != nullptr);

   SetPriority(WatchdogPriority);
}

//------------------------------------------------------------------------------

fn_name SysThread_dtor = "SysThread.dtor";

SysThread::~SysThread()
{
   Debug::ft(SysThread_dtor);

   DeleteSentry(sentry_);
   Delete(nthread_);
}

//------------------------------------------------------------------------------

void SysThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Permanent::Display(stream, prefix, options);

   stream << prefix << "nthread : " << nthread_ << CRLF;
   stream << prefix << "nid     : " << strHex(nid_, 4, false) << CRLF;
   stream << prefix << "status  : " << status_.to_string() << CRLF;
   stream << prefix << "sentry  : " << sentry_ << CRLF;
   stream << prefix << "signal  : " << signal_ << CRLF;
}
}
