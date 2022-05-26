//==============================================================================
//
//  TestCallFactory.cpp
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
#include "BcSessions.h"
#include "BcProtocol.h"
#include "Debug.h"
#include "SbAppIds.h"
#include "TestSessions.h"

using namespace SessionTools;

//------------------------------------------------------------------------------

namespace CallBase
{
TestCallFactory::TestCallFactory() :
   BcFactory(TestCallFactoryId, CipProtocolId, "CIP Test Calls")
{
   Debug::ft("TestCallFactory.ctor");
}

//------------------------------------------------------------------------------

TestCallFactory::~TestCallFactory()
{
   Debug::ftnt("TestCallFactory.dtor");
}

//------------------------------------------------------------------------------

ProtocolSM* TestCallFactory::AllocIcPsm
   (const Message& msg, ProtocolLayer& lower) const
{
   Debug::ft("TestCallFactory.AllocIcPsm");

   return new CipPsm(CipTbcFactoryId, lower, false);
}

//------------------------------------------------------------------------------

RootServiceSM* TestCallFactory::AllocRoot
   (const Message& msg, ProtocolSM& psm) const
{
   Debug::ft("TestCallFactory.AllocRoot");

   return new TestSsm(psm);
}

//------------------------------------------------------------------------------

Cause::Ind TestCallFactory::VerifyRoute(RouteResult::Id rid) const
{
   Debug::ft("TestCallFactory.VerifyRoute");

   //b Change this to access the DN registry once DnProfile is refactored
   //  as a base class for PotsProfile.

   //  There is no point in sending a CIP IAM if the destination DN is not
   //  registered.
   //
// if(Singleton<PotsProfileRegistry>::Instance()->Profile(rid) == nullptr)
// {
//    return Cause::UnallocatedNumber;
// }

   return Cause::NilInd;
}
}
