//==============================================================================
//
//  ThisThread.cpp
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
#include "ThisThread.h"
#include "NbTracer.h"
#include "Thread.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
bool ThisThread::EnterBlockingOperation(BlockingReason why, fn_name_arg func)
{
   return Thread::EnterBlockingOperation(why, func);
}

//------------------------------------------------------------------------------

void ThisThread::ExitBlockingOperation(fn_name_arg func)
{
   return Thread::ExitBlockingOperation(func);
}

//------------------------------------------------------------------------------

void ThisThread::IncludeInTrace()
{
   NbTracer::SelectThread(RunningThreadId(), TraceIncluded);
}

//------------------------------------------------------------------------------

void ThisThread::MakePreemptable()
{
   Thread::MakePreemptable();
}

//------------------------------------------------------------------------------

void ThisThread::MakeUnpreemptable()
{
   Thread::MakeUnpreemptable();
}

//------------------------------------------------------------------------------

void ThisThread::MemProtect()
{
   Thread::MemProtect();
}

//------------------------------------------------------------------------------

void ThisThread::MemUnprotect()
{
   Thread::MemUnprotect();
}

//------------------------------------------------------------------------------

DelayRc ThisThread::Pause(msecs_t msecs)
{
   return Thread::Pause(msecs);
}

//------------------------------------------------------------------------------

void ThisThread::PauseOver(word limit)
{
   Thread::PauseOver(limit);
}

//------------------------------------------------------------------------------

word ThisThread::RtcPercentUsed()
{
   return Thread::RtcPercentUsed();
}

//------------------------------------------------------------------------------

ThreadId ThisThread::RunningThreadId()
{
   return Thread::RunningThread()->Tid();
}

//------------------------------------------------------------------------------

TraceRc ThisThread::StartTracing(const string& options)
{
   return Thread::StartTracing(options);
}

//------------------------------------------------------------------------------

void ThisThread::StopTracing()
{
   Thread::StopTracing();
}
}
