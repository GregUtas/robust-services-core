//==============================================================================
//
//  BcFactory.cpp
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
#include "BcSessions.h"
#include "Debug.h"

//------------------------------------------------------------------------------

namespace CallBase
{
fn_name BcFactory_ctor = "BcFactory.ctor";

BcFactory::BcFactory(Id fid, ProtocolId prid, c_string name) :
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

   Debug::SwLog(BcFactory_VerifyRoute, strOver(this), Fid());
   return Cause::ExchangeRoutingError;
}
}
