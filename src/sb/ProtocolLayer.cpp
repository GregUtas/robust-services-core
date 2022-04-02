//==============================================================================
//
//  ProtocolLayer.cpp
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
#include "ProtocolLayer.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Context.h"
#include "Debug.h"
#include "SysTypes.h"

using namespace NetworkBase;
using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
ProtocolLayer::ProtocolLayer(Context* ctx) :
   ctx_(ctx),
   upper_(nullptr),
   lower_(nullptr)
{
   Debug::ft("ProtocolLayer.ctor(first)");

   if(ctx_ == nullptr) ctx_ = Context::RunningContext();
   Debug::Assert(ctx_ != nullptr);
}

//------------------------------------------------------------------------------

ProtocolLayer::ProtocolLayer(ProtocolLayer& adj, bool upper) :
   ctx_(nullptr),
   upper_(nullptr),
   lower_(nullptr)
{
   Debug::ft("ProtocolLayer.ctor(subseq)");
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
   Debug::ftnt(ProtocolLayer_dtor);

   //  There should be no upper layer.  Regardless, notify any adjacent
   //  layer that still exists and clear our reference to it.
   //
   if(upper_ != nullptr)
   {
      Debug::SwLog(ProtocolLayer_dtor, "unexpected upper layer", GetFactory());
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

void ProtocolLayer::AdjacentDeleted(bool upper)
{
   Debug::ft("ProtocolLayer.AdjacentDeleted");

   if(upper)
      upper_ = nullptr;
   else
      lower_ = nullptr;
}

//------------------------------------------------------------------------------

ProtocolLayer* ProtocolLayer::AllocLower(const Message* msg)
{
   Debug::ft("ProtocolLayer.AllocLower");

   return nullptr;
}

//------------------------------------------------------------------------------

ProtocolLayer* ProtocolLayer::AllocUpper(const Message& msg)
{
   Debug::ft("ProtocolLayer.AllocUpper");

   return nullptr;
}

//------------------------------------------------------------------------------

SysTcpSocket* ProtocolLayer::CreateAppSocket()
{
   Debug::ft("ProtocolLayer.CreateAppSocket");

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

void ProtocolLayer::EnsureLower(const Message* msg)
{
   Debug::ft("ProtocolLayer.EnsureLower");

   //  If the layer below doesn't exist, try to create it.
   //
   if(lower_ == nullptr)
   {
      lower_ = AllocLower(msg);

      if(lower_ == nullptr)
      {
         Context::Kill("failed to allocate lower layer", GetFactory());
         return;
      }

      lower_->upper_ = this;
   }
}

//------------------------------------------------------------------------------

MsgPort* ProtocolLayer::EnsurePort()
{
   Debug::ft("ProtocolLayer.EnsurePort");

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
   Debug::ftnt(ProtocolLayer_GetFactory);

   Debug::SwLog(ProtocolLayer_GetFactory, strOver(this), 0);
   return NIL_ID;
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

   Debug::SwLog(ProtocolLayer_Port, strOver(this), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

Event* ProtocolLayer::ReceiveMsg(Message& msg)
{
   Debug::ft("ProtocolLayer.ReceiveMsg");

   Context::Kill(strOver(this), GetFactory());
   return nullptr;
}

//------------------------------------------------------------------------------

RootServiceSM* ProtocolLayer::RootSsm() const
{
   Debug::ft("ProtocolLayer.RootSsm");

   if(ctx_ != nullptr) return ctx_->RootSsm();
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_Route = "ProtocolLayer.Route";

Message::Route ProtocolLayer::Route() const
{
   Debug::ft(ProtocolLayer_Route);

   Debug::SwLog(ProtocolLayer_Route, strOver(this), 0);
   return Message::External;
}

//------------------------------------------------------------------------------

bool ProtocolLayer::SendMsg(Message& msg)
{
   Debug::ft("ProtocolLayer.SendMsg");

   Context::Kill(strOver(this), GetFactory());
   return false;
}

//------------------------------------------------------------------------------

bool ProtocolLayer::SendToLower(Message& msg)
{
   Debug::ft("ProtocolLayer.SendToLower");

   //  Wrap the current message, pass it to the layer below, and flag it
   //  as handled unless it will be passed transparently.
   //
   EnsureLower(&msg);
   auto llmsg = lower_->WrapMsg(msg);
   if(&msg != llmsg) msg.Handled(false);
   return lower_->SendMsg(*llmsg);
}

//------------------------------------------------------------------------------

Event* ProtocolLayer::SendToUpper(Message& msg)
{
   Debug::ft("ProtocolLayer.SendToUpper");

   //  If the layer above doesn't exist, try to create it.
   //
   if(upper_ == nullptr)
   {
      upper_ = AllocUpper(msg);

      if(upper_ == nullptr)
      {
         Context::Kill("failed to allocate upper layer",
            pack2(msg.GetProtocol(), msg.GetSignal()));
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

Message* ProtocolLayer::UnwrapMsg(Message& msg)
{
   Debug::ft("ProtocolLayer.UnwrapMsg");

   //  A layer that is not at the bottom of a stack must implement
   //  this function.
   //
   Context::Kill(strOver(this), GetFactory());
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name ProtocolLayer_UppermostPsm = "ProtocolLayer.UppermostPsm";

ProtocolSM* ProtocolLayer::UppermostPsm() const
{
   Debug::ft(ProtocolLayer_UppermostPsm);

   Debug::SwLog(ProtocolLayer_UppermostPsm, strOver(this), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

Message* ProtocolLayer::WrapMsg(Message& msg)
{
   Debug::ft("ProtocolLayer.WrapMsg");

   //  A layer that is not at the bottom of a stack must implement
   //  this function.
   //
   Context::Kill(strOver(this), GetFactory());
   return nullptr;
}
}
