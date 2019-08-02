//==============================================================================
//
//  MsgPort.cpp
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
#include "MsgPort.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Clock.h"
#include "Context.h"
#include "Debug.h"
#include "FactoryRegistry.h"
#include "Formatters.h"
#include "IpPortRegistry.h"
#include "Message.h"
#include "MsgHeader.h"
#include "ObjectPoolRegistry.h"
#include "ProtocolSM.h"
#include "PsmFactory.h"
#include "SbPools.h"
#include "SbTrace.h"
#include "Singleton.h"
#include "SysTcpSocket.h"
#include "SysTypes.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using namespace NetworkBase;
using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
const PooledObjectId MsgPort::MaxId = 0x40000;  // 2^18 = 256K

//------------------------------------------------------------------------------

fn_name MsgPort_ctor1 = "MsgPort.ctor(i/c)";

MsgPort::MsgPort(const Message& msg, Context& ctx) : ProtocolLayer(&ctx),
   locAddr_(msg.GetReceiver()),
   remAddr_(msg.GetSender()),
   msgRcvd_(true),
   msgSent_(false)
{
   Debug::ft(MsgPort_ctor1);

   Initialize(&msg);
}

//------------------------------------------------------------------------------

fn_name MsgPort_ctor2 = "MsgPort.ctor(o/g)";

MsgPort::MsgPort(ProtocolLayer& upper) : ProtocolLayer(upper, true),
   msgRcvd_(false),
   msgSent_(false)
{
   Debug::ft(MsgPort_ctor2);

   //  We were created on behalf of UPPER's factory.
   //
   locAddr_.sbAddr_.fid = static_cast< ProtocolSM& >(upper).GetFactory();
   Initialize(nullptr);
}

//------------------------------------------------------------------------------

fn_name MsgPort_dtor = "MsgPort.dtor";

MsgPort::~MsgPort()
{
   Debug::ft(MsgPort_dtor);

   //  Record the port's deletion if this context is traced.
   //
   TransTrace* trans = nullptr;

   if(Context::RunningContextTraced(trans))
   {
      auto warp = Clock::TicksNow();

      if(Singleton< TraceBuffer >::Instance()->ToolIsOn(ContextTracer))
      {
         new PortTrace(PortTrace::Deletion, *this);
      }

      if(trans != nullptr) trans->ResumeTime(warp);
   }

   remAddr_.ReleaseSocket();

   auto ctx = GetContext();
   if(ctx != nullptr) ctx->ExqPort(*this);
}

//------------------------------------------------------------------------------

fn_name MsgPort_AdjacentDeleted = "MsgPort.AdjacentDeleted";

void MsgPort::AdjacentDeleted(bool upper)
{
   Debug::ft(MsgPort_AdjacentDeleted);

   ProtocolLayer::AdjacentDeleted(upper);
   delete this;
}

//------------------------------------------------------------------------------

fn_name MsgPort_AllocUpper = "MsgPort.AllocUpper";

ProtocolLayer* MsgPort::AllocUpper(const Message& msg)
{
   Debug::ft(MsgPort_AllocUpper);

   //  Create the first PSM for the incoming message.  This is done by
   //  delegating to the receiving factory.
   //
   auto hdr = msg.Header();
   auto fid = hdr->rxAddr.fid;
   auto fac = Singleton < FactoryRegistry >::Instance()->GetFactory(fid);

   if(fac == nullptr)
   {
      Debug::SwLog(MsgPort_AllocUpper, pack2(hdr->protocol, hdr->signal), 0);
      return nullptr;
   }

   if(fac->GetType() == SingleMsg)
   {
      Debug::SwLog(MsgPort_AllocUpper, pack2(hdr->protocol, hdr->signal), fid);
      return nullptr;
   }

   return static_cast< PsmFactory* >(fac)->AllocIcPsm(msg, *this);
}

//------------------------------------------------------------------------------

fn_name MsgPort_Cleanup = "MsgPort.Cleanup";

void MsgPort::Cleanup()
{
   Debug::ft(MsgPort_Cleanup);

   remAddr_.ReleaseSocket();
   ProtocolLayer::Cleanup();
}

//------------------------------------------------------------------------------

void MsgPort::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   ProtocolLayer::Display(stream, prefix, options);

   stream << prefix << "locAddr : " << CRLF;
   locAddr_.Display(stream, prefix + spaces(2), options);

   stream << prefix << "remAddr : " << CRLF;
   remAddr_.Display(stream, prefix + spaces(2), options);

   stream << prefix << "msgRcvd : " << msgRcvd_ << CRLF;
   stream << prefix << "msgSent : " << msgSent_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name MsgPort_DropPeer = "MsgPort.DropPeer";

bool MsgPort::DropPeer(const GlobalAddress& peerPrevRemAddr)
{
   Debug::ft(MsgPort_DropPeer);

   //  Report failure if this PSM's peer cannot be found.
   //
   auto peerPort = Find(locAddr_.sbAddr_);
   if(peerPort == nullptr) return false;

   //  Reset the peer to communicate with its previous address,
   //  and modify this port so that it has no peer.
   //
   peerPort->remAddr_ = peerPrevRemAddr;
   remAddr_ = GlobalAddress();
   return true;
}

//------------------------------------------------------------------------------

fn_name MsgPort_Find = "MsgPort.Find";

MsgPort* MsgPort::Find(const LocalAddress& locAddr)
{
   Debug::ft(MsgPort_Find);

   auto pool = Singleton< MsgPortPool >::Instance();
   if(locAddr.pid != pool->Pid()) return nullptr;

   auto port = static_cast< MsgPort* >(pool->BidToObj(locAddr.bid));

   if(port != nullptr)
   {
      if(port->locAddr_.sbAddr_ == locAddr) return port;
   }

   return port;
}

//------------------------------------------------------------------------------

fn_name MsgPort_FindPeer = "MsgPort.FindPeer";

MsgPort* MsgPort::FindPeer(const GlobalAddress& remAddr)
{
   Debug::ft(MsgPort_FindPeer);

   return Singleton< MsgPortPool >::Instance()->FindPeerPort(remAddr);
}

//------------------------------------------------------------------------------

fn_name MsgPort_Initialize = "MsgPort.Initialize";

void MsgPort::Initialize(const Message* msg)
{
   Debug::ft(MsgPort_Initialize);

   //  Construct our address and add ourselves to our context's port queue.
   //
   locAddr_.sbAddr_.seq = ObjectPool::ObjSeq(this);
   locAddr_.sbAddr_.pid = ObjectPool::ObjPid(this);

   auto reg = Singleton< ObjectPoolRegistry >::Instance();
   auto pool = reg->Pool(locAddr_.sbAddr_.pid);
   locAddr_.sbAddr_.bid = pool->ObjBid(this, true);

   auto ctx = GetContext();
   ctx->EnqPort(*this);

   //  If we received the initial message, acquire the associated TCP socket
   //  (if any) and update our peer with our address (if intraprocessor).
   //
   if(msgRcvd_)
   {
      auto socket = remAddr_.GetSocket();
      if(socket != nullptr) socket->Acquire();
      UpdatePeer();
   }

   //  Inform our factory about our allocation.
   //
   auto fid = locAddr_.sbAddr_.fid;
   auto fac = Singleton< FactoryRegistry >::Instance()->GetFactory(fid);

   if(fac != nullptr)
      static_cast< PsmFactory* >(fac)->PortAllocated(*this, msg);
   else
      Debug::SwLog(MsgPort_Initialize, fid, 0);

   //  Record the port's creation if this context is traced.
   //
   TransTrace* trans = nullptr;

   if(ctx->TraceOn(trans))
   {
      auto warp = Clock::TicksNow();

      if(Singleton< TraceBuffer >::Instance()->ToolIsOn(ContextTracer))
      {
         new PortTrace(PortTrace::Creation, *this);
      }

      if(trans != nullptr) trans->ResumeTime(warp);
   }
}

//------------------------------------------------------------------------------

fn_name MsgPort_JoinPeer = "MsgPort.JoinPeer";

ProtocolLayer* MsgPort::JoinPeer
   (const LocalAddress& peer, GlobalAddress& peerPrevRemAddr)
{
   Debug::ft(MsgPort_JoinPeer);

   //  Find the peer port.
   //
   auto peerPort = Find(peer);
   if(peerPort == nullptr) return nullptr;

   //  Save the address with which PEER is currently communicating and
   //  then configure this PSM and PEER to communicate.  The PSMs know
   //  each other's addresses, so mark them as having sent messages to
   //  one another.
   //
   peerPrevRemAddr = peerPort->remAddr_;
   locAddr_ = GlobalAddress(peerPort->locAddr_, locAddr_.sbAddr_);
   remAddr_ = peerPort->locAddr_;
   peerPort->remAddr_ = locAddr_;

   msgRcvd_ = true;
   msgSent_ = true;
   peerPort->msgRcvd_ = true;
   peerPort->msgSent_ = true;

   return peerPort;
}

//------------------------------------------------------------------------------

fn_name MsgPort_new = "MsgPort.operator new";

void* MsgPort::operator new(size_t size)
{
   Debug::ft(MsgPort_new);

   return Singleton< MsgPortPool >::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

void MsgPort::Patch(sel_t selector, void* arguments)
{
   ProtocolLayer::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name MsgPort_Port = "MsgPort.Port";

MsgPort* MsgPort::Port() const
{
   Debug::ft(MsgPort_Port);

   return const_cast< MsgPort* >(this);
}

//------------------------------------------------------------------------------

fn_name MsgPort_ReceiveMsg = "MsgPort.ReceiveMsg";

Event* MsgPort::ReceiveMsg(Message& msg)
{
   Debug::ft(MsgPort_ReceiveMsg);

   //  If this port was created to receive a message, msgRcvd_ was set by the
   //  constructor.  If this port was created to send a message, then msgRcvd_
   //  was set when its peer invoked UpdatePeer.  Therefore, msgRcvd_ can only
   //  be false if this port was created and sent an interprocessor message or
   //  never sent a message at all.  If this is the case, it has now received
   //  a message unless it sent this message to itself.
   //
   if(!msgRcvd_ && !msg.Header()->self)
   {
      msgRcvd_ = true;
      remAddr_ = msg.GetSender();

      if(!msgSent_)
      {
         locAddr_.sbAddr_.fid = msg.Header()->rxAddr.fid;
         locAddr_ = GlobalAddress(msg.RxIpAddr(), locAddr_.sbAddr_);
      }
   }

   //  Pass the message up the protocol stack.
   //
   return SendToUpper(msg);
}

//------------------------------------------------------------------------------

fn_name MsgPort_Route = "MsgPort.Route";

Message::Route MsgPort::Route() const
{
   Debug::ft(MsgPort_Route);

   auto upper = Upper();
   if(upper != nullptr) return upper->Route();

   Context::Kill(MsgPort_Route, 0, 0);
   return Message::External;
}

//------------------------------------------------------------------------------

fn_name MsgPort_SendMsg = "MsgPort.SendMsg";

bool MsgPort::SendMsg(Message& msg)
{
   Debug::ft(MsgPort_SendMsg);

   if(!msgRcvd_ && !msgSent_)
   {
      //  This is the first message.  Set our IP address from the message
      //  but provide our local address, which the PSM did not know.  Set
      //  our peer's address from the message.
      //
      auto& txaddr = msg.TxIpAddr();
      locAddr_ = GlobalAddress(txaddr, locAddr_.sbAddr_);
      remAddr_ = msg.GetReceiver();
      msg.Header()->txAddr = locAddr_.sbAddr_;

      //  If the protocol stack does not have a socket, give the lowermost
      //  PSM the opportunity to create one.
      //
      if(remAddr_.GetSocket() == nullptr)
      {
         auto socket = Upper()->CreateAppSocket();

         if(socket != nullptr)
         {
            remAddr_.SetSocket(socket);
            msg.SetReceiver(remAddr_);
            socket->Acquire();
         }
      }
   }
   else
   {
      //  This is a subsequent message, so set both addresses.
      //
      msg.SetSender(locAddr_);
      msg.SetReceiver(remAddr_);
   }

   auto self = msg.Header()->self;
   auto sent = msg.Send(Route());

   //  If the message was sent and the port had not sent a message, now it
   //  has, unless it sent the message to itself.
   //
   if(!msgSent_ && sent) msgSent_ = !self;
   return sent;
}

//------------------------------------------------------------------------------

fn_name MsgPort_UpdatePeer = "MsgPort.UpdatePeer";

void MsgPort::UpdatePeer() const
{
   Debug::ft(MsgPort_UpdatePeer);

   //  When a message arrives and creates a port, immediately update the peer
   //  (the port that sent the message) so that it knows the address of this
   //  port, which was just created to receive the message.  This allows the
   //  peer to send another message before it receives a response.  This can
   //  only be done if the peer is a port (not a factory) on the same processor.
   //
   if(remAddr_.sbAddr_.bid == NIL_ID) return;

   auto reg = Singleton< IpPortRegistry >::Instance();
   if(!reg->CanBypassStack(locAddr_, remAddr_)) return;

   auto peer = Find(remAddr_.sbAddr_);

   if(peer != nullptr)
   {
      peer->remAddr_ = locAddr_;
      peer->msgRcvd_ = true;
   }
}

//------------------------------------------------------------------------------

fn_name MsgPort_UppermostPsm = "MsgPort.UppermostPsm";

ProtocolSM* MsgPort::UppermostPsm() const
{
   Debug::ft(MsgPort_UppermostPsm);

   auto upper = Upper();
   if(upper == nullptr) return nullptr;
   return upper->UppermostPsm();
}

//------------------------------------------------------------------------------

fn_name MsgPort_WrapMsg = "MsgPort.WrapMsg";

Message* MsgPort::WrapMsg(Message& msg)
{
   Debug::ft(MsgPort_WrapMsg);

   return &msg;
}
}
