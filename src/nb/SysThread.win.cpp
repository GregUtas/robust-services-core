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
#include <Windows.h>
#include "Debug.h"
#include "Duration.h"
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

SysThread_t SysThread::Create
   (ThreadEntry entry, const Thread* client, size_t stackSize, SysThreadId& nid)
{
   Debug::ft("SysThread.Create");

   //  Create a native thread.
   //
   auto result = _beginthreadex(nullptr, stackSize,
      (_beginthreadex_proc_type) entry, (void*) client, 0, &nid);
   auto handle = (HANDLE) result;

   if(handle != nullptr)
   {
      //  Disable Windows priority boosts.
      //
      SetThreadPriorityBoost(handle, true);
   }

   return handle;
}

//------------------------------------------------------------------------------

SysSentry_t SysThread::CreateSentry()
{
   Debug::ft("SysThread.CreateSentry");

   //  On another platform, this is likely to be a combination of a
   //  condition variable and mutex, wrapped within an object that is
   //  private to this file.  The first false argument indicates that
   //  the event should be automatically reset when signalled, and the
   //  second indicates that it is not signalled in its inital state.
   //
   return CreateEvent(nullptr, false, false, nullptr);
}

//------------------------------------------------------------------------------

void SysThread::Delete(SysThread_t& thread)
{
   Debug::ftnt("SysThread.Delete");

   if(thread != nullptr)
   {
      CloseHandle(thread);
      thread = nullptr;
   }
}

//------------------------------------------------------------------------------

void SysThread::DeleteSentry(SysSentry_t& sentry)
{
   Debug::ftnt("SysThread.DeleteSentry");

   if(sentry != nullptr)
   {
      CloseHandle(sentry);
      sentry = nullptr;
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

   //  If the platform supports sigaction, it is preferred.  It should mask
   //  signals that do not point to an error in the signal handler itself.
   //  This is only a sketch.  For example, SIGSEGV should use sigaltstack
   //  to safely catch a stack overrun.
   //
   //  sigaction action;
   //  sigset_t  block_mask;
   //
   //  sigemptyset(&block_mask);
   //  sigaddset(&block_mask, SIGTERM);
   //  sigaddset(&block_mask, SIGINT);
   //
   //  action.sa_handler = handler;
   //  action.sa_mask = block_mask;
   //  action.sa_flags = 0;
   //
   //  sigaction(sig, &action, nullptr);
}

//------------------------------------------------------------------------------

fn_name SysThread_Resume = "SysThread.Resume";

bool SysThread::Resume(SysSentry_t& sentry)
{
   Debug::ft(SysThread_Resume);

   //  Signal SENTRY in case the thread is blocked on it.
   //
   if(SetEvent(sentry)) return true;
   Debug::SwLog(SysThread_Resume, "failed to set event", GetLastError());
   return false;
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

   if(!SetThreadPriority(nthread_, PriorityMap[prio]))
   {
      status_.set(SetPriorityFailed);
      return false;
   }

   priority_ = prio;
   status_.reset(SetPriorityFailed);
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

//------------------------------------------------------------------------------

fn_name SysThread_Suspend = "SysThread.Suspend";

DelayRc SysThread::Suspend(SysSentry_t& sentry, const Duration& timeout)
{
   Debug::ft(SysThread_Suspend);

   //  This operation can only be applied to the running thread.
   //
   if(RunningThreadId() != nid_)
   {
      Debug::SwLog(SysThread_Suspend, "thread not running", nid_);
      return DelayError;
   }

   auto rc = WaitForSingleObject(sentry, timeout.ToMsecs());

   switch(rc)
   {
   case WAIT_TIMEOUT:
      //
      //  Our timeout occurred before we were signalled.
      //
      return DelayCompleted;
   case WAIT_OBJECT_0:
      //
      //  Someone signalled us.
      //
      return DelayInterrupted;
   case WAIT_ABANDONED:
      //
      //  We're the only thread that waits on SENTRY, so this shouldn't occur.
      //
      Debug::SwLog(SysThread_Suspend, "unexpected result", rc);
      return DelayInterrupted;
   default:
      Debug::SwLog(SysThread_Suspend, "unknown result", GetLastError());
   }

   return DelayError;
}

//------------------------------------------------------------------------------

SysThread_t SysThread::Wrap()
{
   Debug::ft("SysThread.Wrap");

   //  Set our overall process priority and return a handle to our thread.
   //
   auto process = GetCurrentProcess();
   SetPriorityClass(process, HIGH_PRIORITY_CLASS);

   SysThread_t clone = GetCurrentThread();
   SysThread_t nthread;

   if(!DuplicateHandle(process, clone, process,
      &nthread, 0, false, DUPLICATE_SAME_ACCESS))
   {
      Debug::Assert(false);
   }

   return nthread;
}
}
#endif
