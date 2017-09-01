//==============================================================================
//
//  ThisThread.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef THISTHREAD_H_INCLUDED
#define THISTHREAD_H_INCLUDED

#include "Clock.h"
#include "NbTypes.h"
#include "SysTypes.h"
#include "ToolTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
   //  Wrapper for Thread static functions.  It avoids the need for some
   //  application software to #include Thread.h.  The functions defined
   //  here simply invoke the eponymous functions on Thread.
   //
   namespace ThisThread
   {
      ThreadId RunningThreadId();
      void MakeUnpreemptable();
      void MakePreemptable();
      word RtcPercentUsed();
      DelayRc Pause(msecs_t msecs = TIMEOUT_IMMED);
      void PauseOver(word limit);
      bool EnterBlockingOperation(BlockingReason why, fn_name_arg func);
      void ExitBlockingOperation(fn_name_arg func);
      void MemUnprotect();
      void MemProtect();
      TraceRc StartTracing(bool immediate, bool autostop);
      void StopTracing();
   }
}
#endif
