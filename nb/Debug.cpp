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
#include <sstream>
#include "CoutThread.h"
#include "Element.h"
#include "Formatters.h"
#include "InitFlags.h"
#include "Log.h"
#include "NbAppIds.h"
#include "SoftwareException.h"
#include "SysThreadStack.h"
#include "ThisThread.h"

using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
std::atomic_flag Debug::FtLock_ = ATOMIC_FLAG_INIT;
Flags Debug::FcFlags_ = Flags(InitFlags::TraceInit() ? 1 << TracingActive : 0);
Flags Debug::SwFlags_ = Flags();

//------------------------------------------------------------------------------

void Debug::ft(fn_name_arg func)
{
   if(FcFlags_.none()) return;
   if(FtLock_.test_and_set()) return;
   Thread::FunctionInvoked(func);
   FtLock_.clear();
}

//------------------------------------------------------------------------------

fn_name Debug_GenerateSwLog = "Debug.GenerateSwLog";

void Debug::GenerateSwLog(fn_name_arg func, const string& errstr,
      debug64_t offset, LogLevel level, fixed_string title)
{
   if(!Thread::EnterSwLog()) return;

   Debug::ft(Debug_GenerateSwLog);

   if(level == ErrorLog)
   {
      throw SoftwareException(errstr, offset, 3);
   }

   ostringstreamPtr log;

   if(title == nullptr)
      log = Log::Create("SOFTWARE ERROR");
   else
      log = Log::Create(title);

   if(log != nullptr)
   {
      *log << level << " in ";
      if(func != nullptr)
         *log << func;
      else
         *log << "Unknown Function";
      *log << CRLF;

      *log << "errval=" << errstr;
      *log << " offset=" << strHex(offset) << CRLF;

      if(level != InfoLog) SysThreadStack::Display(*log, 1);
      Log::Spool(log);
   }

   Thread::ExitSwLog(false);
}

//------------------------------------------------------------------------------

fn_name Debug_GetSwFlags = "Debug.GetSwFlags";

Flags Debug::GetSwFlags()
{
   Debug::ft(Debug_GetSwFlags);

   if(Element::RunningInLab()) return SwFlags_;
   return Flags();
}

//------------------------------------------------------------------------------

fn_name Debug_noop = "Debug.noop";

void Debug::noop()
{
   Debug::ft(Debug_noop);
}

//------------------------------------------------------------------------------

fn_name Debug_Progress = "Debug.Progress";

void Debug::Progress(const string& s, bool eol, bool force)
{
   Debug::ft(Debug_Progress);

   if(force || SwFlagOn(ShowToolProgress))
   {
      CoutThread::Spool(s.c_str(), eol);
      ThisThread::Pause(10);
   }
}

//------------------------------------------------------------------------------

void Debug::Reset()
{
   FtLock_.clear();
   Thread::ExitSwLog(true);
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
   Debug::ft(Debug_SetSwFlag);

   if(Element::RunningInLab() && (fid <= MaxFlagId))
   {
      SwFlags_.set(fid, value);
   }
}

//------------------------------------------------------------------------------

fn_name Debug_SwErr1 = "Debug.SwErr(int)";

void Debug::SwErr(fn_name_arg func, debug64_t errval, debug64_t offset,
   LogLevel level, fixed_string title)
{
   Debug::ft(Debug_SwErr1);

   GenerateSwLog(func, std::to_string(errval), offset, level, title);
}

//------------------------------------------------------------------------------

fn_name Debug_SwErr2 = "Debug.SwErr(string)";

void Debug::SwErr(fn_name_arg func, const string& errstr, debug64_t offset,
   LogLevel level, fixed_string title)
{
   Debug::ft(Debug_SwErr2);

   GenerateSwLog(func, errstr, offset, level, title);
}

//------------------------------------------------------------------------------

fn_name Debug_SwFlagOn = "Debug.SwFlagOn";

bool Debug::SwFlagOn(FlagId fid)
{
   Debug::ft(Debug_SwFlagOn);

   if(Element::RunningInLab() && (fid <= MaxFlagId))
   {
      return SwFlags_.test(fid);
   }

   return false;
}
}
