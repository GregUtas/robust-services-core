//==============================================================================
//
//  MsgFactory.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "MsgFactory.h"
#include "Clock.h"
#include "Debug.h"
#include "Message.h"
#include "MsgContext.h"
#include "NbTypes.h"
#include "SbTrace.h"
#include "SbTracer.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name MsgFactory_ctor = "MsgFactory.ctor";

MsgFactory::MsgFactory(Id fid, ContextType type, ProtocolId prid,
   const char* name) : Factory(fid, type, prid, name)
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

   auto warp = Clock::TicksNow();
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
      if(Singleton< TraceBuffer >::Instance()->ToolIsOn(ContextTracer))
      {
         new MsgTrace(MsgTrace::Creation, msg, Message::Internal);
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
   Context::Kill(MsgFactory_ProcessIcMsg, msg.GetProtocol(), msg.GetSignal());
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