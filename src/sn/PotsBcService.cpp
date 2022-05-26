//==============================================================================
//
//  PotsBcService.cpp
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
#include "PotsSessions.h"
#include "Debug.h"
#include "PotsBcHandlers.h"
#include "SbAppIds.h"
#include "Singleton.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
PotsBcService::PotsBcService() : ProxyBcService(PotsCallServiceId, true)
{
   Debug::ft("PotsBcService.ctor");

   //  Create and register all of our states, event handlers, and triggers.
   //
   Singleton<PotsBcNull>::Instance();
   Singleton<PotsBcAuthorizingOrigination>::Instance();
   Singleton<PotsBcCollectingInformation>::Instance();
   Singleton<PotsBcAnalyzingInformation>::Instance();
   Singleton<PotsBcSelectingRoute>::Instance();
   Singleton<PotsBcAuthorizingCallSetup>::Instance();
   Singleton<PotsBcSendingCall>::Instance();
   Singleton<PotsBcOrigAlerting>::Instance();
   Singleton<PotsBcAuthorizingTermination>::Instance();
   Singleton<PotsBcSelectingFacility>::Instance();
   Singleton<PotsBcPresentingCall>::Instance();
   Singleton<PotsBcTermAlerting>::Instance();
   Singleton<PotsBcActive>::Instance();
   Singleton<PotsBcLocalSuspending>::Instance();
   Singleton<PotsBcRemoteSuspending>::Instance();
   Singleton<PotsBcException>::Instance();

   BindHandler(*Singleton<PotsBcNuAnalyzeLocalMessage>::Instance(),
      BcEventHandler::NuAnalyzeLocalMessage);
   BindHandler(*Singleton<PotsBcNuOriginate>::Instance(),
      BcEventHandler::NuOriginate);
   BindHandler(*Singleton<PotsBcNuTerminate>::Instance(),
      BcEventHandler::NuTerminate);
   BindHandler(*Singleton<PotsBcReleaseCall>::Instance(),
      BcEventHandler::NuReleaseCall);

   BindHandler(*Singleton<PotsBcAoAnalyzeLocalMessage>::Instance(),
      BcEventHandler::AoAnalyzeLocalMessage);
   BindHandler(*Singleton<PotsBcAoAuthorizeOrigination>::Instance(),
      BcEventHandler::AoAuthorizeOrigination);
   BindHandler(*Singleton<PotsBcAoOriginationDenied>::Instance(),
      BcEventHandler::AoOriginationDenied);
   BindHandler(*Singleton<PotsBcLocalRelease>::Instance(),
      BcEventHandler::AoLocalRelease);
   BindHandler(*Singleton<PotsBcReleaseCall>::Instance(),
      BcEventHandler::AoReleaseCall);

   BindHandler(*Singleton<PotsBcCiAnalyzeLocalMessage>::Instance(),
      BcEventHandler::CiAnalyzeLocalMessage);
   BindHandler(*Singleton<PotsBcCiCollectInformation>::Instance(),
      BcEventHandler::CiCollectInformation);
   BindHandler(*Singleton<PotsBcCiLocalInformation>::Instance(),
      BcEventHandler::CiLocalInformation);
   BindHandler(*Singleton<PotsBcCiCollectionTimeout>::Instance(),
      BcEventHandler::CiCollectionTimeout);
   BindHandler(*Singleton<PotsBcLocalRelease>::Instance(),
      BcEventHandler::CiLocalRelease);
   BindHandler(*Singleton<PotsBcReleaseCall>::Instance(),
      BcEventHandler::CiReleaseCall);

   BindHandler(*Singleton<PotsBcAoAnalyzeLocalMessage>::Instance(),
      BcEventHandler::AiAnalyzeLocalMessage);
   BindHandler(*Singleton<PotsBcAiAnalyzeInformation>::Instance(),
      BcEventHandler::AiAnalyzeInformation);
   BindHandler(*Singleton<PotsBcAiInvalidInformation>::Instance(),
      BcEventHandler::AiInvalidInformation);
   BindHandler(*Singleton<PotsBcLocalRelease>::Instance(),
      BcEventHandler::AiLocalRelease);
   BindHandler(*Singleton<PotsBcReleaseCall>::Instance(),
      BcEventHandler::AiReleaseCall);

   BindHandler(*Singleton<PotsBcAoAnalyzeLocalMessage>::Instance(),
      BcEventHandler::SrAnalyzeLocalMessage);
   BindHandler(*Singleton<PotsBcSrSelectRoute>::Instance(),
      BcEventHandler::SrSelectRoute);
   BindHandler(*Singleton<PotsBcLocalRelease>::Instance(),
      BcEventHandler::SrLocalRelease);
   BindHandler(*Singleton<PotsBcReleaseCall>::Instance(),
      BcEventHandler::SrReleaseCall);

   BindHandler(*Singleton<PotsBcAoAnalyzeLocalMessage>::Instance(),
      BcEventHandler::AsAnalyzeLocalMessage);
   BindHandler(*Singleton<PotsBcAsAuthorizeCallSetup>::Instance(),
      BcEventHandler::AsAuthorizeCallSetup);
   BindHandler(*Singleton<PotsBcLocalRelease>::Instance(),
      BcEventHandler::AsLocalRelease);
   BindHandler(*Singleton<PotsBcReleaseCall>::Instance(),
      BcEventHandler::AsReleaseCall);

   BindHandler(*Singleton<PotsBcScAnalyzeLocalMessage>::Instance(),
      BcEventHandler::ScAnalyzeLocalMessage);
   BindHandler(*Singleton<PotsBcScSendCall>::Instance(),
      BcEventHandler::ScSendCall);
   BindHandler(*Singleton<PotsBcScRemoteBusy>::Instance(),
      BcEventHandler::ScRemoteBusy);
   BindHandler(*Singleton<PotsBcScRemoteProgress>::Instance(),
      BcEventHandler::ScRemoteProgress);
   BindHandler(*Singleton<PotsBcScRemoteAlerting>::Instance(),
      BcEventHandler::ScRemoteAlerting);
   BindHandler(*Singleton<PotsBcRemoteAnswer>::Instance(),
      BcEventHandler::ScRemoteAnswer);
   BindHandler(*Singleton<PotsBcScRemoteRelease>::Instance(),
      BcEventHandler::ScRemoteRelease);
   BindHandler(*Singleton<PotsBcLocalRelease>::Instance(),
      BcEventHandler::ScLocalRelease);
   BindHandler(*Singleton<PotsBcReleaseCall>::Instance(),
      BcEventHandler::ScReleaseCall);

   BindHandler(*Singleton<PotsBcScAnalyzeLocalMessage>::Instance(),
      BcEventHandler::OaAnalyzeLocalMessage);
   BindHandler(*Singleton<PotsBcRemoteAnswer>::Instance(),
      BcEventHandler::OaRemoteAnswer);
   BindHandler(*Singleton<PotsBcOaRemoteNoAnswer>::Instance(),
      BcEventHandler::OaRemoteNoAnswer);
   BindHandler(*Singleton<PotsBcScRemoteRelease>::Instance(),
      BcEventHandler::OaRemoteRelease);
   BindHandler(*Singleton<PotsBcLocalRelease>::Instance(),
      BcEventHandler::OaLocalRelease);
   BindHandler(*Singleton<PotsBcReleaseCall>::Instance(),
      BcEventHandler::OaReleaseCall);

   BindHandler(*Singleton<PotsBcAtAuthorizeTermination>::Instance(),
      BcEventHandler::AtAuthorizeTermination);
   BindHandler(*Singleton<PotsBcAtTerminationDenied>::Instance(),
      BcEventHandler::AtTerminationDenied);
   BindHandler(*Singleton<PotsBcScRemoteRelease>::Instance(),
      BcEventHandler::AtRemoteRelease);
   BindHandler(*Singleton<PotsBcReleaseCall>::Instance(),
      BcEventHandler::AtReleaseCall);

   BindHandler(*Singleton<PotsBcSfAnalyzeLocalMessage>::Instance(),
      BcEventHandler::SfAnalyzeLocalMessage);
   BindHandler(*Singleton<PotsBcSfSelectFacility>::Instance(),
      BcEventHandler::SfSelectFacility);
   BindHandler(*Singleton<PotsBcSfLocalBusy>::Instance(),
      BcEventHandler::SfLocalBusy);
   BindHandler(*Singleton<PotsBcScRemoteRelease>::Instance(),
      BcEventHandler::SfRemoteRelease);
   BindHandler(*Singleton<PotsBcReleaseCall>::Instance(),
      BcEventHandler::SfReleaseCall);

   BindHandler(*Singleton<PotsBcPcAnalyzeLocalMessage>::Instance(),
      BcEventHandler::PcAnalyzeLocalMessage);
   BindHandler(*Singleton<PotsBcPcPresentCall>::Instance(),
      BcEventHandler::PcPresentCall);
   BindHandler(*Singleton<PotsBcPcFacilityFailure>::Instance(),
      BcEventHandler::PcFacilityFailure);
   BindHandler(*Singleton<PotsBcPcLocalAlerting>::Instance(),
      BcEventHandler::PcLocalAlerting);
   BindHandler(*Singleton<PotsBcLocalAnswer>::Instance(),
      BcEventHandler::PcLocalAnswer);
   BindHandler(*Singleton<PotsBcPcRemoteRelease>::Instance(),
      BcEventHandler::PcRemoteRelease);
   BindHandler(*Singleton<PotsBcReleaseCall>::Instance(),
      BcEventHandler::PcReleaseCall);

   BindHandler(*Singleton<PotsBcTaAnalyzeLocalMessage>::Instance(),
      BcEventHandler::TaAnalyzeLocalMessage);
   BindHandler(*Singleton<PotsBcLocalAnswer>::Instance(),
      BcEventHandler::TaLocalAnswer);
   BindHandler(*Singleton<PotsBcTaLocalNoAnswer>::Instance(),
      BcEventHandler::TaLocalNoAnswer);
   BindHandler(*Singleton<PotsBcTaRemoteRelease>::Instance(),
      BcEventHandler::TaRemoteRelease);
   BindHandler(*Singleton<PotsBcReleaseCall>::Instance(),
      BcEventHandler::TaReleaseCall);

   BindHandler(*Singleton<PotsBcAcAnalyzeLocalMessage>::Instance(),
      BcEventHandler::AcAnalyzeLocalMessage);
   BindHandler(*Singleton<PotsBcAcLocalSuspend>::Instance(),
      BcEventHandler::AcLocalSuspend);
   BindHandler(*Singleton<PotsBcLocalRelease>::Instance(),
      BcEventHandler::AcLocalRelease);
   BindHandler(*Singleton<PotsBcAcRemoteSuspend>::Instance(),
      BcEventHandler::AcRemoteSuspend);
   BindHandler(*Singleton<PotsBcScRemoteRelease>::Instance(),
      BcEventHandler::AcRemoteRelease);
   BindHandler(*Singleton<PotsBcReleaseCall>::Instance(),
      BcEventHandler::AcReleaseCall);

   BindHandler(*Singleton<PotsBcAcAnalyzeLocalMessage>::Instance(),
      BcEventHandler::LsAnalyzeLocalMessage);
   BindHandler(*Singleton<PotsBcLsLocalResume>::Instance(),
      BcEventHandler::LsLocalResume);
   BindHandler(*Singleton<PotsBcLocalRelease>::Instance(),
      BcEventHandler::LsLocalRelease);
   BindHandler(*Singleton<PotsBcLsRemoteRelease>::Instance(),
      BcEventHandler::LsRemoteRelease);
   BindHandler(*Singleton<PotsBcReleaseCall>::Instance(),
      BcEventHandler::LsReleaseCall);

   BindHandler(*Singleton<PotsBcAcAnalyzeLocalMessage>::Instance(),
      BcEventHandler::RsAnalyzeLocalMessage);
   BindHandler(*Singleton<PotsBcLocalRelease>::Instance(),
      BcEventHandler::RsLocalRelease);
   BindHandler(*Singleton<PotsBcRsRemoteResume>::Instance(),
      BcEventHandler::RsRemoteResume);
   BindHandler(*Singleton<PotsBcScRemoteRelease>::Instance(),
      BcEventHandler::RsRemoteRelease);
   BindHandler(*Singleton<PotsBcReleaseCall>::Instance(),
      BcEventHandler::RsReleaseCall);

   BindHandler(*Singleton<PotsBcExAnalyzeLocalMessage>::Instance(),
      BcEventHandler::ExAnalyzeLocalMessage);
   BindHandler(*Singleton<PotsBcExApplyTreatment>::Instance(),
      BcEventHandler::ExApplyTreatment);
   BindHandler(*Singleton<PotsBcLocalRelease>::Instance(),
      BcEventHandler::ExLocalRelease);
   BindHandler(*Singleton<PotsBcReleaseCall>::Instance(),
      BcEventHandler::ExReleaseCall);

   BindHandler(*Singleton<PotsBcReleaseUser>::Instance(),
      ProxyBcEventHandler::ReleaseUser);

   BindTrigger(*Singleton<PotsAuthorizeOriginationSap>::Instance());
   BindTrigger(*Singleton<PotsCollectInformationSap>::Instance());
   BindTrigger(*Singleton<PotsAuthorizeTerminationSap>::Instance());
   BindTrigger(*Singleton<PotsLocalBusySap>::Instance());
   BindTrigger(*Singleton<PotsLocalAlertingSnp>::Instance());
}

//------------------------------------------------------------------------------

PotsBcService::~PotsBcService()
{
   Debug::ftnt("PotsBcService.dtor");
}
}
