//==============================================================================
//
//  MsgFactory.cpp
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
#include "MsgFactory.h"
#include "Algorithms.h"
#include "Debug.h"
#include "Message.h"
#include "MsgContext.h"
#include "NbTypes.h"
#include "SbTrace.h"
#include "SbTracer.h"
#include "Singleton.h"
#include "TimePoint.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name MsgFactory_ctor = "MsgFactory.ctor";

MsgFactory::MsgFactory(Id fid, ContextType type, ProtocolId prid,
   c_string name) : Factory(fid, type, prid, name)
{
   Debug::ft(MsgFactory_ctor);
}

//------------------------------------------------------------------------------

fn_name MsgFactory_dtor = "MsgFactory.dtor";

MsgFactory::~MsgFactory()
{
   Debug::ft(MsgFactory_dtor);
}

//------------------------------------------------------------------------------

fn_name MsgFactory_AllocContext = "MsgFactory.AllocContext";

Context* MsgFactory::AllocContext() const
{
   Debug::ft(MsgFactory_AllocContext);

   return new MsgContext(GetFaction());
}

//------------------------------------------------------------------------------

fn_name MsgFactory_CaptureMsg = "MsgFactory.CaptureMsg";

void MsgFactory::CaptureMsg(Context& ctx, const Message& msg, TransTrace* tt)
{
   Debug::ft(MsgFactory_CaptureMsg);

   auto warp = TimePoint::Now();
   auto sbt = Singleton< SbTracer >::Instance();

   if(!ctx.TraceOn())
   {
      ctx.SetTrace(sbt->MsgStatus(msg, MsgIncoming) == TraceIncluded);
   }

   //  We are running at I/O level, so the TransTrace record is passed as the
   //  TT argument.  It is NOT available from the other version of TraceOn().
   //
   if(ctx.TraceOn())
   {
      auto buff = Singleton< TraceBuffer >::Instance();

      if(buff->ToolIsOn(ContextTracer))
      {
         auto rec = new MsgTrace(MsgTrace::Creation, msg, Message::Internal);
         buff->Insert(rec);
      }
   }

   if(tt != nullptr)
   {
      tt->ResumeTime(warp);
      tt->EndOfTransaction();
   }
}

//------------------------------------------------------------------------------

void MsgFactory::Patch(sel_t selector, void* arguments)
{
   Factory::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name MsgFactory_ProcessIcMsg = "MsgFactory.ProcessIcMsg";

void MsgFactory::ProcessIcMsg(Message& msg) const
{
   Debug::ft(MsgFactory_ProcessIcMsg);

   //  This must be implemented by a subclass if required.
   //
   Context::Kill(strOver(this), pack2(msg.GetProtocol(), msg.GetSignal()));
}

//------------------------------------------------------------------------------

fn_name MsgFactory_ReceiveMsg = "MsgFactory.ReceiveMsg";

Factory::Rc MsgFactory::ReceiveMsg
   (Message& msg, bool atIoLevel, TransTrace* tt, Context*& ctx)
{
   Debug::ft(MsgFactory_ReceiveMsg);

   //  Create a message context and queue the message against it.  The
   //  context already exists, however, when a subclass is invoking us.
   //
   if(ctx == nullptr)
   {
      ctx = new MsgContext(GetFaction());
      if(ctx == nullptr) return CtxAllocFailed;
      IncrContexts();
   }

   if(!ctx->EnqMsg(msg))
   {
      delete ctx;
      ctx = nullptr;
      return ContextCorrupt;
   }

   if(atIoLevel && Debug::TraceOn()) CaptureMsg(*ctx, msg, tt);
   return InputOk;
}
}
