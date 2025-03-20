//==============================================================================
//
//  SysTcpSocket.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "SysTcpSocket.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "InputHandler.h"
#include "IpBuffer.h"
#include "IpPort.h"
#include "IpPortRegistry.h"
#include "NwLogs.h"
#include "NwTrace.h"
#include "Restart.h"
#include "Singleton.h"
#include "SysIpL3Addr.h"
#include "TcpIpService.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
SysTcpSocket::SysTcpSocket(ipport_t port,
   const TcpIpService* service, AllocRc& rc) : SysSocket(port, service, rc),
   state_(Idle),
   disconnecting_(false),
   iotActive_(false),
   appState_(Initial),
   icMsg_(nullptr)
{
   Debug::ft("SysTcpSocket.ctor");

   ogMsgq_.Init(Pooled::LinkDiff());
   if(rc != AllocOk) return;
   if(SetBlocking(false) && SetClose(true)) return;
   rc = SetOptionError;
   Disconnect();
}

//------------------------------------------------------------------------------

SysTcpSocket::SysTcpSocket(SysSocket_t socket, ipport_t port) :
   SysSocket(socket, port),
   state_(Connected),
   disconnecting_(false),
   iotActive_(false),
   appState_(Initial),
   icMsg_(nullptr)
{
   Debug::ft("SysTcpSocket.ctor(wrap)");

   ogMsgq_.Init(Pooled::LinkDiff());
   if(SetBlocking(false) && SetClose(true)) return;
   Disconnect();
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_dtor = "SysTcpSocket.dtor";

SysTcpSocket::~SysTcpSocket()
{
   Debug::ftnt(SysTcpSocket_dtor);

   //  Neither the application nor the I/O thread should be using the socket.
   //  If the socket has just received a message, the socket should not be
   //  deleted until the application has had a chance to process it.  During
   //  a restart, however, the socket is deleted to unblock TcpIoThread so
   //  that it can exit.
   //
   if(iotActive_ || (appState_ == Acquired) ||
      ((appState_ == Initial) && (state_ != Idle)))
   {
      if(Restart::GetStage() == Running)
      {
         Debug::SwLog(SysTcpSocket_dtor,
            "socket still in use", pack2(iotActive_, appState_));
      }
   }

   if(icMsg_ != nullptr)
   {
      delete icMsg_;
      icMsg_ = nullptr;
   }

   ogMsgq_.Purge();
   Close(disconnecting_);
}

//------------------------------------------------------------------------------

void SysTcpSocket::Acquire()
{
   Debug::ft("SysTcpSocket.Acquire");

   TraceEvent(NwTrace::Acquire, state_);
   appState_ = Acquired;
}

//------------------------------------------------------------------------------

IpBuffer* SysTcpSocket::AcquireIcMsg()
{
   Debug::ft("SysTcpSocket.AcquireIcMsg");

   auto buff = icMsg_;
   icMsg_ = nullptr;
   return buff;
}

//------------------------------------------------------------------------------

void SysTcpSocket::ClaimBlocks()
{
   Debug::ft("SysTcpSocket.ClaimBlocks");

   for(auto buff = ogMsgq_.First(); buff != nullptr; ogMsgq_.Next(buff))
   {
      buff->ClaimBlocks();
   }

   if(icMsg_ != nullptr) icMsg_->ClaimBlocks();
}

//------------------------------------------------------------------------------

bool SysTcpSocket::Deregister()
{
   Debug::ft("SysTcpSocket.Deregister");

   //  If the application has not released the socket, close it without
   //  deleting its wrapper object.  It will be deleted when the
   //  application invokes Release().
   //
   TraceEvent(NwTrace::Deregister, state_);
   iotActive_ = false;

   if((appState_ == Released) || ((appState_ == Initial) && (state_ == Idle)))
   {
      delete this;
      return true;
   }

   Disconnect();
   return false;
}

//------------------------------------------------------------------------------

void SysTcpSocket::Dispatch()
{
   Debug::ft("SysTcpSocket.Dispatch");

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

   stream << prefix << "state         : " << state_ << CRLF;
   stream << prefix << "disconnecting : " << disconnecting_ << CRLF;
   stream << prefix << "iotActive     : " << iotActive_ << CRLF;
   stream << prefix << "appState      : " << int(appState_) << CRLF;
   stream << prefix << "inFlags       : " << inFlags_.to_string() << CRLF;
   stream << prefix << "outFlags      : " << outFlags_.to_string() << CRLF;
   stream << prefix << "icMsg         : " << icMsg_ << CRLF;
   stream << prefix << "ogMsgq        : " << CRLF;
   ogMsgq_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

bool SysTcpSocket::IsOpen() const
{
   Debug::ft("SysTcpSocket.IsOpen");

   return (!disconnecting_ && IsValid());
}

//------------------------------------------------------------------------------

void SysTcpSocket::Purge()
{
   Debug::ft("SysTcpSocket.Purge");

   TraceEvent(NwTrace::Purge, state_);
   iotActive_ = false;
   appState_ = Released;
   delete this;
}

//------------------------------------------------------------------------------

SysSocket::SendRc SysTcpSocket::QueueBuff(IpBuffer* buff, bool henq)
{
   Debug::ft("SysTcpSocket.QueueBuff");

   TraceEvent(NwTrace::Queue, state_);
   if(!IsOpen()) return SendFailed;

   if(!buff->IsQueued())
   {
      //  This buffer is being queued for the first time, so make a copy of it.
      //  The sender retains ownership of the original buffer to keep things
      //  simple: ownership need not be transferred; trace tools can still
      //  capture the buffer's contents; and the sender can free the buffer
      //  when it no longer requires access the outgoing message.
      //
      buff = buff->Clone();
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

void SysTcpSocket::Register()
{
   Debug::ft("SysTcpSocket.Register");

   TraceEvent(NwTrace::Register, state_);
   iotActive_ = true;
}

//------------------------------------------------------------------------------

void SysTcpSocket::Release()
{
   Debug::ft("SysTcpSocket.Release");

   //  If the socket is still in the I/O thread's socket array, close it
   //  without deleting its wrapper object.  When the PollInvalid event
   //  occurs, Deregister() will delete the wrapper.
   //
   TraceEvent(NwTrace::Release, state_);
   appState_ = Released;

   if(!iotActive_)
      delete this;
   else
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
         OutputLog(NetworkSocketError, "connect", &buff);
         return SendFailed;
      }
   }

   if(state_ == Connecting) return QueueBuff(&buff);

   //  Before sending the message, check that the socket is connected.
   //
   if(state_ != Connected)
   {
      Debug::SwLog(SysTcpSocket_SendBuff,
         "invalid state", pack2(txport, state_));
      return SendFailed;
   }

   //  Set the peer address in the buffer so that it will be correct if
   //  a log is generated.
   //
   SysIpL3Addr peer;

   if(!RemAddr(peer))
   {
      Debug::SwLog(SysTcpSocket_SendBuff,
         "invalid state", pack2(txport, Connected));
      Disconnect();
      return SendFailed;
   }

   buff.SetRxAddr(peer);

   //  If no bytes get sent, queue the buffer if the socket was blocked,
   //  else report an error.
   //
   auto port = Singleton<IpPortRegistry>::Instance()->GetPort(txport);
   byte_t* src = nullptr;
   auto size = buff.OutgoingBytes(src);
   auto data = port->GetHandler()->HostToNetwork(buff, src, size);
   auto sent = Send(data, size);

   if(sent == 0)
   {
      return QueueBuff(&buff);
   }
   else if(sent == -1)
   {
      OutputLog(NetworkSocketError, "send", &buff);
      return SendFailed;
   }

   port->BytesSent(size);
   return SendOk;
}

//------------------------------------------------------------------------------

void SysTcpSocket::SetIcMsg(IpBuffer* buff)
{
   Debug::ft("SysTcpSocket.SetIcMsg");

   if((icMsg_ != nullptr) && (icMsg_ != buff))
   {
      delete icMsg_;
   }

   icMsg_ = buff;
}
}
