//==============================================================================
//
//  CinThread.cpp
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
#include "CinThread.h"
#include <istream>
#include <ostream>
#include "Debug.h"
#include "Duration.h"
#include "FunctionGuard.h"
#include "Restart.h"
#include "Singleton.h"
#include "SysConsole.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CinThread_ctor = "CinThread.ctor";

CinThread::CinThread() : Thread(OperationsFaction),
   client_(nullptr)
{
   Debug::ft(CinThread_ctor);

   buff_[0] = NUL;
   SetInitialized();
}

//------------------------------------------------------------------------------

fn_name CinThread_dtor = "CinThread.dtor";

CinThread::~CinThread()
{
   Debug::ftnt(CinThread_dtor);
}

//------------------------------------------------------------------------------

c_string CinThread::AbbrName() const
{
   return "cin";
}

//------------------------------------------------------------------------------

void CinThread::ClearClient(const Thread* client)
{
   if(client_ == client) client_ = nullptr;
}

//------------------------------------------------------------------------------

fn_name CinThread_Destroy = "CinThread.Destroy";

void CinThread::Destroy()
{
   Debug::ft(CinThread_Destroy);

   Singleton< CinThread >::Destroy();
}

//------------------------------------------------------------------------------

void CinThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   stream << prefix << "buff   : " << buff_ << CRLF;
   stream << prefix << "client : " << client_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name CinThread_Enter = "CinThread.Enter";

void CinThread::Enter()
{
   //  Read input in advance and buffer it.  If a client is already waiting
   //  for input, interrupt it immediately.  It resumes execution in GetLine,
   //  which reports the input to the client.  Sleep forever.  When a client
   //  finally reads the console, GetLine awakens us to read the next input.
   //
   while(true)
   {
      Debug::ft(CinThread_Enter);

      auto& source = SysConsole::In();

      EnterBlockingOperation(BlockedOnStream, CinThread_Enter);
         std::getline(source, buff_);
      ExitBlockingOperation(CinThread_Enter);

      //  If there was an error, clear it.
      //
      if(!source) source.clear();

      //  If characters were entered, sleep.  Sleeping gives the client
      //  time to wake up and process the input.  If there is no client,
      //  sleeping buffers the input until a client requests it.
      //
      if(!buff_.empty())
      {
         if(client_ != nullptr) client_->Interrupt();
         Pause(TIMEOUT_NEVER);
      }
   }
}

//------------------------------------------------------------------------------

fn_name CinThread_GetLine = "CinThread.GetLine";

std::streamsize CinThread::GetLine(string& buff)
{
   Debug::ft(CinThread_GetLine);

   //  Do not read from the console during a restart.  It blocks a thread,
   //  which prevents it from exiting.
   //
   if(Restart::GetStage() != Running) return StreamRestart;

   auto client = RunningThread();

   //  Make sure we're running unpreemptably, which will prevent more than
   //  one client from being in this code at the same time.
   //
   FunctionGuard guard(Guard_MakeUnpreemptable);

   auto server = Singleton< CinThread >::Instance();

   if(server->buff_.empty())
   {
      //  Nothing is buffered.  Register the client and put it to sleep;
      //  the server thread will interrupt it when input is available.
      //
      if(!server->SetClient(client)) return StreamInUse;
      Pause(TIMEOUT_NEVER);

      //  When we put the client to sleep, another thread could interrupt
      //  it before console input arrives.  To receive input, the client
      //  must call this function again.
      //
      if(server->buff_.empty())
      {
         server->client_ = nullptr;
         return StreamInterrupt;
      }
   }

   //  Input is available.  Copy it to the client's buffer, reset the
   //  server's data, and awaken the server thread so that it can read
   //  the next input.
   //
   buff = server->buff_;
   server->buff_.clear();
   server->client_ = nullptr;
   guard.Release();

   server->Interrupt();
   return buff.size();
}

//------------------------------------------------------------------------------

void CinThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name CinThread_SetClient = "CinThread.SetClient";

bool CinThread::SetClient(Thread* client)
{
   Debug::ft(CinThread_SetClient);

   //  This succeeds if
   //  o no client is currently registered
   //  o CLIENT is already registered
   //
   if(client_ == nullptr)
   {
      client_ = client;
      return true;
   }

   return (client_ == client);
}
}
