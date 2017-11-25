//==============================================================================
//
//  CoutThread.cpp
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
#include "CoutThread.h"
#include <iosfwd>
#include <ostream>
#include <sstream>
#include <string>
#include "Clock.h"
#include "Debug.h"
#include "FunctionGuard.h"
#include "Restart.h"
#include "Singleton.h"
#include "StreamRequest.h"
#include "SysConsole.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CoutThread_ctor = "CoutThread.ctor";

CoutThread::CoutThread() : Thread(BackgroundFaction)
{
   Debug::ft(CoutThread_ctor);
}

//------------------------------------------------------------------------------

fn_name CoutThread_dtor = "CoutThread.dtor";

CoutThread::~CoutThread()
{
   Debug::ft(CoutThread_dtor);
}

//------------------------------------------------------------------------------

const char* CoutThread::AbbrName() const
{
   return "cout";
}

//------------------------------------------------------------------------------

fn_name CoutThread_Destroy = "CoutThread.Destroy";

void CoutThread::Destroy()
{
   Debug::ft(CoutThread_Destroy);

   Singleton< CoutThread >::Destroy();
}

//------------------------------------------------------------------------------

fn_name CoutThread_Enter = "CoutThread.Enter";

void CoutThread::Enter()
{
   Debug::ft(CoutThread_Enter);

   while(true)
   {
      auto msg = DeqMsg(TIMEOUT_NEVER);
      auto req = static_cast< StreamRequest* >(msg);

      if(req == nullptr) continue;
      ostringstreamPtr stream(req->TakeStream());

      delete msg;
      msg = nullptr;

      FunctionGuard guard(FunctionGuard::MakePreemptable);
      SysConsole::Out() << stream->str() << std::flush;
      stream.reset();
   }
}

//------------------------------------------------------------------------------

void CoutThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CoutThread_Spool1 = "CoutThread.Spool(stream)";

void CoutThread::Spool(ostringstreamPtr& stream)
{
   Debug::ft(CoutThread_Spool1);

   if(stream == nullptr) return;

   //  During a restart, our thread won't run, so output the stream directly.
   //
   if(Restart::GetStatus() != Running)
   {
      SysConsole::Out() << stream->str() << std::flush;
      stream.reset();
      return;
   }

   //  Forward the stream to our thread.  This must be done unpreemptably
   //  because both this function (which runs on the client thread) and
   //  our Enter function contend for our message queue.  (If the client
   //  is in SystemFaction or higher, its priority effectively makes it
   //  unpreemptable.)
   //
   auto client = RunningThread();
   auto faction = client->GetFaction();

   FunctionGuard
      guard(FunctionGuard::MakeUnpreemptable, faction <= PayloadFaction);

   auto request = new StreamRequest;

   if(request != nullptr)
   {
      request->GiveStream(stream);
      Singleton< CoutThread >::Instance()->EnqMsg(*request);
   }
   else
   {
      stream.reset();
   }
}

//------------------------------------------------------------------------------

fn_name CoutThread_Spool2 = "CoutThread.Spool(string)";

void CoutThread::Spool(const char* s, bool eol)
{
   Debug::ft(CoutThread_Spool2);

   ostringstreamPtr stream(new std::ostringstream);
   if(stream == nullptr) return;
   *stream << s;
   if(eol) *stream << CRLF;
   Spool(stream);
}
}
