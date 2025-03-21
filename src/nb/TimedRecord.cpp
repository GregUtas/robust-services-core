//==============================================================================
//
//  TimedRecord.cpp
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
#include "TimedRecord.h"
#include <iomanip>
#include <ostream>
#include "NbTypes.h"
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
//  Used instead of TraceDump::Tab to highlight a context switch.
//
static const string& ContextSwitchTab()
{
   static string ContextSwitchStr;

   if(ContextSwitchStr.empty())
   {
      ContextSwitchStr = TraceDump::Tab();
      ContextSwitchStr.front() = '>';
   }

   return ContextSwitchStr;
}

//------------------------------------------------------------------------------
//
//  The previous thread for which a record was displayed.
//
static ThreadId PrevTid_ = NIL_ID;

//------------------------------------------------------------------------------

TimedRecord::TimedRecord(FlagId owner) :
   TraceRecord(owner),
   nid_(SysThread::RunningThreadId()),
   time_(SystemTime::Now())
{
}

//------------------------------------------------------------------------------

bool TimedRecord::Display(ostream& stream, const string& opts)
{
   if(!SystemTime::IsValid(time_)) return false;

   auto reg = Singleton<ThreadRegistry>::Instance();
   auto thr = reg->FindThread(nid_);
   if((thr != nullptr) && (thr->CalcStatus(false) != TraceIncluded))
      return false;

   auto tid = (thr != nullptr ? thr->Tid() : reg->FindTid(nid_));
   stream << GetTime(opts) << TraceDump::Tab();
   stream << setw(TraceDump::TidWidth) << tid;

   if(tid == PrevTid_)
   {
      stream << TraceDump::Tab();
   }
   else
   {
      stream << ContextSwitchTab();
      PrevTid_ = tid;
   }

   stream << EventString() << TraceDump::Tab();
   return true;
}

//------------------------------------------------------------------------------

string TimedRecord::GetTime(const string& opts) const
{
   //  Convert our timestamp to hh:mm:ss.mmm and remove the hours.
   //
   if(opts.find(NoTimeData) != string::npos) return "00:00.000";
   return to_string(time_, MinSecMsecs);
}
}
