//==============================================================================
//
//  SsmFactory.cpp
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
#include "SsmFactory.h"
#include "Debug.h"
#include "LocalAddress.h"
#include "Message.h"
#include "MsgHeader.h"
#include "SsmContext.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name SsmFactory_ctor = "SsmFactory.ctor";

SsmFactory::SsmFactory(Id fid, ProtocolId prid, c_string name) :
   PsmFactory(fid, MultiPort, prid, name)
{
   Debug::ft(SsmFactory_ctor);
}

//------------------------------------------------------------------------------

fn_name SsmFactory_dtor = "SsmFactory.dtor";

SsmFactory::~SsmFactory()
{
   Debug::ftnt(SsmFactory_dtor);
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
   Debug::SwLog(SsmFactory_AllocOgPsm, Fid(), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name SsmFactory_AllocRoot = "SsmFactory.AllocRoot";

RootServiceSM* SsmFactory::AllocRoot(const Message& msg, ProtocolSM& psm) const
{
   Debug::ft(SsmFactory_AllocRoot);

   //  This must be implemented by a subclass if required.
   //
   Debug::SwLog(SsmFactory_AllocRoot, Fid(), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name SsmFactory_FindContext = "SsmFactory.FindContext";

SsmContext* SsmFactory::FindContext(const Message& msg) const
{
   Debug::ft(SsmFactory_FindContext);

   //  This must be implemented by a subclass if required.
   //
   Debug::SwLog(SsmFactory_FindContext, Fid(), 0);
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
