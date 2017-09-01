//==============================================================================
//
//  ThisThread.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "ThisThread.h"
#include "Thread.h"

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

TraceRc ThisThread::StartTracing(bool immediate, bool autostop)
{
   return Thread::StartTracing(immediate, autostop);
}

//------------------------------------------------------------------------------

void ThisThread::StopTracing()
{
   Thread::StopTracing();
}
}