//==============================================================================
//
//  CbModule.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CbModule.h"
#include "BcCause.h"
#include "BcProgress.h"
#include "BcProtocol.h"
#include "BcSessions.h"
#include "Debug.h"
#include "MbModule.h"
#include "NbAppIds.h"
#include "ProxyBcSessions.h"
#include "SbAppIds.h"
#include "ServiceCodeRegistry.h"
#include "Singleton.h"
#include "StModule.h"
#include "SymbolRegistry.h"
#include "SysTypes.h"

using namespace SessionTools;
using namespace MediaBase;

//------------------------------------------------------------------------------

namespace CallBase
{
bool CbModule::Registered = Register();

//------------------------------------------------------------------------------

fn_name CbModule_ctor = "CbModule.ctor";

CbModule::CbModule() : Module(CbModuleId)
{
   Debug::ft(CbModule_ctor);
}

//------------------------------------------------------------------------------

fn_name CbModule_dtor = "CbModule.dtor";

CbModule::~CbModule()
{
   Debug::ft(CbModule_dtor);
}

//------------------------------------------------------------------------------

fn_name CbModule_Register = "CbModule.Register";

bool CbModule::Register()
{
   Debug::ft(CbModule_Register);

   //  Create the modules required by CallBase.
   //
   Singleton< StModule >::Instance();
   Singleton< MbModule >::Instance();
   Singleton< CbModule >::Instance();
   return true;
}

//------------------------------------------------------------------------------

fn_name CbModule_Shutdown = "CbModule.Shutdown";

void CbModule::Shutdown(RestartLevel level)
{
   Debug::ft(CbModule_Shutdown);

   Singleton< ServiceCodeRegistry >::Instance()->Shutdown(level);

   BcSsm::ResetStateCounts(level);
//s Singleton< CipUdpService >::Instance()->Shutdown(level);
   Singleton< CipTcpService >::Instance()->Shutdown(level);
   Singleton< ProxyBcFactory >::Instance()->Shutdown(level);
   Singleton< TestCallFactory >::Instance()->Shutdown(level);
   Singleton< CipTbcFactory >::Instance()->Shutdown(level);
   Singleton< CipObcFactory >::Instance()->Shutdown(level);
   Singleton< CipProtocol >::Instance()->Shutdown(level);
}

//------------------------------------------------------------------------------

fn_name CbModule_Startup = "CbModule.Startup";

void CbModule::Startup(RestartLevel level)
{
   Debug::ft(CbModule_Startup);

   Singleton< CipProtocol >::Instance()->Startup(level);
   Singleton< CipObcFactory >::Instance()->Startup(level);
   Singleton< CipTbcFactory >::Instance()->Startup(level);
   Singleton< TestCallFactory >::Instance()->Startup(level);
   Singleton< ProxyBcFactory >::Instance()->Startup(level);
//s Singleton< CipUdpService >::Instance()->Startup(level);
   Singleton< CipTcpService >::Instance()->Startup(level);
   Singleton< ServiceCodeRegistry >::Instance()->Startup(level);

   //  Define symbols.
   //
   if(level < RestartCold) return;

   auto reg = Singleton< SymbolRegistry >::Instance();

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
   reg->BindSymbol("flag.cipalwaysoverip", CipAlwaysOverIpFlag);
   reg->BindSymbol("flag.cipiamtimeout", CipIamTimeoutFlag);
   reg->BindSymbol("flag.cipalertingtimeout", CipAlertingTimeoutFlag);
}
}