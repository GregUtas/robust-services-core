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
#include "Clock.h"
#include "CoutThread.h"
#include "Debug.h"
#include "Element.h"
#include "FileThread.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "Log.h"
#include "Restart.h"
#include "Singleton.h"
#include "StreamRequest.h"
#include "SysConsole.h"
#include "SysFile.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
SysMutex LogThread::LogFileLock_;

//------------------------------------------------------------------------------

fn_name LogThread_ctor = "LogThread.ctor";

LogThread::LogThread() : Thread(BackgroundFaction)
{
   Debug::ft(LogThread_ctor);
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
   stream << "LogFileLock_ : " << CRLF;
   LogFileLock_.Display(stream, lead, options);
}

//------------------------------------------------------------------------------

fn_name LogThread_Enter = "LogThread.Enter";

void LogThread::Enter()
{
   Debug::ft(LogThread_Enter);

   while(true)
   {
      auto msg = DeqMsg(TIMEOUT_NEVER);
      auto req = static_cast< StreamRequest* >(msg);

      if(req == nullptr) continue;
      ostringstreamPtr log(req->TakeStream());

      delete msg;
      msg = nullptr;

      //  Add the log to the log file.  In the lab, also write a copy to
      //  the console.
      //
      if(Element::RunningInLab())
      {
         ostringstreamPtr clone(new std::ostringstream(log->str()));
         CoutThread::Spool(clone);
      }

      auto file = Log::FileName() + ".txt";
      FileThread::Spool(file, log);
   }
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

   if(log == nullptr) return;
   if(log->str().back() != CRLF) *log << CRLF;

   //  During a restart, our thread won't run, so output the log directly.
   //  This is done locked to avoid contention for the log file, since many
   //  threads come through here while exiting.
   //
   if(Restart::GetStatus() != Running)
   {
      auto rc = LogFileLock_.Acquire(TIMEOUT_NEVER);

      if(rc == SysMutex::Acquired)
      {
         auto path =
            Element::OutputPath() + PATH_SEPARATOR + Log::FileName() + ".txt";
         auto file = SysFile::CreateOstream(path.c_str());

         if(file != nullptr)
         {
            *file << log->str();
            file.reset();
         }

         LogFileLock_.Release();
      }

      if(Element::RunningInLab()) SysConsole::Out() << log->str() << std::flush;
      log.reset();
      return;
   }

   //  Forward the log to our thread.  This must be done unpreemptably
   //  because both this function (which runs on the client thread) and
   //  our Enter function contend for our message queue.  (If the client
   //  is in SystemFaction or higher, its priority effectively makes it
   //  unpreemptable.)
   //
   auto client = RunningThread(false);
   auto faction = (client != nullptr ? client->GetFaction() : SystemFaction);

   FunctionGuard
      guard(FunctionGuard::MakeUnpreemptable, faction <= PayloadFaction);

   auto request = new StreamRequest;

   if(request != nullptr)
   {
      request->GiveStream(log);
      Singleton< LogThread >::Instance()->EnqMsg(*request);
   }
   else
   {
      log.reset();
   }
}
}
