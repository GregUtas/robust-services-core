//==============================================================================
//
//  Debug.cpp
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
#include "Debug.h"
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
#include "SoftwareException.h"
#include "SysThread.h"
#include "SysThreadStack.h"
#include "ThisThread.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fixed_string UnexpectedInvocation = "unexpected invocation";

Flags Debug::SwFlags_ = Flags();
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
//
//  The cost of function tracing was assessed by running POTS traffic while
//  tracing threads in the Payload faction only.  The results, determined by
//  when the system entered overload, were
//  o tracing off: 17000 calls/minute
//  o tracing on: 4000 calls/minute
//
void Debug::ft(fn_name_arg func)
{
   if(FcFlags_.none()) return;
   Thread::FunctionInvoked(func);
}

//------------------------------------------------------------------------------

void Debug::ftnt(fn_name_arg func)
{
   if(FcFlags_.none()) return;
   Thread::FunctionInvoked(func, std::nothrow);
}

//------------------------------------------------------------------------------

fn_name Debug_GetSwFlags = "Debug.GetSwFlags";

Flags Debug::GetSwFlags()
{
   Debug::ft(Debug_GetSwFlags);

   if(Element::RunningInLab()) return SwFlags_;
   return NoFlags;
}

//------------------------------------------------------------------------------

fn_name Debug_noop = "Debug.noop";

void Debug::noop()
{
   Debug::ft(Debug_noop);
}

//------------------------------------------------------------------------------

fn_name Debug_Progress = "Debug.Progress";

void Debug::Progress(const string& s)
{
   Debug::ft(Debug_Progress);

   CoutThread::Spool(s.c_str());
   ThisThread::Pause(Duration(10, mSECS));
}

//------------------------------------------------------------------------------

fn_name Debug_ResetSwFlags = "Debug.ResetSwFlags";

void Debug::ResetSwFlags()
{
   Debug::ft(Debug_ResetSwFlags);

   SwFlags_.reset();
}

//------------------------------------------------------------------------------

fn_name Debug_SetSwFlag = "Debug.SetSwFlag";

void Debug::SetSwFlag(FlagId fid, bool value)
{
   Debug::ftnt(Debug_SetSwFlag);

   if(Element::RunningInLab() && (fid <= MaxFlagId))
   {
      SwFlags_.set(fid, value);

      //  To be reenabled, RootThread has to be signalled.
      //
      if((fid == DisableRootThread) && !value)
      {
         Singleton< RootThread >::Extant()->systhrd_->Proceed();
      }
   }
}

//------------------------------------------------------------------------------

fn_name Debug_SwErr = "Debug.SwErr";

void Debug::SwErr(const string& errstr, debug64_t offset)
{
   Debug::ft(Debug_SwErr);

   throw SoftwareException(errstr, offset, 1);
}

//------------------------------------------------------------------------------

fn_name Debug_SwFlagOn = "Debug.SwFlagOn";

bool Debug::SwFlagOn(FlagId fid)
{
   Debug::ftnt(Debug_SwFlagOn);

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
      *log << "  errval=" << HexPrefixStr;
      *log << std::hex << errval << std::dec << CRLF;

      if(stack) SysThreadStack::Display(*log, 1);
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
