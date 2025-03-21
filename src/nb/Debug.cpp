//==============================================================================
//
//  Debug.cpp
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
#include "Debug.h"
#include <cstdint>
#include <ios>
#include <new>
#include <sstream>
#include "AssertionException.h"
#include "CoutThread.h"
#include "Duration.h"
#include "Element.h"
#include "Formatters.h"
#include "InitFlags.h"
#include "Log.h"
#include "NbAppIds.h"
#include "NbLogs.h"
#include "RootThread.h"
#include "Singleton.h"
#include "SysStackTrace.h"
#include "SysThread.h"
#include "ThisThread.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string UnexpectedInvocation = "unexpected invocation";

//  SwFlags_ controls the behavior of software during testing.
//
static Flags SwFlags_ = Flags();

//  Set to ExitCode_ to suppress logs when the system is exiting.  A simple
//  bool is not used in case this gets trampled.
//
static uint32_t ExitStatus_ = 0;

//  ExitCode_ is the magic value which indicates that the system is exiting.
//
static const uint32_t ExitCode_ = 0xDEADC0DE;

Flags Debug::FcFlags_ = Flags(InitFlags::TraceInit() ? 1 << TracingActive : 0);

//------------------------------------------------------------------------------

void Debug::Assert(bool condition, debug64_t errval)
{
   if(!condition)
   {
      throw AssertionException(errval);
   }
}

//------------------------------------------------------------------------------

void Debug::Exiting()
{
   Debug::ft("Debug.Exiting");

   //  Disable function tracing and logs while destructors are invoked
   //  during shutdown.
   //
   FcFlags_.reset();
   ExitStatus_ = ExitCode_;
}

//------------------------------------------------------------------------------
//
//  The cost of function tracing was assessed by running POTS traffic while
//  tracing threads in the Payload faction only.  The results, determined by
//  when the system entered overload, were
//  o tracing off: 17000 calls/minute
//  o tracing on: 4000 calls/minute
//
void Debug::ft(fn_name_arg func) NO_FT
{
   if(FcFlags_.none()) return;
   Thread::FunctionInvoked(func);
}

//------------------------------------------------------------------------------

void Debug::ftnt(fn_name_arg func) NO_FT
{
   if(FcFlags_.none()) return;
   Thread::FunctionInvoked(func, std::nothrow);
}

//------------------------------------------------------------------------------

Flags Debug::GetSwFlags()
{
   Debug::ft("Debug.GetSwFlags");

   if(Element::RunningInLab()) return SwFlags_;
   return NoFlags;
}

//------------------------------------------------------------------------------

void Debug::noop(debug64_t info)
{
   Debug::ft("Debug.noop");
}

//------------------------------------------------------------------------------

void Debug::Progress(const string& s)
{
   Debug::ft("Debug.Progress");

   CoutThread::Spool(s.c_str());
   ThisThread::Pause(msecs_t(10));
}

//------------------------------------------------------------------------------

void Debug::ResetSwFlags()
{
   Debug::ft("Debug.ResetSwFlags");

   SwFlags_.reset();
}

//------------------------------------------------------------------------------

void Debug::SetSwFlag(FlagId fid, bool value)
{
   Debug::ftnt("Debug.SetSwFlag");

   if(Element::RunningInLab() && (fid <= MaxFlagId))
   {
      SwFlags_.set(fid, value);

      //  To be reenabled, RootThread has to be signalled.
      //
      if((fid == DisableRootThread) && !value)
      {
         Singleton<RootThread>::Extant()->systhrd_->Proceed();
      }
   }
}

//------------------------------------------------------------------------------

bool Debug::SwFlagOn(FlagId fid)
{
   Debug::ftnt("Debug.SwFlagOn");

   if(Element::RunningInLab() && (fid <= MaxFlagId))
   {
      return SwFlags_.test(fid);
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name Debug_SwLog = "Debug.SwLog";

void Debug::SwLog(fn_name_arg func,
   const string& errstr, debug64_t errval, bool stack)
{
   Debug::ftnt(Debug_SwLog);

   if(ExitStatus_ == ExitCode_) return;

   if(!Thread::EnterSwLog()) return;

   Debug::ftnt(Debug_SwLog);

   auto log = Log::Create(SoftwareLogGroup, SoftwareError);

   if(log != nullptr)
   {
      *log << Log::Tab << "in ";
      if(func != nullptr)
         *log << func;
      else
         *log << "Unknown Function";
      *log << CRLF;

      *log << Log::Tab << "expl=" << errstr;
      if(!errstr.empty() && (errstr.back() == CRLF))
         *log << Log::Tab;
      else
         *log << spaces(2);
      *log << "errval=" << HexPrefixStr;
      *log << std::hex << errval << std::dec << CRLF;

      if(stack) SysStackTrace::Display(*log);
      Log::Submit(log);
   }

   Thread::ExitSwLog(false);
}

//------------------------------------------------------------------------------

string strOver(const Base* obj, bool ns)
{
   string result = "override not found in " + strClass(obj, ns);
   return result;
}
}
