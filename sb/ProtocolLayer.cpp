//==============================================================================
//
//  ProtocolLayer.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "ProtocolLayer.h"
#include <ostream>
#include <string>
#include "Context.h"
#include "Debug.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name ProtocolLayer_ctor1 = "ProtocolLayer.ctor(first)";

ProtocolLayer::ProtocolLayer(Context* ctx) :
   ctx_(ctx),
   upper_(nullptr),
   lower_(nullptr)
{
   Debug::ft(ProtocolLayer_ctor1);

   if(ctx_ == nullptr) ctx_ = Context::RunningContext();
   Debug::Assert(ctx_ != nullptr);
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_ctor2 = "ProtocolLayer.ctor(subseq)";

ProtocolLayer::ProtocolLayer(ProtocolLayer& adj, bool upper) :
   ctx_(nullptr),
   upper_(nullptr),
   lower_(nullptr)
{
   Debug::ft(ProtocolLayer_ctor2);
   Debug::Assert(&adj != nullptr);

   ctx_ = adj.GetContext();

   if(upper)
      upper_ = &adj;
   else
      lower_ = &adj;
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_dtor = "ProtocolLayer.dtor";

ProtocolLayer::~ProtocolLayer()
{
   Debug::ft(ProtocolLayer_dtor);

   //  There should be no upper layer.  Regardless, notify any adjacent
   //  layer that still exists and clear our reference to it.
   //
   if(upper_ != nullptr)
   {
      Debug::SwErr(ProtocolLayer_dtor, 0, 0);
      upper_->AdjacentDeleted(false);
      upper_ = nullptr;
   }

   if(lower_ != nullptr)
   {
      lower_->AdjacentDeleted(true);
      upper_ = nullptr;
   }
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_AdjacentDeleted = "ProtocolLayer.AdjacentDeleted";

void ProtocolLayer::AdjacentDeleted(bool upper)
{
   Debug::ft(ProtocolLayer_AdjacentDeleted);

   if(upper)
      upper_ = nullptr;
   else
      lower_ = nullptr;
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_AllocLower = "ProtocolLayer.AllocLower";

ProtocolLayer* ProtocolLayer::AllocLower(const Message* msg)
{
   Debug::ft(ProtocolLayer_AllocLower);

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_AllocUpper = "ProtocolLayer.AllocUpper";

ProtocolLayer* ProtocolLayer::AllocUpper(const Message& msg)
{
   Debug::ft(ProtocolLayer_AllocUpper);

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_CreateAppSocket = "ProtocolLayer.CreateAppSocket";

SysSocket* ProtocolLayer::CreateAppSocket()
{
   Debug::ft(ProtocolLayer_CreateAppSocket);

   return nullptr;
}

//------------------------------------------------------------------------------

void ProtocolLayer::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Pooled::Display(stream, prefix, options);

   stream << prefix << "ctx   : " << ctx_ << CRLF;
   stream << prefix << "upper : " << upper_ << CRLF;
   stream << prefix << "lower : " << lower_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_DropPeer = "ProtocolLayer.DropPeer";

bool ProtocolLayer::DropPeer(const GlobalAddress& peerPrevRemAddr)
{
   Debug::ft(ProtocolLayer_DropPeer);

   //  This is a pure virtual function.
   //
   Context::Kill(ProtocolLayer_DropPeer, 0, 0);
   return false;
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_EnsureLower = "ProtocolLayer.EnsureLower";

void ProtocolLayer::EnsureLower(const Message* msg)
{
   Debug::ft(ProtocolLayer_EnsureLower);

   //  If the layer below doesn't exist, try to create it.
   //
   if(lower_ == nullptr)
   {
      lower_ = AllocLower(msg);

      if(lower_ == nullptr)
      {
         Context::Kill(ProtocolLayer_EnsureLower, GetFactory(), 0);
         return;
      }

      lower_->upper_ = this;
   }
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_EnsurePort = "ProtocolLayer.EnsurePort";

MsgPort* ProtocolLayer::EnsurePort()
{
   Debug::ft(ProtocolLayer_EnsurePort);

   //  Create the stack down to the port.
   //
   for(auto upper = this; upper->Port() == nullptr; upper = upper->lower_)
   {
      upper->EnsureLower(nullptr);
   }

   return Port();
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_GetFactory = "ProtocolLayer.GetFactory";

FactoryId ProtocolLayer::GetFactory() const
{
   Debug::ft(ProtocolLayer_GetFactory);

   //  This is a pure virtual function.
   //
   Context::Kill(ProtocolLayer_DropPeer, 0, 0);
   return NIL_ID;
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_JoinPeer = "ProtocolLayer.JoinPeer";

ProtocolLayer* ProtocolLayer::JoinPeer
   (const LocalAddress& peer, GlobalAddress& peerPrevRemAddr)
{
   Debug::ft(ProtocolLayer_JoinPeer);

   //  This is a pure virtual function.
   //
   Context::Kill(ProtocolLayer_JoinPeer, 0, 0);
   return nullptr;
}

//------------------------------------------------------------------------------

void ProtocolLayer::Patch(sel_t selector, void* arguments)
{
   Pooled::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_Port = "ProtocolLayer.Port";

MsgPort* ProtocolLayer::Port() const
{
   Debug::ft(ProtocolLayer_Port);

   //  This is a pure virtual function.
   //
   Context::Kill(ProtocolLayer_Port, 0, 0);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_ReceiveMsg = "ProtocolLayer.ReceiveMsg";

Event* ProtocolLayer::ReceiveMsg(Message& msg)
{
   Debug::ft(ProtocolLayer_ReceiveMsg);

   //  This is a pure virtual function.
   //
   Context::Kill(ProtocolLayer_ReceiveMsg, 0, 0);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_RootSsm = "ProtocolLayer.RootSsm";

RootServiceSM* ProtocolLayer::RootSsm() const
{
   Debug::ft(ProtocolLayer_RootSsm);

   if(ctx_ != nullptr) return ctx_->RootSsm();
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_Route = "ProtocolLayer.Route";

Message::Route ProtocolLayer::Route() const
{
   Debug::ft(ProtocolLayer_Route);

   //  This is a pure virtual function.
   //
   Context::Kill(ProtocolLayer_Route, 0, 0);
   return Message::External;
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_SendMsg = "ProtocolLayer.SendMsg";

bool ProtocolLayer::SendMsg(Message& msg)
{
   Debug::ft(ProtocolLayer_SendMsg);

   //  This is a pure virtual function.
   //
   Context::Kill(ProtocolLayer_SendMsg, 0, 0);
   return false;
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_SendToLower = "ProtocolLayer.SendToLower";

bool ProtocolLayer::SendToLower(Message& msg)
{
   Debug::ft(ProtocolLayer_SendToLower);

   //  Wrap the current message, pass it to the layer below, and flag it
   //  as handled unless it will be passed transparently.
   //
   EnsureLower(&msg);
   auto llmsg = lower_->WrapMsg(msg);
   if(&msg != llmsg) msg.Handled(false);
   return lower_->SendMsg(*llmsg);
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_SendToUpper = "ProtocolLayer.SendToUpper";

Event* ProtocolLayer::SendToUpper(Message& msg)
{
   Debug::ft(ProtocolLayer_SendToUpper);

   //  If the layer above doesn't exist, try to create it.
   //
   if(upper_ == nullptr)
   {
      upper_ = AllocUpper(msg);

      if(upper_ == nullptr)
      {
         Context::Kill
            (ProtocolLayer_SendToUpper, msg.GetProtocol(), msg.GetSignal());
         return nullptr;
      }

      upper_->lower_ = this;
   }

   //  Unwrap MSG and flag it as handled unless it will be passed transparently.
   //  If no message is returned, end the transaction, else receive it.
   //
   auto ulmsg = upper_->UnwrapMsg(msg);
   if(&msg != ulmsg) msg.Handled(false);
   if(ulmsg == nullptr) return nullptr;
   Context::SetContextMsg(ulmsg);
   return upper_->ReceiveMsg(*ulmsg);
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_UnwrapMsg = "ProtocolLayer.UnwrapMsg";

Message* ProtocolLayer::UnwrapMsg(Message& msg)
{
   Debug::ft(ProtocolLayer_UnwrapMsg);

   //  A layer that is not at the bottom of a stack must implement
   //  this function.
   //
   Context::Kill(ProtocolLayer_UnwrapMsg, 0, 0);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_UppermostPsm = "ProtocolLayer.UppermostPsm";

ProtocolSM* ProtocolLayer::UppermostPsm() const
{
   Debug::ft(ProtocolLayer_UppermostPsm);

   //  This is a pure virtual function.
   //
   Context::Kill(ProtocolLayer_UppermostPsm, 0, 0);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_WrapMsg = "ProtocolLayer.WrapMsg";

Message* ProtocolLayer::WrapMsg(Message& msg)
{
   Debug::ft(ProtocolLayer_WrapMsg);

   //  A layer that is not at the bottom of a stack must implement
   //  this function.
   //
   Context::Kill(ProtocolLayer_WrapMsg, 0, 0);
   return nullptr;
}
}
