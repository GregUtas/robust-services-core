//==============================================================================
//
//  SysTcpSocket.cpp
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
#include "SysTcpSocket.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "IpBuffer.h"
#include "IpPort.h"
#include "IpPortRegistry.h"
#include "NwTrace.h"
#include "Singleton.h"
#include "SysIpL3Addr.h"
#include "TcpIpService.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fn_name SysTcpSocket_ctor1 = "SysTcpSocket.ctor";

SysTcpSocket::SysTcpSocket(ipport_t port,
   const TcpIpService* service, AllocRc& rc) : SysSocket(port, service, rc),
   state_(Idle),
   iotActive_(false),
   appActive_(false)
{
   Debug::ft(SysTcpSocket_ctor1);

   ogMsgq_.Init(Pooled::LinkDiff());
   if(rc != AllocOk) return;
   if(SetBlocking(false)) return;
   rc = SetOptionError;
   Disconnect();
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_ctor2 = "SysTcpSocket.ctor(wrap)";

SysTcpSocket::SysTcpSocket(SysSocket_t socket) : SysSocket(socket),
   state_(Connected),
   iotActive_(false),
   appActive_(false)
{
   Debug::ft(SysTcpSocket_ctor2);

   ogMsgq_.Init(Pooled::LinkDiff());
   if(SetBlocking(false)) return;
   Disconnect();
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_dtor = "SysTcpSocket.dtor";

SysTcpSocket::~SysTcpSocket()
{
   Debug::ft(SysTcpSocket_dtor);

   //  Both the application and I/O thread should have released the socket.
   //
   if(appActive_ || iotActive_)
   {
      Debug::SwLog(SysTcpSocket_dtor, appActive_, iotActive_);
   }

   ogMsgq_.Purge();
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_Acquire = "SysTcpSocket.Acquire";

void SysTcpSocket::Acquire()
{
   Debug::ft(SysTcpSocket_Acquire);

   TraceEvent(NwTrace::Acquire, state_);
   appActive_ = true;
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_ClaimBlocks = "SysTcpSocket.ClaimBlocks";

void SysTcpSocket::ClaimBlocks()
{
   Debug::ft(SysTcpSocket_ClaimBlocks);

   for(auto buff = ogMsgq_.First(); buff != nullptr; ogMsgq_.Next(buff))
   {
      buff->Claim();
   }
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_Deregister = "SysTcpSocket.Deregister";

void SysTcpSocket::Deregister()
{
   Debug::ft(SysTcpSocket_Deregister);

   //  If the application has not relinquished the socket, close it
   //  without deleting its wrapper object.  It will be deleted when
   //  the application invokes Release().
   //
   TraceEvent(NwTrace::Deregister, state_);
   iotActive_ = false;
   if(!appActive_) delete this;
   Disconnect();
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_Dispatch = "SysTcpSocket.Dispatch";

void SysTcpSocket::Dispatch()
{
   Debug::ft(SysTcpSocket_Dispatch);

   //  The socket is writeable, so it must be connected.  Stop checking
   //  if it is writeable until it queues another message.
   //
   TraceEvent(NwTrace::Dispatch, state_);
   state_ = Connected;
   inFlags_.reset(PollWrite);

   //  Send our queued outgoing messages.  If a message cannot be sent,
   //  requeue it; this is an error unless the socket blocked.
   //
   for(auto buff = ogMsgq_.Deq(); buff != nullptr; buff = ogMsgq_.Deq())
   {
      auto rc = SendBuff(*buff);

      if(rc == SendOk)
      {
         delete buff;
         continue;
      }

      QueueBuff(buff, true);
      if(rc != SendBlocked) Deregister();
      return;
   }
}

//------------------------------------------------------------------------------

void SysTcpSocket::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   SysSocket::Display(stream, prefix, options);

   stream << prefix << "state     : " << state_ << CRLF;
   stream << prefix << "iotActive : " << iotActive_ << CRLF;
   stream << prefix << "appActive : " << appActive_ << CRLF;
   stream << prefix << "inFlags   : " << inFlags_.to_string() << CRLF;
   stream << prefix << "outFlags  : " << outFlags_.to_string() << CRLF;
   stream << prefix << "ogMsgq    : " << CRLF;
   ogMsgq_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_Purge = "SysTcpSocket.Purge";

void SysTcpSocket::Purge()
{
   Debug::ft(SysTcpSocket_Purge);

   TraceEvent(NwTrace::Purge, state_);
   iotActive_ = false;
   appActive_ = false;
   delete this;
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_QueueBuff = "SysTcpSocket.QueueBuff";

SysSocket::SendRc SysTcpSocket::QueueBuff(IpBuffer* buff, bool henq)
{
   Debug::ft(SysTcpSocket_QueueBuff);

   TraceEvent(NwTrace::Queue, state_);
   if(!IsOpen()) return SendFailed;

   if(!buff->IsQueued())
   {
      //  This buffer is being queued for the first time, so make a copy of it.
      //  The sender retains ownership of the original buffer to keep things
      //  simple.  Ownership need not be transferred; trace tools can still
      //  capture the buffer's contents; and the sender can free the buffer
      //  when it no longer requires access the outgoing message.
      //
      buff = new IpBuffer(*buff);
      if(buff == nullptr) return SendFailed;
   }

   if(henq)
      ogMsgq_.Henq(*buff);
   else
      ogMsgq_.Enq(*buff);

   buff->SetQueued();
   inFlags_.set(PollWrite);
   return SendQueued;
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_Register = "SysTcpSocket.Register";

void SysTcpSocket::Register()
{
   Debug::ft(SysTcpSocket_Register);

   TraceEvent(NwTrace::Register, state_);
   iotActive_ = true;
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_Release = "SysTcpSocket.Release";

void SysTcpSocket::Release()
{
   Debug::ft(SysTcpSocket_Release);

   //  If the socket is still in the I/O thread's socket array, close it
   //  without deleting its wrapper object.  When the PollInvalid event
   //  occurs, Deregister() will delete the wrapper.
   //
   TraceEvent(NwTrace::Release, state_);
   appActive_ = false;
   if(!iotActive_) delete this;
   Disconnect();
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_SendBuff = "SysTcpSocket.SendBuff";

SysSocket::SendRc SysTcpSocket::SendBuff(IpBuffer& buff)
{
   Debug::ft(SysTcpSocket_SendBuff);

   if(!IsOpen() || (state_ == Listening)) return SendFailed;

   auto txport = buff.TxAddr().GetPort();

   if(state_ == Idle)
   {
      //  This is an initial message.  The peer must accept the connection
      //  (the socket must become writeable) before the message can be sent.
      //
      auto rc = Connect(buff.RxAddr());
      TracePeer(NwTrace::Connect, txport, buff.RxAddr(), rc);

      if(rc == 0)
      {
         state_ = Connecting;
         inFlags_.set(PollWrite);
      }
      else
      {
         //s Handle Connect() error.
         //
         OutputLog("TCP CONNECT ERROR", &buff);
         return SendFailed;
      }
   }

   if(state_ == Connecting) return QueueBuff(&buff);

   //  Before sending the message, check that the socket is connected.
   //
   if(state_ != Connected)
   {
      Debug::SwLog(SysTcpSocket_SendBuff, txport, state_);
      return SendFailed;
   }

   //  Set the peer address in the buffer so that it will be correct if
   //  a log is generated.
   //
   SysIpL3Addr peer;

   if(!RemAddr(peer))
   {
      Debug::SwLog(SysTcpSocket_SendBuff, txport, Connected);
      Disconnect();
      return SendFailed;
   }

   buff.SetRxAddr(peer);

   //  If no bytes get sent, queue the buffer if the socket was blocked,
   //  else report an error.
   //
   auto port = Singleton< IpPortRegistry >::Instance()->GetPort(txport);
   byte_t* start = nullptr;
   auto size = buff.OutgoingBytes(start);
   HostToNetwork(start, size, port->GetService()->WordSize());
   auto sent = Send(start, size);

   if(sent <= 0)
   {
      if(sent == 0) return QueueBuff(&buff);

      //s Handle Send() error.
      //
      OutputLog("TCP SEND ERROR", &buff);
      return SendFailed;
   }

   port->BytesSent(size);
   return SendOk;
}
}
