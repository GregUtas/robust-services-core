//==============================================================================
//
//  TcpIoThread.cpp
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
#include "TcpIoThread.h"
#include <sstream>
#include <string>
#include "Clock.h"
#include "Debug.h"
#include "Formatters.h"
#include "InputHandler.h"
#include "IpPort.h"
#include "IpPortRegistry.h"
#include "Log.h"
#include "NbTypes.h"
#include "NwTrace.h"
#include "Singleton.h"
#include "SysIpL3Addr.h"
#include "SysTcpSocket.h"
#include "TcpIpService.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
const size_t TcpIoThread::MaxConns = 48 * 1024;  // 48K

//------------------------------------------------------------------------------

fn_name TcpIoThread_ctor = "TcpIoThread.ctor";

TcpIoThread::TcpIoThread(const TcpIpService* service, ipport_t port) :
   IoThread(service, port),
   listen_(true),
   ready_(0),
   curr_(0)
{
   Debug::ft(TcpIoThread_ctor);

   ipPort_ = Singleton< IpPortRegistry >::Instance()->GetPort(port_, IpTcp);

   if(ipPort_ == nullptr)
   {
      Debug::SwLog(TcpIoThread_ctor, port_, 0);
   }

   listen_ = service->AcceptsConns();

   auto fdSize = service->MaxConns();

   if(fdSize > MaxConns)
   {
      Debug::SwLog(TcpIoThread_ctor, fdSize, MaxConns);
      fdSize = MaxConns;
   }
   else if(listen_)
   {
      if(fdSize < 2)
      {
         Debug::SwLog(TcpIoThread_ctor, fdSize, 2);
         fdSize = 2;
      }
   }
   else
   {
      if(fdSize < 1) Debug::SwLog(TcpIoThread_ctor, fdSize, 1);
      fdSize = 1;
   }

   //  Allocate the maximum size of the sockets_ array immediately.  This is
   //  important because if the array gets extended (and therefore moves) at
   //  run-time, SysTcpSocket::Poll will fail spectacularly if it was blocked
   //  on its polling operation when the resizing occurred.
   //
   sockets_.Init(fdSize, MemDyn);
   sockets_.Reserve(fdSize);
}

//------------------------------------------------------------------------------

fn_name TcpIoThread_dtor = "TcpIoThread.dtor";

TcpIoThread::~TcpIoThread()
{
   Debug::ft(TcpIoThread_dtor);

   ReleaseResources();
}

//------------------------------------------------------------------------------

const char* TcpIoThread::AbbrName() const
{
   return "tcpio";
}

//------------------------------------------------------------------------------

fn_name TcpIoThread_AcceptConn = "TcpIoThread.AcceptConn";

bool TcpIoThread::AcceptConn()
{
   Debug::ft(TcpIoThread_AcceptConn);

   if(!listen_) return false;
   auto listener = Listener();
   auto flags = listener->OutFlags();

   SysIpL3Addr peer;
   listener->SetBlocking(false);
   auto socket = listener->Accept(peer);

   if(socket == nullptr)
   {
      //  If PollRead is no longer set, it means that we have handled all
      //  connection requests.
      //
      if(flags->test(PollRead))
      {
         //s Handle Accept() error.
         //
         OutputLog("TCP ACCEPT ERROR", SocketError, listener);
      }

      return false;
   }

   socket->TracePeer(NwTrace::Accept, port_, peer, 0);

   //  A socket was created for a new connection.  A unique_ptr owns it, so
   //  returning without invoking socket.release() will cause its deletion.
   //  This occurs if it cannot be configured for its service or if our socket
   //  array is full.  In those cases, the socket will be immediately closed.
   //  However, a connection request was pending, so return true nonetheless.
   //
   auto rc = socket->SetService(ipPort_->GetService(), false);
   if(rc != SysSocket::AllocOk) return true;
   if(!InsertSocket(socket.get())) return true;
   socket.release();
   return true;
}

//------------------------------------------------------------------------------

fn_name TcpIoThread_AllocateListener = "TcpIoThread.AllocateListener";

SysTcpSocket* TcpIoThread::AllocateListener()
{
   Debug::ft(TcpIoThread_AllocateListener);

   //  Release any listener registered with our port.
   //
   auto registrant = static_cast< SysTcpSocket* >(ipPort_->GetSocket());

   if(registrant != nullptr)
   {
      registrant->Purge();
      ipPort_->SetSocket(nullptr);
   }

   //  Allocate a new listener.
   //
   auto svc = static_cast< const TcpIpService* >(ipPort_->GetService());
   auto rc = SysSocket::AllocFailed;
   SysTcpSocketPtr socket(new SysTcpSocket(port_, svc, rc));

   if(socket == nullptr)
      return ListenerError(0);

   if(rc != SysSocket::AllocOk)
      return ListenerError(socket->GetError());

   socket->TracePort(NwTrace::Listen, port_, svc->MaxBacklog());

   if(!socket->Listen(svc->MaxBacklog()))
      return ListenerError(socket->GetError());

   Debug::Assert(ipPort_->SetSocket(socket.get()));

   //  If we're replacing our listener, make sure it was released.
   //
   auto listener = Listener();
   auto result = socket.release();

   if(listener == nullptr)
   {
      sockets_.PushBack(result);
   }
   else
   {
      if(listener != registrant) listener->Purge();
      sockets_.Replace(0, result);
   }

   return result;
}

//------------------------------------------------------------------------------

fn_name TcpIoThread_ClaimBlocks = "TcpIoThread.ClaimBlocks";

void TcpIoThread::ClaimBlocks()
{
   Debug::ft(TcpIoThread_ClaimBlocks);

   IoThread::ClaimBlocks();

   for(size_t i = 0; i < sockets_.Size(); ++i)
   {
      sockets_[i]->ClaimBlocks();
   }
}

//------------------------------------------------------------------------------

fn_name TcpIoThread_Cleanup = "TcpIoThread.Cleanup";

void TcpIoThread::Cleanup()
{
   Debug::ft(TcpIoThread_Cleanup);

   ReleaseResources();
   Thread::Cleanup();
}

//------------------------------------------------------------------------------

void TcpIoThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   IoThread::Display(stream, prefix, options);

   stream << prefix << "listen : " << listen_ << CRLF;
   stream << prefix << "curr   : " << curr_ << CRLF;
   stream << prefix << "ready  : " << ready_ << CRLF;
   stream << prefix << "size   : " << sockets_.Size() << CRLF;

   if(!options.test(DispVerbose)) return;

   auto lead = prefix + spaces(2);
   stream << prefix << "sockets  : " << CRLF;

   for(size_t i = 0; i < sockets_.Size(); ++i)
   {
      stream << lead << strIndex(i) << sockets_[i] << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name TcpIoThread_EnsureListener = "TcpIoThread.EnsureListener";

bool TcpIoThread::EnsureListener()
{
   Debug::ft(TcpIoThread_EnsureListener);

   //  This is invoked to
   //  o to find the listener registered with our port
   //  o to allocate a listener if one is not registered with our port
   //  o to replace the listener if it has failed
   //
   if(!listen_) return true;
   auto registrant = static_cast< SysTcpSocket* >(ipPort_->GetSocket());
   auto listener = Listener();

   if(registrant == nullptr)
   {
      if(listener == nullptr)
      {
         //  Allocate the initial listener.
         //
         return AllocateListener();
      }

      //  Our listener isn't registered with our port.
      //
      Debug::SwLog(TcpIoThread_EnsureListener, port_, 0);
      if(ListenerHasFailed(listener)) return AllocateListener();
      Debug::Assert(ipPort_->SetSocket(listener));
      return listener;
   }

   if(listener == nullptr)
   {
      //  We don't have a listener, but our port does.  Use
      //  it unless it has failed.
      //
      if(ListenerHasFailed(registrant))
         return AllocateListener();
      sockets_.PushBack(registrant);
      return registrant;
   }

   if(registrant != listener)
   {
      //  A different listener is registered with our port.
      //
      Debug::SwLog(TcpIoThread_EnsureListener, port_, 1);
      ipPort_->SetSocket(nullptr);
      Debug::Assert(ipPort_->SetSocket(listener));
   }

   if(ListenerHasFailed(listener)) return AllocateListener();
   return listener;
}

//------------------------------------------------------------------------------

fn_name TcpIoThread_Enter = "TcpIoThread.Enter";

void TcpIoThread::Enter()
{
   Debug::ft(TcpIoThread_Enter);

   PollFlags* flags = nullptr;
   size_t first = (listen_ ? 1 : 0);

   //  Exit if a listener socket cannot be created.
   //
   if(!EnsureListener()) return;

   while(true)
   {
      //  Wait for socket events.  If we have no sockets, sleep until
      //  InsertSocket wakes us.
      //
      if(sockets_.Empty()) Pause(TIMEOUT_NEVER);

      ready_ = PollSockets();

      if(ready_ < 0)
      {
         //s Handle Poll() error.
         //
         OutputLog("TCP POLL ERROR", SocketError, sockets_.Front());
         Pause(20);
         continue;
      }

      //  If the listener has a pending event, adjust the ready count so
      //  that servicing of application sockets will stop as soon as the
      //  last application socket with a pending event has been handled.
      //
      if(listen_)
      {
         flags = Listener()->OutFlags();
         if(flags->any()) --ready_;
      }

      //  Before looking for new connection requests on the listener,
      //  service the application sockets with pending events.  This
      //  follows the overload control principle of handling progress
      //  work (existing sockets) before accepting new work.
      //
      host_ = IpPortRegistry::HostAddress();

      for(curr_ = first; curr_ < sockets_.Size(); ++curr_)
      {
         ServiceSocket();
         ConditionalPause(90);
      }

      //  Service connection requests on the listener.
      //
      while(AcceptConn())
      {
         ++recvs_;
         ConditionalPause(90);
      }

      //  If the listener has another flag set, it is probably an error.
      //
      if((flags != nullptr) && (flags->any()))
      {
         if(!EnsureListener()) return;
      }
   }
}

//------------------------------------------------------------------------------

fn_name TcpIoThread_EraseSocket = "TcpIoThread.EraseSocket";

void TcpIoThread::EraseSocket(size_t& index)
{
   Debug::ft(TcpIoThread_EraseSocket);

   //  Release the socket unless it's the listening socket.
   //
   if(listen_ && (index == 0))
   {
      Debug::SwLog(TcpIoThread_EraseSocket, 0, 0);
      return;
   }

   //  Fetch the socket to be released and remove it from the array.
   //
   auto socket = sockets_[index];
   sockets_.Erase(index);
   --index;

   auto handler = ipPort_->GetHandler();
   auto deleted = false;

   //  If the socket was invalid, nullify it, otherwise release it
   //  (which deletes it if the application has also released it).
   //
   if(socket->OutFlags()->test(PollInvalid))
      socket->Invalidate();
   else
      deleted = socket->Deregister();

   //  If the socket has not been deleted, the application has not
   //  yet released it, so inform it that the socket has failed.
   //
   if(!deleted) handler->SocketFailed(socket);
}

//------------------------------------------------------------------------------

fn_name TcpIoThread_InsertSocket = "TcpIoThread.InsertSocket";

bool TcpIoThread::InsertSocket(SysSocket* socket)
{
   Debug::ft(TcpIoThread_InsertSocket);

   if(socket->Protocol() != IpTcp) return false;

   auto interrupt = sockets_.Empty();
   auto sock = static_cast< SysTcpSocket* >(socket);

   if(sockets_.PushBack(sock))
   {
      //  If the thread had no sockets, it is sleeping forever
      //  and must be woken up to service its new socket.
      //
      auto flags = sock->InFlags();
      flags->set(PollRead);
      sock->Register();
      if(interrupt) Interrupt();
      return true;
   }

   ipPort_->PollArrayOverflow();
   return false;
}

//------------------------------------------------------------------------------

SysTcpSocket* TcpIoThread::Listener() const
{
   if(sockets_.Empty()) return nullptr;
   return sockets_.Front();
}

//------------------------------------------------------------------------------

fn_name TcpIoThread_ListenerError = "TcpIoThread.ListenerError";

SysTcpSocket* TcpIoThread::ListenerError(word errval) const
{
   Debug::ft(TcpIoThread_ListenerError);

   OutputLog("TCP I/O THREAD FAILURE", SocketNull, nullptr, errval);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name TcpIoThread_ListenerHasFailed = "TcpIoThread.ListenerHasFailed";

bool TcpIoThread::ListenerHasFailed(SysTcpSocket* listener) const
{
   Debug::ft(TcpIoThread_ListenerHasFailed);

   auto flags = listener->OutFlags();

   if(flags->test(PollInvalid) || flags->test(PollError) ||
      flags->test(PollHungUp))
   {
      OutputLog("TCP LISTENER ERROR", SocketFlags, listener);
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

fn_name TcpIoThread_OutputLog = "TcpIoThread.OutputLog";

void TcpIoThread::OutputLog(fixed_string expl, Error error,
   SysTcpSocket* socket, debug32_t errval) const
{
   Debug::ft(TcpIoThread_OutputLog);

   if((error == SocketError) && (socket->GetError() == 0)) return;

   auto log = Log::Create(expl);
   if(log == nullptr) return;
   *log << "port=" << port_;

   switch(error)
   {
   case SocketNull:
      *log << " errval=" << errval;
      break;
   case SocketError:
      *log << " errval=" << socket->GetError();
      break;
   case SocketFlags:
      *log << " flags=" << socket->OutFlags()->to_string();
      break;
   }

   *log << CRLF;
   Log::Spool(log);
}

//------------------------------------------------------------------------------

void TcpIoThread::Patch(sel_t selector, void* arguments)
{
   IoThread::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name TcpIoThread_PollSockets = "TcpIoThread.PollSockets";

word TcpIoThread::PollSockets()
{
   Debug::ft(TcpIoThread_PollSockets);

   if(listen_)
   {
      auto listener = Listener();
      listener->SetBlocking(true);
      listener->InFlags()->set(PollRead);
   }

   auto sockets = sockets_.Items();
   auto size = sockets_.Size();
   if(size == 0) return 0;
   auto ready = SysTcpSocket::Poll(sockets, size, TIMEOUT_IMMED);

   if(ready == 0)
   {
      ipPort_->RecvsInSequence(recvs_);

      EnterBlockingOperation(BlockedOnNetwork, TcpIoThread_Enter);
      {
         ready = SysTcpSocket::Poll(sockets, size, 2 * TIMEOUT_1_SEC);
      }
      ExitBlockingOperation(TcpIoThread_Enter);

      recvs_ = 0;
   }

   if(ready > 0) sockets_.Front()->TraceEvent(NwTrace::Poll, ready);
   return ready;
}

//------------------------------------------------------------------------------

fn_name TcpIoThread_ReleaseResources = "TcpIoThread.ReleaseResources";

void TcpIoThread::ReleaseResources()
{
   Debug::ft(TcpIoThread_ReleaseResources);

   for(auto size = sockets_.Size(); size > 0; size = sockets_.Size())
   {
      auto socket = sockets_[size - 1];
      if(socket == nullptr) continue;
      sockets_.Erase(size - 1);
      socket->Purge();
   }

   if(ipPort_ != nullptr) ipPort_->SetSocket(nullptr);
}

//------------------------------------------------------------------------------

fn_name TcpIoThread_ServiceSocket = "TcpIoThread.ServiceSocket";

void TcpIoThread::ServiceSocket()
{
   Debug::ft(TcpIoThread_ServiceSocket);

   auto socket = sockets_[curr_];
   if(socket == nullptr) return;

   //  Return if this socket has neither reported an event nor
   //  been released by the application.
   //
   if(socket->GetAppState() == SysTcpSocket::Released)
   {
      EraseSocket(curr_);
      return;
   }

   auto flags = socket->OutFlags();
   if(flags->none()) return;

   //  Before servicing the socket, pause if at risk of running
   //  locked too long.
   //
   ConditionalPause(90);
   --ready_;

   //  Erase the socket if it has disconnected or is no longer valid.
   //
   if(flags->test(PollHungUp) || flags->test(PollError) ||
      flags->test(PollInvalid))
   {
      EraseSocket(curr_);
      return;
   }

   //  If the socket is writeable, tell it to send queued messages.
   //
   if(flags->test(PollWrite)) socket->Dispatch();

   //  If the socket has an incoming message, read it.  On failure,
   //  release the socket.
   //
   if(!flags->test(PollRead)) return;

   ticks0_ = Clock::TicksNow();

   auto rcvd = socket->Recv(buffer_, SysSocket::MaxMsgSize);

   if(rcvd <= 0)
   {
      if(rcvd < 0)
      {
         //s Handle Recv() error.
         //
         OutputLog("TCP RECV ERROR", SocketError, socket);
      }

      EraseSocket(curr_);
      return;
   }

   ++recvs_;
   ipPort_->BytesRcvd(rcvd);

   //  Construct the address from which this message came (txAddr_) and
   //  the address where it arrived (rxAddr_).  The former, but not the
   //  latter, will contain a pointer to the socket.  Pass the message
   //  to the input handler.
   //
   if(!socket->RemAddr(txAddr_))
   {
      OutputLog("TCP PEER UNKNOWN", SocketError, socket);
      return;
   }

   rxAddr_ = SysIpL3Addr(host_, port_, IpTcp, socket);
   InvokeHandler(*ipPort_, buffer_, rcvd);
}

//------------------------------------------------------------------------------

fn_name TcpIoThread_Unblock = "TcpIoThread.Unblock";

void TcpIoThread::Unblock()
{
   Debug::ft(TcpIoThread_Unblock);

   //  Delete the thread's sockets.  If it is blocked on Recv, this should
   //  unblock it.
   //
   ReleaseResources();
}
}
