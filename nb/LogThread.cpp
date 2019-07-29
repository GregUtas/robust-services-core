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
#include <iosfwd>
#include <ostream>
#include <sstream>
#include <string>
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
#include "SysConsole.h"
#include "SysFile.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
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
}

//------------------------------------------------------------------------------

fn_name LogThread_dtor = "LogThread.dtor";

LogThread::~LogThread()
{
   Debug::ft(LogThread_dtor);
}

//------------------------------------------------------------------------------

const char* LogThread::AbbrName() const
{
   return "log";
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

   auto lead = prefix + spaces(2);
   stream << prefix << "lock : " << CRLF;
   lock_.Display(stream, lead, options);
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

      auto buff = reg->First();
      auto stream = GetLogsFromBuffer(buff);

      if(stream == nullptr)
      {
         delay = TIMEOUT_NEVER;  // log buffer is empty
         continue;
      }

      delay = TIMEOUT_IMMED;  // still more logs in buffer

      //  Add the log to the log file.  In the lab, also write a copy to
      //  the console.
      //
      if(Element::RunningInLab())
      {
         ostringstreamPtr clone(new std::ostringstream(stream->str()));
         CoutThread::Spool(clone);
      }

      FileThread::Spool(buff->FileName(), stream);
   }
}

//------------------------------------------------------------------------------

ostringstreamPtr LogThread::GetLogsFromBuffer(LogBuffer* buffer)
{
   //  If the log buffer contains any logs, create a stream to spool them.
   //
   if(buffer == nullptr) return nullptr;

   MutexGuard guard(buffer->GetLock());

   auto entry = buffer->First();
   if(entry == nullptr) return nullptr;
   ostringstreamPtr log(new std::ostringstream);
   if(log == nullptr) return nullptr;

   //  Accumulate logs until they exceed the size limit.  But first, insert
   //  a warning if some logs were discarded because the buffer was full.
   //
   auto discards = buffer->Discards();

   if(discards > 0)
   {
      *log << CRLF << Log::Tab << "WARNING: ";
      *log << discards << " log(s) discarded";
      *log << CRLF;
      buffer->ResetDiscards();
   }

   while((log->str().size() < BundledLogSizeThreshold) && (entry != nullptr))
   {
      *log << static_cast< const char* >(entry->log);
      buffer->Next(entry);
      buffer->Pop();  //* move to callback invoked after StreamRequest handled
   }

   return log;
}

//------------------------------------------------------------------------------

void LogThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name LogThread_Spool = "LogThread.Spool";

void LogThread::Spool(ostringstreamPtr& log)
{
   Debug::ft(LogThread_Spool);

   //  This is only intended to be invoked during a restart.  Our thread won't
   //  get to run, so output the log directly.  This is done locked to avoid
   //  contention for the log file, because many threads come through here to
   //  generate an exit log during the shutdown phase.
   //
   auto level = Restart::GetStatus();
   Debug::Assert(level != Running, level);

   MutexGuard guard(&lock_);

   auto name = Singleton< LogBufferRegistry >::Instance()->FileName();
   auto path = Element::OutputPath() + PATH_SEPARATOR + name;
   auto file = SysFile::CreateOstream(path.c_str());

   if(file != nullptr)
   {
      *file << log->str();
      file.reset();
   }

   if(Element::RunningInLab()) SysConsole::Out() << log->str() << std::flush;
   log.reset();
}
}
