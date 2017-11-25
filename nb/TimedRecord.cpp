//==============================================================================
//
//  TimedRecord.cpp
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
