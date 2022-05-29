//==============================================================================
//
//  SysThread.win.cpp
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
#ifdef OS_WIN

#include "SysThread.h"
#include <csignal>
#include <cstdint>
#include <process.h>
#include <stdlib.h>
#include <Windows.h>
#include "Debug.h"
#include "NbSignals.h"
#include "Thread.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Mapping of external to internal priorities.
//
const int PriorityMap[SysThread::Priority_N] =
{
   THREAD_PRIORITY_BELOW_NORMAL,  // LowPriority
   THREAD_PRIORITY_NORMAL,        // DefaultPriority
   THREAD_PRIORITY_ABOVE_NORMAL,  // SystemPriority
   THREAD_PRIORITY_HIGHEST        // WatchdogPriority
};

//------------------------------------------------------------------------------

static signal_t AccessViolationType(const _EXCEPTION_POINTERS* ex)
{
   auto rec = ex->ExceptionRecord;

   if(rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
   {
      if(rec->NumberParameters > 0)
      {
         if(rec->ExceptionInformation[0] == 1) return SIGWRITE;
      }
   }

   return SIGSEGV;
}

//------------------------------------------------------------------------------
//
//  Converts a Windows structured exception to a C++ exception.
//
static void SE_Handler(uint32_t errval, const _EXCEPTION_POINTERS* ex)
{
   //  Reenable Debug functions before tracing this function.
   //
   Thread::ResetDebugFlags();
   Debug::ft("NodeBase.SE_Handler");

   signal_t sig = 0;

   switch(errval)                         // errval:
   {
   case DBG_CONTROL_C:                    // 0x40010005
      sig = SIGINT;
      break;

   case DBG_CONTROL_BREAK:                // 0x40010008
      sig = SIGBREAK;
      break;

   case STATUS_ACCESS_VIOLATION:          // 0xC0000005
      sig = AccessViolationType(ex);
      break;

   case STATUS_DATATYPE_MISALIGNMENT:     // 0x80000002
   case STATUS_IN_PAGE_ERROR:             // 0xC0000006
   case STATUS_INVALID_HANDLE:            // 0xC0000008
   case STATUS_NO_MEMORY:                 // 0xC0000017
      sig = SIGSEGV;
      break;

   case STATUS_ILLEGAL_INSTRUCTION:       // 0xC000001D
      sig = SIGILL;
      break;

   case STATUS_NONCONTINUABLE_EXCEPTION:  // 0xC0000025
      sig = SIGTERM;
      break;

   case STATUS_INVALID_DISPOSITION:       // 0xC0000026
   case STATUS_ARRAY_BOUNDS_EXCEEDED:     // 0xC000008C
      sig = SIGSEGV;
      break;

   case STATUS_FLOAT_DENORMAL_OPERAND:    // 0xC000008D
   case STATUS_FLOAT_DIVIDE_BY_ZERO:      // 0xC000008E
   case STATUS_FLOAT_INEXACT_RESULT:      // 0xC000008F
   case STATUS_FLOAT_INVALID_OPERATION:   // 0xC0000090
   case STATUS_FLOAT_OVERFLOW:            // 0xC0000091
   case STATUS_FLOAT_STACK_CHECK:         // 0xC0000092
   case STATUS_FLOAT_UNDERFLOW:           // 0xC0000093
   case STATUS_INTEGER_DIVIDE_BY_ZERO:    // 0xC0000094
   case STATUS_INTEGER_OVERFLOW:          // 0xC0000095
      sig = SIGFPE;
      break;

   case STATUS_PRIVILEGED_INSTRUCTION:    // 0xC0000096
      sig = SIGILL;
      break;

   case STATUS_STACK_OVERFLOW:            // 0xC00000FD
      //
      //  A stack overflow in Windows now raises the exception
      //  System.StackOverflowException, which cannot be caught.
      //  Stack checking in Thread should therefore be enabled.
      //
      sig = SIGSTACK1;
      break;

   default:
      sig = SIGTERM;
   }

   //  Handle SIG.  This usually throws an exception; in any case, it will
   //  not return here.  If it does return, there is no specific provision
   //  for reraising a structured exception, so simply return and assume
   //  that Windows will handle it, probably brutally.
   //
   Thread::HandleSignal(sig, errval);
}

//------------------------------------------------------------------------------

static unsigned int EnterThread(void* arg)
{
   Debug::ft("NodeBase.EnterThread");

   //  Our argument is a pointer to a Thread.
   //
   auto thread = static_cast<Thread*>(arg);
   return thread->Start();
}

//------------------------------------------------------------------------------

void SysThread::ConfigureProcess()
{
   Debug::ft("SysThread.ConfigureProcess");

   //  Disable special abort handling and set our overall process priority.
   //
   _set_abort_behavior(0, _CALL_REPORTFAULT | _WRITE_ABORT_MSG);

   auto process = GetCurrentProcess();
   SetPriorityClass(process, HIGH_PRIORITY_CLASS);
}

//------------------------------------------------------------------------------

fn_name SysThread_Create = "SysThread.Create";

bool SysThread::Create(const Thread* client, size_t size)
{
   Debug::ft(SysThread_Create);

   //  Create a native thread and provide its identifiers.  Disable Windows
   //  priority boosts, which could interfere with our priority scheme.
   //
   unsigned int id;
   nthread_ = _beginthreadex
      (nullptr, size, EnterThread, (void*) client, 0, &id);

   if(nthread_ == 0)
   {
      return ReportError(SysThread_Create, "_beginthreadex", GetLastError());
   }

   nid_ = id;
   SetThreadPriorityBoost((HANDLE) nthread_, true);
   return true;
}

//------------------------------------------------------------------------------

void SysThread::Delete()
{
   Debug::ftnt("SysThread.Delete");

   if(nthread_ != 0)
   {
      CloseHandle((HANDLE) nthread_);
      nthread_ = 0;
   }
}

//------------------------------------------------------------------------------

void SysThread::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void SysThread::RegisterForSignal(signal_t sig, sighandler_t handler)
{
   signal(sig, handler);
}

//------------------------------------------------------------------------------

SysThreadId SysThread::RunningThreadId() NO_FT
{
   return GetCurrentThreadId();
}

//------------------------------------------------------------------------------

bool SysThread::SetPriority(Priority prio)
{
   Debug::ft("SysThread.SetPriority");

   if(priority_ == prio) return true;

   if(!SetThreadPriority((HANDLE) nthread_, PriorityMap[prio]))
   {
      return false;
   }

   priority_ = prio;
   return true;
}

//------------------------------------------------------------------------------

fn_name SysThread_Start = "SysThread.Start";

signal_t SysThread::Start()
{
   Debug::ft(SysThread_Start);

   //  This is also invoked when recovering from a trap, so see if a stack
   //  overflow occurred.  Some of these are irrecoverable, in which case
   //  returning SIGSTACK2 causes the thread to exit.
   //
   if(status_.test(StackOverflowed))
   {
      if(_resetstkoflw() == 0)
      {
         Debug::SwLog(SysThread_Start, status_.to_string(), nid_);
         return SIGSTACK2;
      }

      status_.reset(StackOverflowed);
   }

   //  The translator for Windows structured exceptions must be installed
   //  on a per-thread basis.
   //
   _set_se_translator((_se_translator_function) SE_Handler);
   return 0;
}
}
#endif
