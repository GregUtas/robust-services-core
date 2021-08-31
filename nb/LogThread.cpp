//==============================================================================
//
//  LogThread.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include <iosfwd>
#include <new>
#include <ostream>
#include <sstream>
#include <string>
#include "CallbackRequest.h"
#include "CfgParmRegistry.h"
#include "CoutThread.h"
#include "Debug.h"
#include "Duration.h"
#include "Element.h"
#include "FileThread.h"
#include "Formatters.h"
#include "Log.h"
#include "LogBuffer.h"
#include "LogBufferRegistry.h"
#include "MutexGuard.h"
#include "NbDaemons.h"
#include "NbPools.h"
#include "Restart.h"
#include "Singleton.h"
#include "SysConsole.h"
#include "SysFile.h"
#include "SysMutex.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//  To prevent interleaved output in the log file.
//
static SysMutex LogFileLock_("LogFileLock");

//------------------------------------------------------------------------------
//
//  Copies the STREAM of logs to the console when appropriate.
//
static void CopyToConsole(const ostringstreamPtr& stream)
{
   Debug::ft("NodeBase.CopyToConsole");

   //  In a lab load, display the logs on the console.
   //
   if(Element::RunningInLab())
   {
      ostringstreamPtr clone
         (new (std::nothrow) std::ostringstream(stream->str()));
      if(clone != nullptr) CoutThread::Spool(clone);
   }
}

//------------------------------------------------------------------------------

LogThread::LogThread() :
   Thread(BackgroundFaction, Singleton< LogDaemon >::Instance())
{
   Debug::ft("LogThread.ctor");

   auto reg = Singleton< CfgParmRegistry >::Instance();

   noSpoolingMessageCount_.reset
      (static_cast< CfgIntParm* >(reg->FindParm("NoSpoolingMessageCount")));

   if(noSpoolingMessageCount_ == nullptr)
   {
      noSpoolingMessageCount_.reset
         (new CfgIntParm("NoSpoolingMessageCount", "400", 200, 600,
         "messages reserved for work other than spooling logs"));
      reg->BindParm(*noSpoolingMessageCount_);
   }

   SetInitialized();
}

//------------------------------------------------------------------------------

LogThread::~LogThread()
{
   Debug::ftnt("LogThread.dtor");

   //  Clear our configuration parameter so that it won't be deleted.
   //  (it resides in protected memory.)  When we are recreated, our
   //  constructor will discover that it still exists.
   //
   noSpoolingMessageCount_.release();
}

//------------------------------------------------------------------------------

c_string LogThread::AbbrName() const
{
   return LogDaemonName;
}

//------------------------------------------------------------------------------

void LogThread::Destroy()
{
   Debug::ft("LogThread.Destroy");

   Singleton< LogThread >::Destroy();
}

//------------------------------------------------------------------------------

void LogThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   stream << prefix << "noSpoolingMessageCount : ";
   stream << strObj(noSpoolingMessageCount_.get()) << CRLF;
}

//------------------------------------------------------------------------------

void LogThread::Enter()
{
   Debug::ft("LogThread.Enter");

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

      if(msgs->AvailCount() <= NoSpoolingMessageCount())
      {
         delay = ONE_SEC;  // wait for more MsgBuffers
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
   Debug::ftnt(LogThread_Spool);

   //  This is only intended to be invoked during a restart.  Our thread won't
   //  get to run, so output the log directly.  This is done locked to avoid
   //  contention for the log file, because many threads come through here to
   //  generate an exit log during the shutdown phase.
   //
   auto level = Restart::GetStage();

   if(level == Running)
   {
      Debug::SwLog(LogThread_Spool, "invoked while in service", 0);
      return;
   }

   if((log == nullptr) || (GetLogType(log->Id()) != PeriodicLog))
   {
      //  In a lab load, write the log to the console and the console
      //  transcript file.
      //
      if(Element::RunningInLab())
      {
         SysConsole::Out() << stream->str() << std::flush;

         auto path = Element::OutputPath() +
            PATH_SEPARATOR + Element::ConsoleFileName() + ".txt";
         auto file = SysFile::CreateOstream(path.c_str());

         if(file != nullptr)
         {
            *file << stream->str();
            file.reset();
         }
      }
   }

   auto name = Singleton< LogBufferRegistry >::Extant()->FileName();
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
