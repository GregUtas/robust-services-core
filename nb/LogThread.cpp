//==============================================================================
//
//  LogThread.cpp
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
#include "LogThread.h"
#include <cstddef>
#include <iosfwd>
#include <sstream>
#include <string>
#include "CallbackRequest.h"
#include "CfgIntParm.h"
#include "CfgParmRegistry.h"
#include "Clock.h"
#include "CoutThread.h"
#include "Debug.h"
#include "Element.h"
#include "FileThread.h"
#include "Formatters.h"
#include "Log.h"
#include "LogBuffer.h"
#include "LogBufferRegistry.h"
#include "MutexGuard.h"
#include "NbPools.h"
#include "Restart.h"
#include "Singleton.h"
#include "SysFile.h"
#include "SysMutex.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  To prevent interleaved output in the log file.
//
SysMutex LogFileLock_("LogFileLock");

//------------------------------------------------------------------------------

word LogThread::NoSpoolingMessageCount_ = 400;

//------------------------------------------------------------------------------

fn_name LogThread_ctor = "LogThread.ctor";

LogThread::LogThread() : Thread(BackgroundFaction)
{
   Debug::ft(LogThread_ctor);

   auto reg = Singleton< CfgParmRegistry >::Instance();

   noSpoolingMessageCount_.reset
      (static_cast< CfgIntParm* >(reg->FindParm("NoSpoolingMessageCount")));

   if(noSpoolingMessageCount_ == nullptr)
   {
      noSpoolingMessageCount_.reset
         (new CfgIntParm("NoSpoolingMessageCount", "400",
         &NoSpoolingMessageCount_, 200, 600,
         "messages reserved for work other than spooling logs"));
      reg->BindParm(*noSpoolingMessageCount_);
   }

   SetInitialized();
}

//------------------------------------------------------------------------------

fn_name LogThread_dtor = "LogThread.dtor";

LogThread::~LogThread()
{
   Debug::ft(LogThread_dtor);

   //  Clear our configuration parameter so that it won't be deleted.
   //  (it resides in protected memory.)  When we are recreated, our
   //  constructor will discover that it still exists.
   //
   noSpoolingMessageCount_.release();
}

//------------------------------------------------------------------------------

c_string LogThread::AbbrName() const
{
   return "log";
}

//------------------------------------------------------------------------------

fn_name LogThread_CopyToConsole = "LogThread.CopyToConsole";

void LogThread::CopyToConsole(const ostringstreamPtr& stream)
{
   Debug::ft(LogThread_CopyToConsole);

   //  In a lab load, display the logs on the console.
   //
   if(Element::RunningInLab())
   {
      ostringstreamPtr clone(new std::ostringstream(stream->str()));
      CoutThread::Spool(clone);
   }
}

//------------------------------------------------------------------------------

fn_name LogThread_Destroy = "LogThread.Destroy";

void LogThread::Destroy()
{
   Debug::ft(LogThread_Destroy);

   Singleton< LogThread >::Destroy();
}

//------------------------------------------------------------------------------

void LogThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   stream << prefix << "NoSpoolingMessageCount : ";
   stream << NoSpoolingMessageCount_ << CRLF;
   stream << prefix << "noSpoolingMessageCount : ";
   stream << strObj(noSpoolingMessageCount_.get()) << CRLF;
}

//------------------------------------------------------------------------------

fn_name LogThread_Enter = "LogThread.Enter";

void LogThread::Enter()
{
   Debug::ft(LogThread_Enter);

   auto reg = Singleton< LogBufferRegistry >::Instance();
   auto msgs = Singleton< MsgBufferPool >::Instance();
   auto delay = TIMEOUT_NEVER;

   //  The log thread usually pauses forever and is interrupted when a log
   //  is added to the log buffer.  However, it only pauses for 1 second if
   //  the number of remaining MsgBuffers was too low to use any for log
   //  spooling.  And when the log buffer still has entries after spooling
   //  the first set of logs, the log thread only yields before resuming.
   //
   while(true)
   {
      Pause(delay);

      if(msgs->AvailCount() <= size_t(NoSpoolingMessageCount_))
      {
         delay = TIMEOUT_1_SEC;  // wait for more MsgBuffers
         continue;
      }

      auto buff = reg->Active();
      CallbackRequestPtr callback;
      auto periodic = false;
      auto stream = buff->GetLogs(callback, periodic);

      if(stream == nullptr)
      {
         delay = TIMEOUT_NEVER;  // log buffer is empty
         continue;
      }

      delay = TIMEOUT_IMMED;  // still more logs in buffer

      //  Add the log to the log file and possibly the console.
      //
      if(!periodic) CopyToConsole(stream);
      FileThread::Spool(buff->FileName(), stream, callback);
   }
}

//------------------------------------------------------------------------------

void LogThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name LogThread_Spool = "LogThread.Spool";

void LogThread::Spool(ostringstreamPtr& stream, const Log* log)
{
   Debug::ft(LogThread_Spool);

   //  This is only intended to be invoked during a restart.  Our thread won't
   //  get to run, so output the log directly.  This is done locked to avoid
   //  contention for the log file, because many threads come through here to
   //  generate an exit log during the shutdown phase.
   //
   auto level = Restart::GetStatus();
   Debug::Assert(level != Running, level);

   if((log == nullptr) || (GetLogType(log->Id()) != PeriodicLog))
   {
      CopyToConsole(stream);
   }

   auto name = Singleton< LogBufferRegistry >::Instance()->FileName();
   auto path = Element::OutputPath() + PATH_SEPARATOR + name;

   MutexGuard guard(&LogFileLock_);

   auto file = SysFile::CreateOstream(path.c_str());

   if(file != nullptr)
   {
      *file << stream->str();
      file.reset();
   }

   stream.reset();
}
}
