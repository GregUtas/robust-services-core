//==============================================================================
//
//  CbModule.cpp
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
#include "CbModule.h"
#include "BcCause.h"
#include "BcProgress.h"
#include "BcProtocol.h"
#include "BcSessions.h"
#include "Debug.h"
#include "MbModule.h"
#include "ModuleRegistry.h"
#include "NbAppIds.h"
#include "ProxyBcSessions.h"
#include "SbAppIds.h"
#include "ServiceCodeRegistry.h"
#include "Singleton.h"
#include "StModule.h"
#include "SymbolRegistry.h"

using namespace SessionBase;
using namespace SessionTools;
using namespace MediaBase;

//------------------------------------------------------------------------------

namespace CallBase
{
CbModule::CbModule() : Module()
{
   Debug::ft("CbModule.ctor");

   //  Create the modules required by CallBase.
   //
   Singleton<StModule>::Instance();
   Singleton<MbModule>::Instance();
   Singleton<ModuleRegistry>::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

CbModule::~CbModule()
{
   Debug::ftnt("CbModule.dtor");
}

//------------------------------------------------------------------------------

void CbModule::Shutdown(RestartLevel level)
{
   Debug::ft("CbModule.Shutdown");

   Singleton<ServiceCodeRegistry>::Instance()->Shutdown(level);

   BcSsm::ResetStateCounts(level);
}

//------------------------------------------------------------------------------

void CbModule::Startup(RestartLevel level)
{
   Debug::ft("CbModule.Startup");

   Singleton<CipProtocol>::Instance()->Startup(level);
   Singleton<CipObcFactory>::Instance()->Startup(level);
   Singleton<CipTbcFactory>::Instance()->Startup(level);
   Singleton<TestCallFactory>::Instance()->Startup(level);
   Singleton<ProxyBcFactory>::Instance()->Startup(level);
   Singleton<CipUdpService>::Instance()->Startup(level);
   Singleton<CipTcpService>::Instance()->Startup(level);
   Singleton<ServiceCodeRegistry>::Instance()->Startup(level);

   //  Define symbols.
   //
   auto reg = Singleton<SymbolRegistry>::Instance();
   reg->BindSymbol("factory.cip.obc", CipObcFactoryId);
   reg->BindSymbol("factory.cip.tbc", CipTbcFactoryId);
   reg->BindSymbol("factory.call.proxy", ProxyCallFactoryId);
   reg->BindSymbol("factory.call.test", TestCallFactoryId);

   reg->BindSymbol("prog.eos", Progress::EndOfSelection);
   reg->BindSymbol("prog.alerting", Progress::Alerting);
   reg->BindSymbol("prog.suspend", Progress::Suspend);
   reg->BindSymbol("prog.resume", Progress::Resume);
   reg->BindSymbol("prog.media", Progress::MediaUpdate);

   reg->BindSymbol("cause.unallocnumber", Cause::UnallocatedNumber);
   reg->BindSymbol("cause.confirmation", Cause::Confirmation);
   reg->BindSymbol("cause.addresstimeout", Cause::AddressTimeout);
   reg->BindSymbol("cause.normal", Cause::NormalCallClearing);
   reg->BindSymbol("cause.userbusy", Cause::UserBusy);
   reg->BindSymbol("cause.alertingtimeout", Cause::AlertingTimeout);
   reg->BindSymbol("cause.answertimeout", Cause::AnswerTimeout);
   reg->BindSymbol("cause.exchangerouting", Cause::ExchangeRoutingError);
   reg->BindSymbol("cause.destoutoforder", Cause::DestinationOutOfOrder);
   reg->BindSymbol("cause.invalidaddress", Cause::InvalidAddress);
   reg->BindSymbol("cause.facilityreject", Cause::FacilityRejected);
   reg->BindSymbol("cause.temporary", Cause::TemporaryFailure);
   reg->BindSymbol("cause.incomingbarred", Cause::OutgoingCallsBarred);
   reg->BindSymbol("cause.outgoingbarred", Cause::IncomingCallsBarred);
   reg->BindSymbol("cause.callredirected", Cause::CallRedirected);
   reg->BindSymbol("cause.maxredirection", Cause::ExcessiveRedirection);
   reg->BindSymbol("cause.invalidmessage", Cause::MessageInvalidForState);
   reg->BindSymbol("cause.parameterabsent", Cause::ParameterAbsent);
   reg->BindSymbol("cause.protocoltimeout", Cause::ProtocolTimeout);
   reg->BindSymbol("cause.resetcircuit", Cause::ResetCircuit);

   reg->BindSymbol("flag.calltrap", CallTrapFlag);
   reg->BindSymbol("flag.cipiamtimeout", CipIamTimeoutFlag);
   reg->BindSymbol("flag.cipalertingtimeout", CipAlertingTimeoutFlag);
}
}
