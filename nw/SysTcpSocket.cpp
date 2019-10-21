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
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "InputHandler.h"
#include "IpBuffer.h"
#include "IpPort.h"
#include "IpPortRegistry.h"
#include "NwLogs.h"
#include "NwTrace.h"
#include "Singleton.h"
#include "SysIpL3Addr.h"
#include "TcpIpService.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fn_name SysTcpSocket_ctor1 = "SysTcpSocket.ctor";

SysTcpSocket::SysTcpSocket(ipport_t port,
   const TcpIpService* service, AllocRc& rc) : SysSocket(port, service, rc),
   state_(Idle),
   disconnecting_(false),
   iotActive_(false),
   appState_(Initial),
   icMsg_(nullptr)
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
   disconnecting_(false),
   iotActive_(false),
   appState_(Initial),
   icMsg_(nullptr)
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

   //  Neither the application nor the I/O thread should be using the socket.
   //  If the socket has just received a message, the socket should not be
   //  deleted until the application has had a chance to process it.
   //
   if(iotActive_ || (appState_ == Acquired ) ||
      ((appState_ == Initial) && (state_ != Idle)))
   {
      Debug::SwLog(SysTcpSocket_dtor,
         "socket still in use", pack2(iotActive_, appState_));
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

fn_name SysTcpSocket_Acquire = "SysTcpSocket.Acquire";

void SysTcpSocket::Acquire()
{
   Debug::ft(SysTcpSocket_Acquire);

   TraceEvent(NwTrace::Acquire, state_);
   appState_ = Acquired;
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_AcquireIcMsg = "SysTcpSocket.AcquireIcMsg";

IpBuffer* SysTcpSocket::AcquireIcMsg()
{
   Debug::ft(SysTcpSocket_AcquireIcMsg);

   auto buff = icMsg_;
   icMsg_ = nullptr;
   return buff;
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

   if(icMsg_ != nullptr) icMsg_->Claim();
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_Deregister = "SysTcpSocket.Deregister";

bool SysTcpSocket::Deregister()
{
   Debug::ft(SysTcpSocket_Deregister);

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

fn_name SysTcpSocket_IsOpen = "SysTcpSocket.IsOpen";

bool SysTcpSocket::IsOpen() const
{
   Debug::ft(SysTcpSocket_IsOpen);

   return (!disconnecting_ && IsValid());
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_Purge = "SysTcpSocket.Purge";

void SysTcpSocket::Purge()
{
   Debug::ft(SysTcpSocket_Purge);

   TraceEvent(NwTrace::Purge, state_);
   iotActive_ = false;
   appState_ = Released;
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
      //  simple: ownership need not be transferred; trace tools can still
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
         //s Handle Connect() error.
         //
         OutputLog(NetworkSocketError, "Connect", &buff);
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
   auto port = Singleton< IpPortRegistry >::Instance()->GetPort(txport);
   byte_t* src = nullptr;
   auto size = buff.OutgoingBytes(src);
   auto dest = port->GetHandler()->HostToNetwork(buff, src, size);
   auto sent = Send(dest, size);

   if(sent <= 0)
   {
      if(sent == 0) return QueueBuff(&buff);

      //s Handle Send() error.
      //
      OutputLog(NetworkSocketError, "Send", &buff);
      return SendFailed;
   }

   port->BytesSent(size);
   return SendOk;
}

//------------------------------------------------------------------------------

fn_name SysTcpSocket_SetIcMsg = "SysTcpSocket.SetIcMsg";

void SysTcpSocket::SetIcMsg(IpBuffer* buff)
{
   Debug::ft(SysTcpSocket_SetIcMsg);

   if((icMsg_ != nullptr) && (icMsg_ != buff))
   {
      delete icMsg_;
   }

   icMsg_ = buff;
}
}
