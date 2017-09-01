//==============================================================================
//
//  LogThread.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
#include "FunctionGuard.h"
#include "Log.h"
#include "Restart.h"
#include "Singleton.h"
#include "StreamRequest.h"
#include "SysConsole.h"
#include "SysFile.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
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

   //  During a restart, our thread won't run, so output the log directly.
   //
   if(Restart::GetStatus() != Running)
   {
      auto path =
         Element::OutputPath() + PATH_SEPARATOR + Log::FileName() + ".txt";
      auto file = ostreamPtr(SysFile::CreateOstream(path.c_str()));

      if(file != nullptr)
      {
         *file << log->str();
         file.reset();
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
