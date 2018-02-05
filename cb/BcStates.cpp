//==============================================================================
//
//  BcStates.cpp
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
//b Event handlers not used in the POTS implementation are commented out.
//
fn_name BcState_ctor = "BcState.ctor";

BcState::BcState(ServiceId sid, Id stid) : State(sid, stid)
{
   Debug::ft(BcState_ctor);
}

fn_name BcState_dtor = "BcState.dtor";

BcState::~BcState()
{
   Debug::ft(BcState_dtor);
}

//------------------------------------------------------------------------------

BcNull::BcNull(ServiceId sid) : BcState(sid, Null)
{
   BindMsgAnalyzer
      (BcEventHandler::NuAnalyzeLocalMessage, Service::UserPort);
   BindMsgAnalyzer
      (BcEventHandler::NuAnalyzeRemoteMessage, Service::NetworkPort);
   BindEventHandler
      (BcEventHandler::NuOriginate, BcEvent::Originate);
   BindEventHandler
      (BcEventHandler::NuTerminate, BcEvent::Terminate);
   BindEventHandler
      (BcEventHandler::NuReleaseCall, BcEvent::ReleaseCall);
}

BcNull::~BcNull() { }

//------------------------------------------------------------------------------

BcAuthorizingOrigination::BcAuthorizingOrigination(ServiceId sid) :
   BcState(sid, AuthorizingOrigination)
{
   BindMsgAnalyzer
      (BcEventHandler::AoAnalyzeLocalMessage, Service::UserPort);
   BindEventHandler
      (BcEventHandler::AoAuthorizeOrigination, BcEvent::AuthorizeOrigination);
   BindEventHandler
      (BcEventHandler::AoOriginationDenied, BcEvent::OriginationDenied);
// BindEventHandler
//    (BcEventHandler::AoLocalDisconnect, BcEvent::LocalDisconnect);
   BindEventHandler
      (BcEventHandler::AoLocalRelease, BcEvent::LocalRelease);
   BindEventHandler
      (BcEventHandler::AoReleaseCall, BcEvent::ReleaseCall);
}

BcAuthorizingOrigination::~BcAuthorizingOrigination() { }

//------------------------------------------------------------------------------

BcCollectingInformation::BcCollectingInformation(ServiceId sid) :
   BcState(sid, CollectingInformation)
{
   BindMsgAnalyzer
      (BcEventHandler::CiAnalyzeLocalMessage, Service::UserPort);
   BindEventHandler
      (BcEventHandler::CiCollectInformation, BcEvent::CollectInformation);
   BindEventHandler
      (BcEventHandler::CiCollectionTimeout, BcEvent::CollectionTimeout);
   BindEventHandler
      (BcEventHandler::CiLocalInformation, BcEvent::LocalInformation);
// BindEventHandler
//    (BcEventHandler::CiLocalDisconnect, BcEvent::LocalDisconnect);
   BindEventHandler
      (BcEventHandler::CiLocalRelease, BcEvent::LocalRelease);
   BindEventHandler
      (BcEventHandler::CiReleaseCall, BcEvent::ReleaseCall);
}

BcCollectingInformation::~BcCollectingInformation() { }

//------------------------------------------------------------------------------

BcAnalyzingInformation::BcAnalyzingInformation(ServiceId sid) :
   BcState(sid, AnalyzingInformation)
{
   BindMsgAnalyzer
      (BcEventHandler::AiAnalyzeLocalMessage, Service::UserPort);
   BindEventHandler
      (BcEventHandler::AiAnalyzeInformation, BcEvent::AnalyzeInformation);
   BindEventHandler
      (BcEventHandler::AiInvalidInformation, BcEvent::InvalidInformation);
// BindEventHandler
//    (BcEventHandler::AiReanalyzeInformation, BcEvent::ReanalyzeInformation);
// BindEventHandler
//    (BcEventHandler::AiLocalDisconnect, BcEvent::LocalDisconnect);
   BindEventHandler
      (BcEventHandler::AiLocalRelease, BcEvent::LocalRelease);
   BindEventHandler
      (BcEventHandler::AiReleaseCall, BcEvent::ReleaseCall);
}

BcAnalyzingInformation::~BcAnalyzingInformation() { }

//------------------------------------------------------------------------------

BcSelectingRoute::BcSelectingRoute(ServiceId sid) :
   BcState(sid, SelectingRoute)
{
   BindMsgAnalyzer
      (BcEventHandler::SrAnalyzeLocalMessage, Service::UserPort);
   BindEventHandler
      (BcEventHandler::SrSelectRoute, BcEvent::SelectRoute);
// BindEventHandler
//    (BcEventHandler::SrReanalyzeInformation, BcEvent::ReanalyzeInformation);
// BindEventHandler
//    (BcEventHandler::SrNetworkBusy, BcEvent::NetworkBusy);
// BindEventHandler
//    (BcEventHandler::SrLocalDisconnect, BcEvent::LocalDisconnect);
   BindEventHandler
      (BcEventHandler::SrLocalRelease, BcEvent::LocalRelease);
   BindEventHandler
      (BcEventHandler::SrReleaseCall, BcEvent::ReleaseCall);
}

BcSelectingRoute::~BcSelectingRoute() { }

//------------------------------------------------------------------------------

BcAuthorizingCallSetup::BcAuthorizingCallSetup(ServiceId sid) :
   BcState(sid, AuthorizingCallSetup)
{
   BindMsgAnalyzer
      (BcEventHandler::AsAnalyzeLocalMessage, Service::UserPort);
   BindEventHandler
      (BcEventHandler::AsAuthorizeCallSetup, BcEvent::AuthorizeCallSetup);
// BindEventHandler
//    (BcEventHandler::AsAuthorizationDenied, BcEvent::AuthorizationDenied);
// BindEventHandler
//    (BcEventHandler::AsLocalDisconnect, BcEvent::LocalDisconnect);
   BindEventHandler
      (BcEventHandler::AsLocalRelease, BcEvent::LocalRelease);
   BindEventHandler
      (BcEventHandler::AsReleaseCall, BcEvent::ReleaseCall);
}

BcAuthorizingCallSetup::~BcAuthorizingCallSetup() { }

//------------------------------------------------------------------------------

BcSendingCall::BcSendingCall(ServiceId sid) :
   BcState(sid, SendingCall)
{
   BindMsgAnalyzer
      (BcEventHandler::ScAnalyzeLocalMessage, Service::UserPort);
   BindMsgAnalyzer
      (BcEventHandler::ScAnalyzeRemoteMessage, Service::NetworkPort);
   BindEventHandler
      (BcEventHandler::ScSendCall, BcEvent::SendCall);
// BindEventHandler
//    (BcEventHandler::ScRouteBusy, BcEvent::RouteBusy);
// BindEventHandler
//    (BcEventHandler::ScLocalInformation, BcEvent::LocalInformation);
   BindEventHandler
      (BcEventHandler::ScRemoteProgress, BcEvent::RemoteProgress);
   BindEventHandler
      (BcEventHandler::ScRemoteAlerting, BcEvent::RemoteAlerting);
   BindEventHandler
      (BcEventHandler::ScRemoteAnswer, BcEvent::RemoteAnswer);
   BindEventHandler
      (BcEventHandler::ScRemoteBusy, BcEvent::RemoteBusy);
// BindEventHandler
//    (BcEventHandler::ScRemoteNoAnswer, BcEvent::RemoteNoAnswer);
   BindEventHandler
      (BcEventHandler::ScRemoteRelease, BcEvent::RemoteRelease);
// BindEventHandler
//    (BcEventHandler::ScLocalDisconnect, BcEvent::LocalDisconnect);
   BindEventHandler
      (BcEventHandler::ScLocalRelease, BcEvent::LocalRelease);
   BindEventHandler
      (BcEventHandler::ScReleaseCall, BcEvent::ReleaseCall);
// BindEventHandler
//    (BcEventHandler::LocalProgress, BcEvent::LocalProgress);
}

BcSendingCall::~BcSendingCall() { }

//------------------------------------------------------------------------------

BcOrigAlerting::BcOrigAlerting(ServiceId sid) :
   BcState(sid, OrigAlerting)
{
   BindMsgAnalyzer
      (BcEventHandler::OaAnalyzeLocalMessage, Service::UserPort);
   BindMsgAnalyzer
      (BcEventHandler::OaAnalyzeRemoteMessage, Service::NetworkPort);
   BindEventHandler
      (BcEventHandler::OaRemoteAnswer, BcEvent::RemoteAnswer);
   BindEventHandler
      (BcEventHandler::OaRemoteNoAnswer, BcEvent::RemoteNoAnswer);
   BindEventHandler
      (BcEventHandler::OaRemoteRelease, BcEvent::RemoteRelease);
// BindEventHandler
//    (BcEventHandler::OaLocalDisconnect, BcEvent::LocalDisconnect);
   BindEventHandler
      (BcEventHandler::OaLocalRelease, BcEvent::LocalRelease);
   BindEventHandler
      (BcEventHandler::OaReleaseCall, BcEvent::ReleaseCall);
// BindEventHandler
//    (BcEventHandler::LocalProgress, BcEvent::LocalProgress);
// BindEventHandler
//    (BcEventHandler::RemoteProgress, BcEvent::RemoteProgress);
}

BcOrigAlerting::~BcOrigAlerting() { }

//------------------------------------------------------------------------------

BcAuthorizingTermination::BcAuthorizingTermination(ServiceId sid) :
   BcState(sid, AuthorizingTermination)
{
   BindMsgAnalyzer
      (BcEventHandler::AtAnalyzeRemoteMessage, Service::NetworkPort);
   BindEventHandler
      (BcEventHandler::AtAuthorizeTermination, BcEvent::AuthorizeTermination);
   BindEventHandler
      (BcEventHandler::AtTerminationDenied, BcEvent::TerminationDenied);
// BindEventHandler
//    (BcEventHandler::AtRemoteInformation, BcEvent::RemoteInformation);
   BindEventHandler
      (BcEventHandler::AtRemoteRelease, BcEvent::RemoteRelease);
   BindEventHandler
      (BcEventHandler::AtReleaseCall, BcEvent::ReleaseCall);
}

BcAuthorizingTermination::~BcAuthorizingTermination() { }

//------------------------------------------------------------------------------

BcSelectingFacility::BcSelectingFacility(ServiceId sid) :
   BcState(sid, SelectingFacility)
{
   BindMsgAnalyzer
      (BcEventHandler::SfAnalyzeLocalMessage, Service::UserPort);
   BindMsgAnalyzer
      (BcEventHandler::SfAnalyzeRemoteMessage, Service::NetworkPort);
   BindEventHandler
      (BcEventHandler::SfSelectFacility, BcEvent::SelectFacility);
// BindEventHandler
//    (BcEventHandler::SfFacilityFailure, BcEvent::FacilityFailure);
   BindEventHandler
      (BcEventHandler::SfLocalBusy, BcEvent::LocalBusy);
// BindEventHandler
//    (BcEventHandler::SfRemoteInformation, BcEvent::RemoteInformation);
   BindEventHandler
      (BcEventHandler::SfRemoteRelease, BcEvent::RemoteRelease);
   BindEventHandler
      (BcEventHandler::SfReleaseCall, BcEvent::ReleaseCall);
}

BcSelectingFacility::~BcSelectingFacility() { }

//------------------------------------------------------------------------------

BcPresentingCall::BcPresentingCall(ServiceId sid) :
   BcState(sid, PresentingCall)
{
   BindMsgAnalyzer
      (BcEventHandler::PcAnalyzeLocalMessage, Service::UserPort);
   BindMsgAnalyzer
      (BcEventHandler::PcAnalyzeRemoteMessage, Service::NetworkPort);
   BindEventHandler
      (BcEventHandler::PcPresentCall, BcEvent::PresentCall);
   BindEventHandler
      (BcEventHandler::PcFacilityFailure, BcEvent::FacilityFailure);
   BindEventHandler
      (BcEventHandler::PcLocalProgress, BcEvent::LocalProgress);
   BindEventHandler
      (BcEventHandler::PcLocalAlerting, BcEvent::LocalAlerting);
   BindEventHandler
      (BcEventHandler::PcLocalAnswer, BcEvent::LocalAnswer);
// BindEventHandler
//    (BcEventHandler::PcLocalNoAnswer, BcEvent::LocalNoAnswer);
   BindEventHandler
      (BcEventHandler::PcLocalRelease, BcEvent::LocalRelease);
// BindEventHandler
//    (BcEventHandler::PcRemoteInformation, BcEvent::RemoteInformation);
   BindEventHandler
      (BcEventHandler::PcRemoteRelease, BcEvent::RemoteRelease);
   BindEventHandler
      (BcEventHandler::PcReleaseCall, BcEvent::ReleaseCall);
// BindEventHandler
//    (BcEventHandler::RemoteProgress, BcEvent::RemoteProgress);
}

BcPresentingCall::~BcPresentingCall() { }

//------------------------------------------------------------------------------

BcTermAlerting::BcTermAlerting(ServiceId sid) :
   BcState(sid, TermAlerting)
{
   BindMsgAnalyzer
      (BcEventHandler::TaAnalyzeLocalMessage, Service::UserPort);
   BindMsgAnalyzer
      (BcEventHandler::TaAnalyzeRemoteMessage, Service::NetworkPort);
   BindEventHandler
      (BcEventHandler::TaLocalAnswer, BcEvent::LocalAnswer);
   BindEventHandler
      (BcEventHandler::TaLocalNoAnswer, BcEvent::LocalNoAnswer);
   BindEventHandler
      (BcEventHandler::TaLocalRelease, BcEvent::LocalRelease);
   BindEventHandler
      (BcEventHandler::TaRemoteRelease, BcEvent::RemoteRelease);
   BindEventHandler
      (BcEventHandler::TaReleaseCall, BcEvent::ReleaseCall);
// BindEventHandler
//    (BcEventHandler::LocalProgress, BcEvent::LocalProgress);
// BindEventHandler
//    (BcEventHandler::RemoteProgress, BcEvent::RemoteProgress);
}

BcTermAlerting::~BcTermAlerting() { }

//------------------------------------------------------------------------------

BcActive::BcActive(ServiceId sid) : BcState(sid, Active)
{
   BindMsgAnalyzer
      (BcEventHandler::AcAnalyzeLocalMessage, Service::UserPort);
   BindMsgAnalyzer
      (BcEventHandler::AcAnalyzeRemoteMessage, Service::NetworkPort);
   BindEventHandler
      (BcEventHandler::AcLocalSuspend, BcEvent::LocalSuspend);
// BindEventHandler
//    (BcEventHandler::AcLocalDisconnect, BcEvent::LocalDisconnect);
   BindEventHandler
      (BcEventHandler::AcLocalRelease, BcEvent::LocalRelease);
   BindEventHandler
      (BcEventHandler::AcRemoteSuspend, BcEvent::RemoteSuspend);
   BindEventHandler
      (BcEventHandler::AcRemoteRelease, BcEvent::RemoteRelease);
   BindEventHandler
      (BcEventHandler::AcReleaseCall, BcEvent::ReleaseCall);
// BindEventHandler
//    (BcEventHandler::LocalProgress, BcEvent::LocalProgress);
// BindEventHandler
//    (BcEventHandler::RemoteProgress, BcEvent::RemoteProgress);
}

BcActive::~BcActive() { }

//------------------------------------------------------------------------------

BcLocalSuspending::BcLocalSuspending(ServiceId sid) :
   BcState(sid, LocalSuspending)
{
   BindMsgAnalyzer
      (BcEventHandler::LsAnalyzeLocalMessage, Service::UserPort);
   BindMsgAnalyzer
      (BcEventHandler::LsAnalyzeRemoteMessage, Service::NetworkPort);
   BindEventHandler
      (BcEventHandler::LsLocalResume, BcEvent::LocalResume);
   BindEventHandler
      (BcEventHandler::LsLocalRelease, BcEvent::LocalRelease);
   BindEventHandler
      (BcEventHandler::LsRemoteRelease, BcEvent::RemoteRelease);
   BindEventHandler
      (BcEventHandler::LsReleaseCall, BcEvent::ReleaseCall);
// BindEventHandler
//    (BcEventHandler::LocalProgress, BcEvent::LocalProgress);
// BindEventHandler
//    (BcEventHandler::RemoteProgress, BcEvent::RemoteProgress);
}

BcLocalSuspending::~BcLocalSuspending() { }

//------------------------------------------------------------------------------

BcRemoteSuspending::BcRemoteSuspending(ServiceId sid) :
   BcState(sid, RemoteSuspending)
{
   BindMsgAnalyzer
      (BcEventHandler::RsAnalyzeLocalMessage, Service::UserPort);
   BindMsgAnalyzer
      (BcEventHandler::RsAnalyzeRemoteMessage, Service::NetworkPort);
// BindEventHandler
//    (BcEventHandler::RsLocalDisconnect, BcEvent::LocalDisconnect);
   BindEventHandler
      (BcEventHandler::RsLocalRelease, BcEvent::LocalRelease);
   BindEventHandler
      (BcEventHandler::RsRemoteResume, BcEvent::RemoteResume);
   BindEventHandler
      (BcEventHandler::RsRemoteRelease, BcEvent::RemoteRelease);
   BindEventHandler
      (BcEventHandler::RsReleaseCall, BcEvent::ReleaseCall);
// BindEventHandler
//    (BcEventHandler::LocalProgress, BcEvent::LocalProgress);
// BindEventHandler
//    (BcEventHandler::RemoteProgress, BcEvent::RemoteProgress);
}

BcRemoteSuspending::~BcRemoteSuspending() { }

//------------------------------------------------------------------------------

BcDisconnecting::BcDisconnecting(ServiceId sid) :
   BcState(sid, Disconnecting)
{
// BindMsgAnalyzer
//    (BcEventHandler::DiAnalyzeLocalMessage, Service::UserPort);
// BindEventHandler
//    (BcEventHandler::DiLocalRelease, BcEvent::LocalRelease);
// BindEventHandler
//    (BcEventHandler::DiDisconnectTimeout, BcEvent::DisconnectTimeout);
// BindEventHandler
//    (BcEventHandler::DiReleaseCall, BcEvent::ReleaseCall);
}

BcDisconnecting::~BcDisconnecting() { }

//------------------------------------------------------------------------------

BcException::BcException(ServiceId sid) : BcState(sid, Exception)
{
   BindMsgAnalyzer
      (BcEventHandler::ExAnalyzeLocalMessage, Service::UserPort);
   BindEventHandler
      (BcEventHandler::ExApplyTreatment, BcEvent::ApplyTreatment);
// BindEventHandler
//    (BcEventHandler::ExLocalDisconnect, BcEvent::LocalDisconnect);
   BindEventHandler
      (BcEventHandler::ExLocalRelease, BcEvent::LocalRelease);
   BindEventHandler
      (BcEventHandler::ExReleaseCall, BcEvent::ReleaseCall);
// BindEventHandler
//    (BcEventHandler::LocalProgress, BcEvent::LocalProgress);
}

BcException::~BcException() { }
}
