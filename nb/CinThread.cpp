//==============================================================================
//
//  CinThread.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CinThread.h"
#include <algorithm>
#include <istream>
#include <ostream>
#include <string>
#include "Clock.h"
#include "Debug.h"
#include "FunctionGuard.h"
#include "Memory.h"
#include "Restart.h"
#include "Singleton.h"
#include "SysConsole.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name CinThread_ctor = "CinThread.ctor";

CinThread::CinThread() : Thread(BackgroundFaction),
   size_(0),
   client_(nullptr)
{
   Debug::ft(CinThread_ctor);

   buff_[0] = '\0';
}

//------------------------------------------------------------------------------

fn_name CinThread_dtor = "CinThread.dtor";

CinThread::~CinThread()
{
   Debug::ft(CinThread_dtor);
}

//------------------------------------------------------------------------------

const char* CinThread::AbbrName() const
{
   return "cin";
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
   stream << prefix << "size   : " << size_ << CRLF;
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

      EnterBlockingOperation(BlockedOnConsole, CinThread_Enter);
         source.getline(buff_, BuffSize);
      ExitBlockingOperation(CinThread_Enter);

      size_ = source.gcount();

      //  If there was an error, clear it.
      //
      if(!source) source.clear();

      //  If characters were entered, sleep.  Sleeping gives the client
      //  time to wake up and process the input.  If there is no client,
      //  sleeping buffers the input until a client requests it.
      //
      if(size_ > 0)
      {
         if((client_ != nullptr) && !client_->IsInvalid()) client_->Interrupt();
         Pause(TIMEOUT_NEVER);
      }
   }
}

//------------------------------------------------------------------------------

fn_name CinThread_GetLine = "CinThread.GetLine";

std::streamsize CinThread::GetLine(char* buff, std::streamsize capacity)
{
   //  Do not read from the console during a restart.  It blocks a thread,
   //  which prevents it from exiting.
   //
   if(capacity <= 0) return StreamInterrupt;
   if(Restart::GetStatus() != Running) return StreamRestart;

   auto client = RunningThread();

   //  Make sure we're running unpreemptably, which will prevent more than
   //  one client from being in this code at the same time.
   //
   FunctionGuard guard(FunctionGuard::MakeUnpreemptable);

   Debug::ft(CinThread_GetLine);

   auto server = Singleton< CinThread >::Instance();

   if(server->size_ == 0)
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
      if(server->size_ == 0)
      {
         server->client_ = nullptr;
         return StreamInterrupt;
      }
   }

   //  Input is available.  Copy it to the client's buffer, reset the
   //  server's data, and awaken the server thread so that it can read
   //  the next input.
   //
   auto n = std::min(server->size_, capacity - 1);
   Memory::Copy(buff, server->buff_, n);
   buff[n] = '\0';

   server->buff_[0] = '\0';
   server->size_ = 0;
   server->client_ = nullptr;
   server->Interrupt();
   return n - 1;
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
   //  o an invalid client is currently registered
   //  o CLIENT is already registered
   //
   if((client_ == nullptr) || client_->IsInvalid())
   {
      client_ = client;
      return true;
   }

   return (client_ == client);
}
}
