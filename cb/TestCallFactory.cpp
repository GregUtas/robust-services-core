//==============================================================================
//
//  TestCallFactory.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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
fn_name TestCallFactory_ctor = "TestCallFactory.ctor";

TestCallFactory::TestCallFactory() :
   BcFactory(TestCallFactoryId, CipProtocolId, "CIP Test Calls")
{
   Debug::ft(TestCallFactory_ctor);
}

//------------------------------------------------------------------------------

fn_name TestCallFactory_dtor = "TestCallFactory.dtor";

TestCallFactory::~TestCallFactory()
{
   Debug::ft(TestCallFactory_dtor);
}

//------------------------------------------------------------------------------

fn_name TestCallFactory_AllocIcPsm = "TestCallFactory.AllocIcPsm";

ProtocolSM* TestCallFactory::AllocIcPsm
   (const Message& msg, ProtocolLayer& lower) const
{
   Debug::ft(TestCallFactory_AllocIcPsm);

   return new CipPsm(CipTbcFactoryId, lower, false);
}

//------------------------------------------------------------------------------

fn_name TestCallFactory_AllocRoot = "TestCallFactory.AllocRoot";

RootServiceSM* TestCallFactory::AllocRoot
   (const Message& msg, ProtocolSM& psm) const
{
   Debug::ft(TestCallFactory_AllocRoot);

   return new TestSsm(psm);
}

//------------------------------------------------------------------------------

fn_name TestCallFactory_VerifyRoute = "TestCallFactory.VerifyRoute";

Cause::Ind TestCallFactory::VerifyRoute(RouteResult::Id rid) const
{
   Debug::ft(TestCallFactory_VerifyRoute);

   //b Change this to access the DN registry once DnProfile is refactored
   //  as a base class for PotsProfile.

   //  There is no point in sending a CIP IAM if the destination DN is not
   //  registered.
   //
// if(Singleton< PotsProfileRegistry >::Instance()->Profile(rid) == nullptr)
// {
//    return Cause::UnallocatedNumber;
// }

   return Cause::NilInd;
}
}