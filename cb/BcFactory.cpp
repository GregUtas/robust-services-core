//==============================================================================
//
//  BcFactory.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "BcSessions.h"
#include "Debug.h"

//------------------------------------------------------------------------------

namespace CallBase
{
fn_name BcFactory_ctor = "BcFactory.ctor";

BcFactory::BcFactory(Id fid, ProtocolId prid, const char* name) :
   SsmFactory(fid, prid, name)
{
   Debug::ft(BcFactory_ctor);
}

//------------------------------------------------------------------------------

fn_name BcFactory_dtor = "BcFactory.dtor";

BcFactory::~BcFactory()
{
   Debug::ft(BcFactory_dtor);
}

//------------------------------------------------------------------------------

fn_name BcFactory_VerifyRoute = "BcFactory.VerifyRoute";

Cause::Ind BcFactory::VerifyRoute(RouteResult::Id rid) const
{
   Debug::ft(BcFactory_VerifyRoute);

   Debug::SwErr(BcFactory_VerifyRoute, Fid(), 0);
   return Cause::ExchangeRoutingError;
}
}