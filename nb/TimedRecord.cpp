//==============================================================================
//
//  TimedRecord.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "TimedRecord.h"
#include <iomanip>
#include <ostream>
#include "Singleton.h"
#include "SysThread.h"
#include "Thread.h"
#include "ThreadRegistry.h"
#include "ToolTypes.h"
#include "TraceDump.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
TimedRecord::TimedRecord(size_t size, FlagId owner) : TraceRecord(size, owner),
   nid_(SysThread::RunningThreadId()),
   ticks_(Clock::TicksNow())
{
}

//------------------------------------------------------------------------------

bool TimedRecord::Display(ostream& stream)
{
   auto reg = Singleton< ThreadRegistry >::Instance();
   auto tid = reg->FindThreadId(nid_);
   auto thr = reg->FindThread(nid_);

   if((thr != nullptr) && (thr->CalcStatus(false) != TraceIncluded))
      return false;

   stream << GetTime() << TraceDump::Tab();
   stream << setw(TraceDump::TidWidth) << tid << TraceDump::Tab();
   stream << EventString() << TraceDump::Tab();
   return true;
}

//------------------------------------------------------------------------------

string TimedRecord::GetTime() const
{
   //  Convert our tick timestamp to hh:mm:ss.mmm and remove the hours.
   //
   return Clock::TicksToTime(ticks_, MinsField);
}

//------------------------------------------------------------------------------

ThreadId TimedRecord::Tid() const
{
   return Singleton< ThreadRegistry >::Instance()->FindThreadId(nid_);
}
}
