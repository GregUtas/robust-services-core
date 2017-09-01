//==============================================================================
//
//  PotsProxySessions.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsProxySessions.h"
#include "Debug.h"
#include "PotsBcHandlers.h"
#include "PotsProxyHandlers.h"
#include "PotsSessions.h"
#include "SbAppIds.h"
#include "Singleton.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
fn_name PotsProxyService_ctor = "PotsProxyService.ctor";

PotsProxyService::PotsProxyService() :
   ProxyBcService(PotsProxyServiceId, true)
{
   Debug::ft(PotsProxyService_ctor);

   //  Create and register all of our states, event handlers, and triggers.
   //
   Singleton< PotsProxyNull >::Instance();
   Singleton< PotsProxyAuthorizingOrigination >::Instance();
   Singleton< PotsProxyCollectingInformation >::Instance();
   Singleton< PotsProxyAnalyzingInformation >::Instance();
   Singleton< PotsProxySelectingRoute >::Instance();
   Singleton< PotsProxyAuthorizingCallSetup >::Instance();
   Singleton< PotsProxySendingCall >::Instance();
   Singleton< PotsProxyOrigAlerting >::Instance();
   Singleton< PotsProxyAuthorizingTermination >::Instance();
   Singleton< PotsProxySelectingFacility >::Instance();
   Singleton< PotsProxyPresentingCall >::Instance();
   Singleton< PotsProxyTermAlerting >::Instance();
   Singleton< PotsProxyActive >::Instance();
   Singleton< PotsProxyLocalSuspending >::Instance();
   Singleton< PotsProxyRemoteSuspending >::Instance();
   Singleton< PotsProxyException >::Instance();

   BindHandler(*Singleton< PotsProxyNuAnalyzeLocalMessage >::Instance(),
      BcEventHandler::NuAnalyzeLocalMessage);
   BindHandler(*Singleton< PotsProxyNuOriginate >::Instance(),
      BcEventHandler::NuOriginate);
   BindHandler(*Singleton< PotsProxyReleaseCall >::Instance(),
      BcEventHandler::NuReleaseCall);

   BindHandler(*Singleton< PotsProxyScAnalyzeLocalMessage >::Instance(),
      BcEventHandler::AoAnalyzeLocalMessage);
   BindHandler(*Singleton< PotsBcAoAuthorizeOrigination >::Instance(),
      BcEventHandler::AoAuthorizeOrigination);
   BindHandler(*Singleton< PotsBcAoOriginationDenied >::Instance(),
      BcEventHandler::AoOriginationDenied);
   BindHandler(*Singleton< PotsProxyLocalRelease >::Instance(),
      BcEventHandler::AoLocalRelease);
   BindHandler(*Singleton< PotsProxyReleaseCall >::Instance(),
      BcEventHandler::AoReleaseCall);

   BindHandler(*Singleton< PotsProxyScAnalyzeLocalMessage >::Instance(),
      BcEventHandler::CiAnalyzeLocalMessage);
   BindHandler(*Singleton< PotsProxyCiCollectInformation >::Instance(),
      BcEventHandler::CiCollectInformation);
   BindHandler(*Singleton< PotsProxyLocalRelease >::Instance(),
      BcEventHandler::CiLocalRelease);
   BindHandler(*Singleton< PotsProxyReleaseCall >::Instance(),
      BcEventHandler::CiReleaseCall);

   BindHandler(*Singleton< PotsProxyScAnalyzeLocalMessage >::Instance(),
      BcEventHandler::AiAnalyzeLocalMessage);
   BindHandler(*Singleton< PotsBcAiAnalyzeInformation >::Instance(),
      BcEventHandler::AiAnalyzeInformation);
   BindHandler(*Singleton< PotsBcAiInvalidInformation >::Instance(),
      BcEventHandler::AiInvalidInformation);
   BindHandler(*Singleton< PotsProxyLocalRelease >::Instance(),
      BcEventHandler::AiLocalRelease);
   BindHandler(*Singleton< PotsProxyReleaseCall >::Instance(),
      BcEventHandler::AiReleaseCall);

   BindHandler(*Singleton< PotsProxyScAnalyzeLocalMessage >::Instance(),
      BcEventHandler::SrAnalyzeLocalMessage);
   BindHandler(*Singleton< PotsBcSrSelectRoute >::Instance(),
      BcEventHandler::SrSelectRoute);
   BindHandler(*Singleton< PotsProxyLocalRelease >::Instance(),
      BcEventHandler::SrLocalRelease);
   BindHandler(*Singleton< PotsProxyReleaseCall >::Instance(),
      BcEventHandler::SrReleaseCall);

   BindHandler(*Singleton< PotsProxyScAnalyzeLocalMessage >::Instance(),
      BcEventHandler::AsAnalyzeLocalMessage);
   BindHandler(*Singleton< PotsBcAsAuthorizeCallSetup >::Instance(),
      BcEventHandler::AsAuthorizeCallSetup);
   BindHandler(*Singleton< PotsProxyLocalRelease >::Instance(),
      BcEventHandler::AsLocalRelease);
   BindHandler(*Singleton< PotsProxyReleaseCall >::Instance(),
      BcEventHandler::AsReleaseCall);

   BindHandler(*Singleton< PotsProxyScAnalyzeLocalMessage >::Instance(),
      BcEventHandler::ScAnalyzeLocalMessage);
   BindHandler(*Singleton< PotsProxyScSendCall >::Instance(),
      BcEventHandler::ScSendCall);
   BindHandler(*Singleton< PotsProxyRemoteRelease >::Instance(),
      BcEventHandler::ScRemoteBusy);
   BindHandler(*Singleton< PotsProxyScRemoteProgress >::Instance(),
      BcEventHandler::ScRemoteProgress);
   BindHandler(*Singleton< PotsProxyScRemoteAlerting >::Instance(),
      BcEventHandler::ScRemoteAlerting);
   BindHandler(*Singleton< PotsProxyRemoteAnswer >::Instance(),
      BcEventHandler::ScRemoteAnswer);
   BindHandler(*Singleton< PotsProxyRemoteRelease >::Instance(),
      BcEventHandler::ScRemoteRelease);
   BindHandler(*Singleton< PotsProxyLocalRelease >::Instance(),
      BcEventHandler::ScLocalRelease);
   BindHandler(*Singleton< PotsProxyReleaseCall >::Instance(),
      BcEventHandler::ScReleaseCall);

   BindHandler(*Singleton< PotsProxyScAnalyzeLocalMessage >::Instance(),
      BcEventHandler::OaAnalyzeLocalMessage);
   BindHandler(*Singleton< PotsProxyRemoteAnswer >::Instance(),
      BcEventHandler::OaRemoteAnswer);
   BindHandler(*Singleton< PotsProxyRemoteRelease >::Instance(),
      BcEventHandler::OaRemoteNoAnswer);
   BindHandler(*Singleton< PotsProxyRemoteRelease >::Instance(),
      BcEventHandler::OaRemoteRelease);
   BindHandler(*Singleton< PotsProxyLocalRelease >::Instance(),
      BcEventHandler::OaLocalRelease);
   BindHandler(*Singleton< PotsProxyReleaseCall >::Instance(),
      BcEventHandler::OaReleaseCall);

   //  A pure proxy call does not support the TA or SF states.  When a
   //  proxy UPSM is allocated on TBC, the call enters the PC state.

   BindHandler(*Singleton< PotsProxyPcAnalyzeLocalMessage >::Instance(),
      BcEventHandler::PcAnalyzeLocalMessage);
   BindHandler(*Singleton< PotsProxyPcLocalProgress >::Instance(),
      BcEventHandler::PcLocalProgress);
   BindHandler(*Singleton< PotsProxyLocalAlerting >::Instance(),
      BcEventHandler::PcLocalAlerting);
   BindHandler(*Singleton< PotsProxyLocalAnswer >::Instance(),
      BcEventHandler::PcLocalAnswer);
   BindHandler(*Singleton< PotsProxyLocalRelease >::Instance(),
      BcEventHandler::PcLocalRelease);
   BindHandler(*Singleton< PotsProxyRemoteRelease >::Instance(),
      BcEventHandler::PcRemoteRelease);
   BindHandler(*Singleton< PotsProxyReleaseCall >::Instance(),
      BcEventHandler::PcReleaseCall);

   BindHandler(*Singleton< PotsProxyTaAnalyzeLocalMessage >::Instance(),
      BcEventHandler::TaAnalyzeLocalMessage);
   BindHandler(*Singleton< PotsProxyLocalAlerting >::Instance(),
      ProxyBcEventHandler::TaLocalAlerting);
   BindHandler(*Singleton< PotsProxyLocalAnswer >::Instance(),
      BcEventHandler::TaLocalAnswer);
   BindHandler(*Singleton< PotsProxyLocalRelease >::Instance(),
      BcEventHandler::TaLocalRelease);
   BindHandler(*Singleton< PotsProxyRemoteRelease >::Instance(),
      BcEventHandler::TaRemoteRelease);
   BindHandler(*Singleton< PotsProxyReleaseCall >::Instance(),
      BcEventHandler::TaReleaseCall);

   BindHandler(*Singleton< PotsProxyAcAnalyzeLocalMessage >::Instance(),
      BcEventHandler::AcAnalyzeLocalMessage);
   BindHandler(*Singleton< PotsProxyLocalAnswer >::Instance(),
      ProxyBcEventHandler::AcLocalAnswer);
   BindHandler(*Singleton< PotsProxyAcLocalSuspend >::Instance(),
      BcEventHandler::AcLocalSuspend);
   BindHandler(*Singleton< PotsProxyLocalRelease >::Instance(),
      BcEventHandler::AcLocalRelease);
   BindHandler(*Singleton< PotsProxyAcRemoteSuspend >::Instance(),
      BcEventHandler::AcRemoteSuspend);
   BindHandler(*Singleton< PotsProxyRemoteRelease >::Instance(),
      BcEventHandler::AcRemoteRelease);
   BindHandler(*Singleton< PotsProxyReleaseCall >::Instance(),
      BcEventHandler::AcReleaseCall);

   BindHandler(*Singleton< PotsProxyAcAnalyzeLocalMessage >::Instance(),
      BcEventHandler::LsAnalyzeLocalMessage);
   BindHandler(*Singleton< PotsProxyLsLocalResume >::Instance(),
      BcEventHandler::LsLocalResume);
   BindHandler(*Singleton< PotsProxyLocalRelease >::Instance(),
      BcEventHandler::LsLocalRelease);
   BindHandler(*Singleton< PotsProxyRemoteRelease >::Instance(),
      BcEventHandler::LsRemoteRelease);
   BindHandler(*Singleton< PotsProxyReleaseCall >::Instance(),
      BcEventHandler::LsReleaseCall);

   BindHandler(*Singleton< PotsProxyAcAnalyzeLocalMessage >::Instance(),
      BcEventHandler::RsAnalyzeLocalMessage);
   BindHandler(*Singleton< PotsProxyLocalRelease >::Instance(),
      BcEventHandler::RsLocalRelease);
   BindHandler(*Singleton< PotsProxyRsRemoteResume >::Instance(),
      BcEventHandler::RsRemoteResume);
   BindHandler(*Singleton< PotsProxyRemoteRelease >::Instance(),
      BcEventHandler::RsRemoteRelease);
   BindHandler(*Singleton< PotsProxyReleaseCall >::Instance(),
      BcEventHandler::RsReleaseCall);

   BindTrigger(*Singleton< PotsAuthorizeOriginationSap >::Instance());
   BindTrigger(*Singleton< PotsCollectInformationSap >::Instance());
}

//------------------------------------------------------------------------------

fn_name PotsProxyService_dtor = "PotsProxyService.dtor";

PotsProxyService::~PotsProxyService()
{
   Debug::ft(PotsProxyService_dtor);
}

//==============================================================================

PotsProxyNull::PotsProxyNull() :
   ProxyBcNull(PotsProxyServiceId) { }

PotsProxyNull::~PotsProxyNull() { }

PotsProxyAuthorizingOrigination::PotsProxyAuthorizingOrigination() :
   ProxyBcAuthorizingOrigination(PotsProxyServiceId) { }

PotsProxyAuthorizingOrigination::~PotsProxyAuthorizingOrigination() { }

PotsProxyCollectingInformation::PotsProxyCollectingInformation() :
   ProxyBcCollectingInformation(PotsProxyServiceId) { }

PotsProxyCollectingInformation::~PotsProxyCollectingInformation() { }

PotsProxyAnalyzingInformation::PotsProxyAnalyzingInformation() :
   ProxyBcAnalyzingInformation(PotsProxyServiceId) { }

PotsProxyAnalyzingInformation::~PotsProxyAnalyzingInformation() { }

PotsProxySelectingRoute::PotsProxySelectingRoute() :
   ProxyBcSelectingRoute(PotsProxyServiceId) { }

PotsProxySelectingRoute::~PotsProxySelectingRoute() { }

PotsProxyAuthorizingCallSetup::PotsProxyAuthorizingCallSetup() :
   ProxyBcAuthorizingCallSetup(PotsProxyServiceId) { }

PotsProxyAuthorizingCallSetup::~PotsProxyAuthorizingCallSetup() { }

PotsProxySendingCall::PotsProxySendingCall() :
   ProxyBcSendingCall(PotsProxyServiceId) { }

PotsProxySendingCall::~PotsProxySendingCall() { }

PotsProxyOrigAlerting::PotsProxyOrigAlerting() :
   ProxyBcOrigAlerting(PotsProxyServiceId) { }

PotsProxyOrigAlerting::~PotsProxyOrigAlerting() { }

PotsProxyAuthorizingTermination::PotsProxyAuthorizingTermination() :
   ProxyBcAuthorizingTermination(PotsProxyServiceId) { }

PotsProxyAuthorizingTermination::~PotsProxyAuthorizingTermination() { }

PotsProxySelectingFacility::PotsProxySelectingFacility() :
   ProxyBcSelectingFacility(PotsProxyServiceId) { }

PotsProxySelectingFacility::~PotsProxySelectingFacility() { }

PotsProxyPresentingCall::PotsProxyPresentingCall() :
   ProxyBcPresentingCall(PotsProxyServiceId) { }

PotsProxyPresentingCall::~PotsProxyPresentingCall() { }

PotsProxyTermAlerting::PotsProxyTermAlerting() :
   ProxyBcTermAlerting(PotsProxyServiceId) { }

PotsProxyTermAlerting::~PotsProxyTermAlerting() { }

PotsProxyActive::PotsProxyActive() :
   ProxyBcActive(PotsProxyServiceId) { }

PotsProxyActive::~PotsProxyActive() { }

PotsProxyLocalSuspending::PotsProxyLocalSuspending() :
   ProxyBcLocalSuspending(PotsProxyServiceId) { }

PotsProxyLocalSuspending::~PotsProxyLocalSuspending() { }

PotsProxyRemoteSuspending::PotsProxyRemoteSuspending() :
   ProxyBcRemoteSuspending(PotsProxyServiceId) { }

PotsProxyRemoteSuspending::~PotsProxyRemoteSuspending() { }

PotsProxyException::PotsProxyException() :
   ProxyBcException(PotsProxyServiceId) { }

PotsProxyException::~PotsProxyException() { }
}