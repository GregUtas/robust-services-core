//==============================================================================
//
//  SsmFactory.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "SsmFactory.h"
#include "Debug.h"
#include "LocalAddress.h"
#include "Message.h"
#include "MsgHeader.h"
#include "SsmContext.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name SsmFactory_ctor = "SsmFactory.ctor";

SsmFactory::SsmFactory(Id fid, ProtocolId prid, const char* name) :
   PsmFactory(fid, MultiPort, prid, name)
{
   Debug::ft(SsmFactory_ctor);
}

//------------------------------------------------------------------------------

fn_name SsmFactory_dtor = "SsmFactory.dtor";

SsmFactory::~SsmFactory()
{
   Debug::ft(SsmFactory_dtor);
}

//------------------------------------------------------------------------------

fn_name SsmFactory_AllocContext = "SsmFactory.AllocContext";

Context* SsmFactory::AllocContext() const
{
   Debug::ft(SsmFactory_AllocContext);

   return new SsmContext(GetFaction());
}

//------------------------------------------------------------------------------

fn_name SsmFactory_AllocOgPsm = "SsmFactory.AllocOgPsm";

ProtocolSM* SsmFactory::AllocOgPsm(const Message& msg) const
{
   Debug::ft(SsmFactory_AllocOgPsm);

   //  This must be implemented by a subclass if required.
   //
   Debug::SwErr(SsmFactory_AllocOgPsm, Fid(), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name SsmFactory_AllocRoot = "SsmFactory.AllocRoot";

RootServiceSM* SsmFactory::AllocRoot(const Message& msg, ProtocolSM& psm) const
{
   Debug::ft(SsmFactory_AllocRoot);

   //  This must be implemented by a subclass if required.
   //
   Debug::SwErr(SsmFactory_AllocRoot, Fid(), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name SsmFactory_FindContext = "SsmFactory.FindContext";

SsmContext* SsmFactory::FindContext(const Message& msg) const
{
   Debug::ft(SsmFactory_FindContext);

   //  This must be implemented by a subclass if required.
   //
   Debug::SwErr(SsmFactory_FindContext, Fid(), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

void SsmFactory::Patch(sel_t selector, void* arguments)
{
   PsmFactory::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name SsmFactory_ReceiveMsg = "SsmFactory.ReceiveMsg";

Factory::Rc SsmFactory::ReceiveMsg
   (Message& msg, bool atIoLevel, TransTrace* tt, Context*& ctx)
{
   Debug::ft(SsmFactory_ReceiveMsg);

   auto header = msg.Header();

   //  Find the context if this is a join operation.  When the join flag is set,
   //  the initial flag must also be set, and the message must not be addressed
   //  to a port.
   //
   if(header->join && header->initial && (header->rxAddr.bid == NIL_ID))
   {
      ctx = FindContext(msg);
   }

   return PsmFactory::ReceiveMsg(msg, atIoLevel, tt, ctx);
}
}