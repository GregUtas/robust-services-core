//==============================================================================
//
//  BcService.cpp
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
#include "Debug.h"
#include "Singleton.h"

//------------------------------------------------------------------------------

namespace CallBase
{
fixed_string BcOriginateEventStr            = "BcOriginateEvent";
fixed_string BcAuthorizeOriginationEventStr = "BcAuthorizeOriginationEvent";
fixed_string BcOriginationDeniedEventStr    = "BcOriginationDeniedEvent";
fixed_string BcCollectInformationEventStr   = "BcCollectInformationEvent";
fixed_string BcCollectionTimeoutEventStr    = "BcCollectionTimeoutEvent";
fixed_string BcLocalInformationEventStr     = "BcLocalInformationEvent";
fixed_string BcAnalyzeInformationEventStr   = "BcAnalyzeInformationEvent";
fixed_string BcInvalidInformationEventStr   = "BcInvalidInformationEvent";
fixed_string BcSelectRouteEventStr          = "BcSelectRouteEvent";
fixed_string BcAuthorizeCallSetupEventStr   = "BcAuthorizeCallSetupEvent";
fixed_string BcSendCallEventStr             = "BcSendCallEvent";
fixed_string BcRemoteBusyEventStr           = "BcRemoteBusyEvent";
fixed_string BcRemoteAlertingEventStr       = "BcRemoteAlertingEvent";
fixed_string BcRemoteNoAnswerEventStr       = "BcRemoteNoAnswerEvent";
fixed_string BcRemoteProgressEventStr       = "BcRemoteProgressEvent";
fixed_string BcRemoteAnswerEventStr         = "BcRemoteAnswerEvent";
fixed_string BcTerminateEventStr            = "BcTerminateEvent";
fixed_string BcAuthorizeTerminationEventStr = "BcAuthorizeTerminationEvent";
fixed_string BcTerminationDeniedEventStr    = "BcTerminationDeniedEvent";
fixed_string BcSelectFacilityEventStr       = "BcSelectFacilityEvent";
fixed_string BcLocalBusyEventStr            = "BcLocalBusyEvent";
fixed_string BcPresentCallEventStr          = "BcPresentCallEvent";
fixed_string BcFacilityFailureEventStr      = "BcFacilityFailureEvent";
fixed_string BcLocalAlertingEventStr        = "BcLocalAlertingEvent";
fixed_string BcLocalNoAnswerEventStr        = "BcLocalNoAnswerEvent";
fixed_string BcLocalAnswerEventStr          = "BcLocalAnswerEvent";
fixed_string BcLocalSuspendEventStr         = "BcLocalSuspendEvent";
fixed_string BcLocalResumeEventStr          = "BcLocalResumeEvent";
fixed_string BcRemoteSuspendEventStr        = "BcRemoteSuspendEvent";
fixed_string BcRemoteResumeEventStr         = "BcRemoteResumeEvent";
fixed_string BcLocalReleaseEventStr         = "BcLocalReleaseEvent";
fixed_string BcRemoteReleaseEventStr        = "BcRemoteReleaseEvent";
fixed_string BcReleaseCallEventStr          = "BcReleaseCallEvent";
fixed_string BcApplyTreatmentEventStr       = "BcApplyTreatmentEvent";

//------------------------------------------------------------------------------

BcService::BcService(Id sid, bool modifiable) : Service(sid, modifiable, false)
{
   Debug::ft("BcService.ctor");

   BindHandler(*Singleton< BcNuAnalyzeRemoteMessage >::Instance(),
      BcEventHandler::NuAnalyzeRemoteMessage);

   BindHandler(*Singleton< BcScAnalyzeRemoteMessage >::Instance(),
      BcEventHandler::ScAnalyzeRemoteMessage);
   BindHandler(*Singleton< BcOaAnalyzeRemoteMessage >::Instance(),
      BcEventHandler::OaAnalyzeRemoteMessage);

   BindHandler(*Singleton< BcPcAnalyzeRemoteMessage >::Instance(),
      BcEventHandler::AtAnalyzeRemoteMessage);
   BindHandler(*Singleton< BcPcAnalyzeRemoteMessage >::Instance(),
      BcEventHandler::SfAnalyzeRemoteMessage);
   BindHandler(*Singleton< BcPcAnalyzeRemoteMessage >::Instance(),
      BcEventHandler::PcAnalyzeRemoteMessage);
   BindHandler(*Singleton< BcPcAnalyzeRemoteMessage >::Instance(),
      BcEventHandler::TaAnalyzeRemoteMessage);

   BindHandler(*Singleton< BcAcAnalyzeRemoteMessage >::Instance(),
      BcEventHandler::AcAnalyzeRemoteMessage);
   BindHandler(*Singleton< BcAcAnalyzeRemoteMessage >::Instance(),
      BcEventHandler::LsAnalyzeRemoteMessage);
   BindHandler(*Singleton< BcAcAnalyzeRemoteMessage >::Instance(),
      BcEventHandler::RsAnalyzeRemoteMessage);

   BindEventName(BcOriginateEventStr, BcEvent::Originate);
   BindEventName(BcAuthorizeOriginationEventStr, BcEvent::AuthorizeOrigination);
   BindEventName(BcOriginationDeniedEventStr, BcEvent::OriginationDenied);
   BindEventName(BcCollectInformationEventStr, BcEvent::CollectInformation);
   BindEventName(BcCollectionTimeoutEventStr, BcEvent::CollectionTimeout);
   BindEventName(BcLocalInformationEventStr, BcEvent::LocalInformation);
   BindEventName(BcAnalyzeInformationEventStr, BcEvent::AnalyzeInformation);
   BindEventName(BcInvalidInformationEventStr, BcEvent::InvalidInformation);
   BindEventName(BcSelectRouteEventStr, BcEvent::SelectRoute);
   BindEventName(BcAuthorizeCallSetupEventStr, BcEvent::AuthorizeCallSetup);
   BindEventName(BcSendCallEventStr, BcEvent::SendCall);
   BindEventName(BcRemoteBusyEventStr, BcEvent::RemoteBusy);
   BindEventName(BcRemoteAlertingEventStr, BcEvent::RemoteAlerting);
   BindEventName(BcRemoteNoAnswerEventStr, BcEvent::RemoteNoAnswer);
   BindEventName(BcRemoteProgressEventStr, BcEvent::RemoteProgress);
   BindEventName(BcRemoteAnswerEventStr, BcEvent::RemoteAnswer);
   BindEventName(BcTerminateEventStr, BcEvent::Terminate);
   BindEventName(BcAuthorizeTerminationEventStr, BcEvent::AuthorizeTermination);
   BindEventName(BcTerminationDeniedEventStr, BcEvent::TerminationDenied);
   BindEventName(BcSelectFacilityEventStr, BcEvent::SelectFacility);
   BindEventName(BcLocalBusyEventStr, BcEvent::LocalBusy);
   BindEventName(BcPresentCallEventStr, BcEvent::PresentCall);
   BindEventName(BcFacilityFailureEventStr, BcEvent::FacilityFailure);
   BindEventName(BcLocalAlertingEventStr, BcEvent::LocalAlerting);
   BindEventName(BcLocalNoAnswerEventStr, BcEvent::LocalNoAnswer);
   BindEventName(BcLocalAnswerEventStr, BcEvent::LocalAnswer);
   BindEventName(BcLocalSuspendEventStr, BcEvent::LocalSuspend);
   BindEventName(BcLocalResumeEventStr, BcEvent::LocalResume);
   BindEventName(BcRemoteSuspendEventStr, BcEvent::RemoteSuspend);
   BindEventName(BcRemoteResumeEventStr, BcEvent::RemoteResume);
   BindEventName(BcLocalReleaseEventStr, BcEvent::LocalRelease);
   BindEventName(BcRemoteReleaseEventStr, BcEvent::RemoteRelease);
   BindEventName(BcReleaseCallEventStr, BcEvent::ReleaseCall);
   BindEventName(BcApplyTreatmentEventStr, BcEvent::ApplyTreatment);
}

//------------------------------------------------------------------------------

BcService::~BcService()
{
   Debug::ftnt("BcService.dtor");
}
}
