//==============================================================================
//
//  CoutThread.cpp
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
#include "CoutThread.h"
#include <iosfwd>
#include <ostream>
#include <sstream>
#include <string>
#include "Debug.h"
#include "Duration.h"
#include "FileThread.h"
#include "FunctionGuard.h"
#include "MutexGuard.h"
#include "Restart.h"
#include "Singleton.h"
#include "StreamRequest.h"
#include "SysConsole.h"
#include "SysMutex.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  For serializing access to our message queue.
//
static SysMutex CoutThreadMsgQLock_("CoutThreadMsgQLock");

//------------------------------------------------------------------------------

CoutThread::CoutThread() : Thread(BackgroundFaction)
{
   Debug::ft("CoutThread.ctor");

   SetInitialized();
}

//------------------------------------------------------------------------------

CoutThread::~CoutThread()
{
   Debug::ftnt("CoutThread.dtor");
}

//------------------------------------------------------------------------------

c_string CoutThread::AbbrName() const
{
   return "cout";
}

//------------------------------------------------------------------------------

void CoutThread::Destroy()
{
   Debug::ft("CoutThread.Destroy");

   Singleton< CoutThread >::Destroy();
}

//------------------------------------------------------------------------------

void CoutThread::Enter()
{
   Debug::ft("CoutThread.Enter");

   while(true)
   {
      auto msg = DeqMsg(TIMEOUT_NEVER);
      auto req = static_cast< StreamRequest* >(msg);

      if(req == nullptr) continue;
      ostringstreamPtr stream(req->TakeStream());

      delete msg;
      msg = nullptr;

      FunctionGuard guard(Guard_MakePreemptable);
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

void CoutThread::Spool(ostringstreamPtr& stream)
{
   Debug::ft("CoutThread.Spool(stream)");

   if(stream == nullptr) return;

   //  Copy the output to the console transcript file.
   //
   FileThread::Record(stream->str());

   //  During a restart, our thread won't run, so output the stream directly.
   //
   if(Restart::GetStage() != Running)
   {
      SysConsole::Out() << stream->str() << std::flush;
      stream.reset();
      return;
   }

   //  Forward the stream to our thread.
   //
   auto request = new StreamRequest;
   request->GiveStream(stream);

   //  This function runs on the client thread, so it contends for our
   //  message queue with our Enter function.  Although it's unlikely,
   //  the client could be preemptable or of higher priority.
   //
   MutexGuard guard(&CoutThreadMsgQLock_);
   Singleton< CoutThread >::Instance()->EnqMsg(*request);
}

//------------------------------------------------------------------------------

void CoutThread::Spool(c_string s, bool eol)
{
   Debug::ft("CoutThread.Spool(string)");

   ostringstreamPtr stream(new std::ostringstream);
   *stream << s;
   if(eol) *stream << CRLF;
   Spool(stream);
}
}
