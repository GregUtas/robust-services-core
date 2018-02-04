//==============================================================================
//
//  BcEvents.cpp
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
#include <ostream>
#include "Debug.h"

using namespace SessionBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CallBase
{
fn_name BcEvent_ctor = "BcEvent.ctor";

BcEvent::BcEvent(Id eid, ServiceSM& owner) : Event(eid, &owner)
{
   Debug::ft(BcEvent_ctor);
}

fn_name BcEvent_dtor = "BcEvent.dtor";

BcEvent::~BcEvent()
{
   Debug::ft(BcEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcProgressEvent_ctor = "BcProgressEvent.ctor";

BcProgressEvent::BcProgressEvent
   (Id eid, ServiceSM& owner, Progress::Ind progress) : BcEvent(eid, owner),
   progress_(progress)
{
   Debug::ft(BcProgressEvent_ctor);
}

fn_name BcProgressEvent_dtor = "BcProgressEvent.dtor";

BcProgressEvent::~BcProgressEvent()
{
   Debug::ft(BcProgressEvent_dtor);
}

void BcProgressEvent::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   BcEvent::Display(stream, prefix, options);

   stream << prefix << "progress : " << int(progress_) << CRLF;
}

//------------------------------------------------------------------------------

fn_name BcReleaseEvent_ctor = "BcReleaseEvent.ctor";

BcReleaseEvent::BcReleaseEvent(Id eid, ServiceSM& owner, Cause::Ind cause) :
   BcEvent(eid, owner),
   cause_(cause)
{
   Debug::ft(BcReleaseEvent_ctor);
}

fn_name BcReleaseEvent_dtor = "BcReleaseEvent.dtor";

BcReleaseEvent::~BcReleaseEvent()
{
   Debug::ft(BcReleaseEvent_dtor);
}

void BcReleaseEvent::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   BcEvent::Display(stream, prefix, options);

   stream << prefix << "cause : " << int(cause_) << CRLF;
}

//------------------------------------------------------------------------------

fn_name BcOriginateEvent_ctor = "BcOriginateEvent.ctor";

BcOriginateEvent::BcOriginateEvent(ServiceSM& owner) :
   BcEvent(Originate, owner)
{
   Debug::ft(BcOriginateEvent_ctor);
}

fn_name BcOriginateEvent_dtor = "BcOriginateEvent.dtor";

BcOriginateEvent::~BcOriginateEvent()
{
   Debug::ft(BcOriginateEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcAuthorizeOriginationEvent_ctor = "BcAuthorizeOriginationEvent.ctor";

BcAuthorizeOriginationEvent::BcAuthorizeOriginationEvent(ServiceSM& owner) :
   BcEvent(AuthorizeOrigination, owner)
{
   Debug::ft(BcAuthorizeOriginationEvent_ctor);
}

fn_name BcAuthorizeOriginationEvent_dtor = "BcAuthorizeOriginationEvent.dtor";

BcAuthorizeOriginationEvent::~BcAuthorizeOriginationEvent()
{
   Debug::ft(BcAuthorizeOriginationEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcOriginationDeniedEvent_ctor = "BcOriginationDeniedEvent.ctor";

BcOriginationDeniedEvent::BcOriginationDeniedEvent
   (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(OriginationDenied, owner, cause)
{
   Debug::ft(BcOriginationDeniedEvent_ctor);
}

fn_name BcOriginationDeniedEvent_dtor = "BcOriginationDeniedEvent.dtor";

BcOriginationDeniedEvent::~BcOriginationDeniedEvent()
{
   Debug::ft(BcOriginationDeniedEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcCollectInformationEvent_ctor = "BcCollectInformationEvent.ctor";

BcCollectInformationEvent::BcCollectInformationEvent(ServiceSM& owner) :
   BcEvent(CollectInformation, owner)
{
   Debug::ft(BcCollectInformationEvent_ctor);
}

fn_name BcCollectInformationEvent_dtor = "BcCollectInformationEvent.dtor";

BcCollectInformationEvent::~BcCollectInformationEvent()
{
   Debug::ft(BcCollectInformationEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcCollectionTimeoutEvent_ctor = "BcCollectionTimeoutEvent.ctor";

BcCollectionTimeoutEvent::BcCollectionTimeoutEvent
   (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(CollectionTimeout, owner, cause)
{
   Debug::ft(BcCollectionTimeoutEvent_ctor);
}

fn_name BcCollectionTimeoutEvent_dtor = "BcCollectionTimeoutEvent.dtor";

BcCollectionTimeoutEvent::~BcCollectionTimeoutEvent()
{
   Debug::ft(BcCollectionTimeoutEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcLocalInformationEvent_ctor = "BcLocalInformationEvent.ctor";

BcLocalInformationEvent::BcLocalInformationEvent(ServiceSM& owner) :
   BcEvent(LocalInformation, owner)
{
   Debug::ft(BcLocalInformationEvent_ctor);
}

fn_name BcLocalInformationEvent_dtor = "BcLocalInformationEvent.dtor";

BcLocalInformationEvent::~BcLocalInformationEvent()
{
   Debug::ft(BcLocalInformationEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcAnalyzeInformationEvent_ctor = "BcAnalyzeInformationEvent.ctor";

BcAnalyzeInformationEvent::BcAnalyzeInformationEvent(ServiceSM& owner) :
   BcEvent(AnalyzeInformation, owner)
{
   Debug::ft(BcAnalyzeInformationEvent_ctor);
}

fn_name BcAnalyzeInformationEvent_dtor = "BcAnalyzeInformationEvent.dtor";

BcAnalyzeInformationEvent::~BcAnalyzeInformationEvent()
{
   Debug::ft(BcAnalyzeInformationEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcInvalidInformationEvent_ctor = "BcInvalidInformationEvent.ctor";

BcInvalidInformationEvent::BcInvalidInformationEvent
   (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(InvalidInformation, owner, cause)
{
   Debug::ft(BcInvalidInformationEvent_ctor);
}

fn_name BcInvalidInformationEvent_dtor = "BcInvalidInformationEvent.dtor";

BcInvalidInformationEvent::~BcInvalidInformationEvent()
{
   Debug::ft(BcInvalidInformationEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcSelectRouteEvent_ctor = "BcSelectRouteEvent.ctor";

BcSelectRouteEvent::BcSelectRouteEvent(ServiceSM& owner) :
   BcEvent(SelectRoute, owner)
{
   Debug::ft(BcSelectRouteEvent_ctor);
}

fn_name BcSelectRouteEvent_dtor = "BcSelectRouteEvent.dtor";

BcSelectRouteEvent::~BcSelectRouteEvent()
{
   Debug::ft(BcSelectRouteEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcAuthorizeCallSetupEvent_ctor = "BcAuthorizeCallSetupEvent.ctor";

BcAuthorizeCallSetupEvent::BcAuthorizeCallSetupEvent(ServiceSM& owner) :
   BcEvent(AuthorizeCallSetup, owner)
{
   Debug::ft(BcAuthorizeCallSetupEvent_ctor);
}

fn_name BcAuthorizeCallSetupEvent_dtor = "BcAuthorizeCallSetupEvent.dtor";

BcAuthorizeCallSetupEvent::~BcAuthorizeCallSetupEvent()
{
   Debug::ft(BcAuthorizeCallSetupEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcSendCallEvent_ctor = "BcSendCallEvent.ctor";

BcSendCallEvent::BcSendCallEvent(ServiceSM& owner) : BcEvent(SendCall, owner)
{
   Debug::ft(BcSendCallEvent_ctor);
}

fn_name BcSendCallEvent_dtor = "BcSendCallEvent.dtor";

BcSendCallEvent::~BcSendCallEvent()
{
   Debug::ft(BcSendCallEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcRemoteProgressEvent_ctor = "BcRemoteProgressEvent.ctor";

BcRemoteProgressEvent::BcRemoteProgressEvent
   (ServiceSM& owner, Progress::Ind progress) :
   BcProgressEvent(RemoteProgress, owner, progress)
{
   Debug::ft(BcRemoteProgressEvent_ctor);
}

fn_name BcRemoteProgressEvent_dtor = "BcRemoteProgressEvent.dtor";

BcRemoteProgressEvent::~BcRemoteProgressEvent()
{
   Debug::ft(BcRemoteProgressEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcRemoteBusyEvent_ctor = "BcRemoteBusyEvent.ctor";

BcRemoteBusyEvent::BcRemoteBusyEvent (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(RemoteBusy, owner, cause)
{
   Debug::ft(BcRemoteBusyEvent_ctor);
}

fn_name BcRemoteBusyEvent_dtor = "BcRemoteBusyEvent.dtor";

BcRemoteBusyEvent::~BcRemoteBusyEvent()
{
   Debug::ft(BcRemoteBusyEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcRemoteAlertingEvent_ctor = "BcRemoteAlertingEvent.ctor";

BcRemoteAlertingEvent::BcRemoteAlertingEvent(ServiceSM& owner) :
   BcEvent(RemoteAlerting, owner)
{
   Debug::ft(BcRemoteAlertingEvent_ctor);
}

fn_name BcRemoteAlertingEvent_dtor = "BcRemoteAlertingEvent.dtor";

BcRemoteAlertingEvent::~BcRemoteAlertingEvent()
{
   Debug::ft(BcRemoteAlertingEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcRemoteNoAnswerEvent_ctor = "BcRemoteNoAnswerEvent.ctor";

BcRemoteNoAnswerEvent::BcRemoteNoAnswerEvent
   (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(RemoteNoAnswer, owner, cause)
{
   Debug::ft(BcRemoteNoAnswerEvent_ctor);
}

fn_name BcRemoteNoAnswerEvent_dtor = "BcRemoteNoAnswerEvent.dtor";

BcRemoteNoAnswerEvent::~BcRemoteNoAnswerEvent()
{
   Debug::ft(BcRemoteNoAnswerEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcRemoteAnswerEvent_ctor = "BcRemoteAnswerEvent.ctor";

BcRemoteAnswerEvent::BcRemoteAnswerEvent(ServiceSM& owner) :
   BcEvent(RemoteAnswer, owner)
{
   Debug::ft(BcRemoteAnswerEvent_ctor);
}

fn_name BcRemoteAnswerEvent_dtor = "BcRemoteAnswerEvent.dtor";

BcRemoteAnswerEvent::~BcRemoteAnswerEvent()
{
   Debug::ft(BcRemoteAnswerEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcTerminateEvent_ctor = "BcTerminateEvent.ctor";

BcTerminateEvent::BcTerminateEvent(ServiceSM& owner) :
   BcEvent(Terminate, owner)
{
   Debug::ft(BcTerminateEvent_ctor);
}

fn_name BcTerminateEvent_dtor = "BcTerminateEvent.dtor";

BcTerminateEvent::~BcTerminateEvent()
{
   Debug::ft(BcTerminateEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcAuthorizeTerminationEvent_ctor = "BcAuthorizeTerminationEvent.ctor";

BcAuthorizeTerminationEvent::BcAuthorizeTerminationEvent(ServiceSM& owner) :
   BcEvent(AuthorizeTermination, owner)
{
   Debug::ft(BcAuthorizeTerminationEvent_ctor);
}

fn_name BcAuthorizeTerminationEvent_dtor = "BcAuthorizeTerminationEvent.dtor";

BcAuthorizeTerminationEvent::~BcAuthorizeTerminationEvent()
{
   Debug::ft(BcAuthorizeTerminationEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcTerminationDeniedEvent_ctor = "BcTerminationDeniedEvent.ctor";

BcTerminationDeniedEvent::BcTerminationDeniedEvent
   (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(TerminationDenied, owner, cause)
{
   Debug::ft(BcTerminationDeniedEvent_ctor);
}

fn_name BcTerminationDeniedEvent_dtor = "BcTerminationDeniedEvent.dtor";

BcTerminationDeniedEvent::~BcTerminationDeniedEvent()
{
   Debug::ft(BcTerminationDeniedEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcSelectFacilityEvent_ctor = "BcSelectFacilityEvent.ctor";

BcSelectFacilityEvent::BcSelectFacilityEvent(ServiceSM& owner) :
   BcEvent(SelectFacility, owner)
{
   Debug::ft(BcSelectFacilityEvent_ctor);
}

fn_name BcSelectFacilityEvent_dtor = "BcSelectFacilityEvent.dtor";

BcSelectFacilityEvent::~BcSelectFacilityEvent()
{
   Debug::ft(BcSelectFacilityEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcLocalBusyEvent_ctor = "BcLocalBusyEvent.ctor";

BcLocalBusyEvent::BcLocalBusyEvent(ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(LocalBusy, owner, cause)
{
   Debug::ft(BcLocalBusyEvent_ctor);
}

fn_name BcLocalBusyEvent_dtor = "BcLocalBusyEvent.dtor";

BcLocalBusyEvent::~BcLocalBusyEvent()
{
   Debug::ft(BcLocalBusyEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcPresentCallEvent_ctor = "BcPresentCallEvent.ctor";

BcPresentCallEvent::BcPresentCallEvent(ServiceSM& owner) :
   BcEvent(PresentCall, owner)
{
   Debug::ft(BcPresentCallEvent_ctor);
}

fn_name BcPresentCallEvent_dtor = "BcPresentCallEvent.dtor";

BcPresentCallEvent::~BcPresentCallEvent()
{
   Debug::ft(BcPresentCallEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcFacilityFailureEvent_ctor = "BcFacilityFailureEvent.ctor";

BcFacilityFailureEvent::BcFacilityFailureEvent
   (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(FacilityFailure, owner, cause)
{
   Debug::ft(BcFacilityFailureEvent_ctor);
}

fn_name BcFacilityFailureEvent_dtor = "BcFacilityFailureEvent.dtor";

BcFacilityFailureEvent::~BcFacilityFailureEvent()
{
   Debug::ft(BcFacilityFailureEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcLocalProgressEvent_ctor = "BcLocalProgressEvent.ctor";

BcLocalProgressEvent::BcLocalProgressEvent
   (ServiceSM& owner, Progress::Ind progress) :
   BcProgressEvent(LocalProgress, owner, progress)
{
   Debug::ft(BcLocalProgressEvent_ctor);
}

fn_name BcLocalProgressEvent_dtor = "BcLocalProgressEvent.dtor";

BcLocalProgressEvent::~BcLocalProgressEvent()
{
   Debug::ft(BcLocalProgressEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcLocalAlertingEvent_ctor = "BcLocalAlertingEvent.ctor";

BcLocalAlertingEvent::BcLocalAlertingEvent(ServiceSM& owner) :
   BcEvent(LocalAlerting, owner)
{
   Debug::ft(BcLocalAlertingEvent_ctor);
}

fn_name BcLocalAlertingEvent_dtor = "BcLocalAlertingEvent.dtor";

BcLocalAlertingEvent::~BcLocalAlertingEvent()
{
   Debug::ft(BcLocalAlertingEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcLocalNoAnswerEvent_ctor = "BcLocalNoAnswerEvent.ctor";

BcLocalNoAnswerEvent::BcLocalNoAnswerEvent(ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(LocalNoAnswer, owner, cause)
{
   Debug::ft(BcLocalNoAnswerEvent_ctor);
}

fn_name BcLocalNoAnswerEvent_dtor = "BcLocalNoAnswerEvent.dtor";

BcLocalNoAnswerEvent::~BcLocalNoAnswerEvent()
{
   Debug::ft(BcLocalNoAnswerEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcLocalAnswerEvent_ctor = "BcLocalAnswerEvent.ctor";

BcLocalAnswerEvent::BcLocalAnswerEvent(ServiceSM& owner) :
   BcEvent(LocalAnswer, owner)
{
   Debug::ft(BcLocalAnswerEvent_ctor);
}

fn_name BcLocalAnswerEvent_dtor = "BcLocalAnswerEvent.dtor";

BcLocalAnswerEvent::~BcLocalAnswerEvent()
{
   Debug::ft(BcLocalAnswerEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcLocalSuspendEvent_ctor = "BcLocalSuspendEvent.ctor";

BcLocalSuspendEvent::BcLocalSuspendEvent(ServiceSM& owner) :
   BcEvent(LocalSuspend, owner)
{
   Debug::ft(BcLocalSuspendEvent_ctor);
}

fn_name BcLocalSuspendEvent_dtor = "BcLocalSuspendEvent.dtor";

BcLocalSuspendEvent::~BcLocalSuspendEvent()
{
   Debug::ft(BcLocalSuspendEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcLocalResumeEvent_ctor = "BcLocalResumeEvent.ctor";

BcLocalResumeEvent::BcLocalResumeEvent(ServiceSM& owner) :
   BcEvent(LocalResume, owner)
{
   Debug::ft(BcLocalResumeEvent_ctor);
}

fn_name BcLocalResumeEvent_dtor = "BcLocalResumeEvent.dtor";

BcLocalResumeEvent::~BcLocalResumeEvent()
{
   Debug::ft(BcLocalResumeEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcRemoteSuspendEvent_ctor = "BcRemoteSuspendEvent.ctor";

BcRemoteSuspendEvent::BcRemoteSuspendEvent(ServiceSM& owner) :
   BcEvent(RemoteSuspend, owner)
{
   Debug::ft(BcRemoteSuspendEvent_ctor);
}

fn_name BcRemoteSuspendEvent_dtor = "BcRemoteSuspendEvent.dtor";

BcRemoteSuspendEvent::~BcRemoteSuspendEvent()
{
   Debug::ft(BcRemoteSuspendEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcRemoteResumeEvent_ctor = "BcRemoteResumeEvent.ctor";

BcRemoteResumeEvent::BcRemoteResumeEvent(ServiceSM& owner) :
   BcEvent(RemoteResume, owner)
{
   Debug::ft(BcRemoteResumeEvent_ctor);
}

fn_name BcRemoteResumeEvent_dtor = "BcRemoteResumeEvent.dtor";

BcRemoteResumeEvent::~BcRemoteResumeEvent()
{
   Debug::ft(BcRemoteResumeEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcLocalReleaseEvent_ctor = "BcLocalReleaseEvent.ctor";

BcLocalReleaseEvent::BcLocalReleaseEvent (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(LocalRelease, owner, cause)
{
   Debug::ft(BcLocalReleaseEvent_ctor);
}

fn_name BcLocalReleaseEvent_dtor = "BcLocalReleaseEvent.dtor";

BcLocalReleaseEvent::~BcLocalReleaseEvent()
{
   Debug::ft(BcLocalReleaseEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcRemoteReleaseEvent_ctor = "BcRemoteReleaseEvent.ctor";

BcRemoteReleaseEvent::BcRemoteReleaseEvent(ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(RemoteRelease, owner, cause)
{
   Debug::ft(BcRemoteReleaseEvent_ctor);
}

fn_name BcRemoteReleaseEvent_dtor = "BcRemoteReleaseEvent.dtor";

BcRemoteReleaseEvent::~BcRemoteReleaseEvent()
{
   Debug::ft(BcRemoteReleaseEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcReleaseCallEvent_ctor = "BcReleaseCallEvent.ctor";

BcReleaseCallEvent::BcReleaseCallEvent(ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(ReleaseCall, owner, cause)
{
   Debug::ft(BcReleaseCallEvent_ctor);
}

fn_name BcReleaseCallEvent_dtor = "BcReleaseCallEvent.dtor";

BcReleaseCallEvent::~BcReleaseCallEvent()
{
   Debug::ft(BcReleaseCallEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name BcApplyTreatmentEvent_ctor = "BcApplyTreatmentEvent.ctor";

BcApplyTreatmentEvent::BcApplyTreatmentEvent
   (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(ApplyTreatment, owner, cause)
{
   Debug::ft(BcApplyTreatmentEvent_ctor);
}

fn_name BcApplyTreatmentEvent_dtor = "BcApplyTreatmentEvent.dtor";

BcApplyTreatmentEvent::~BcApplyTreatmentEvent()
{
   Debug::ft(BcApplyTreatmentEvent_dtor);
}

//------------------------------------------------------------------------------
//
//b The following basic call events are not used in the POTS implementation.
//
//  BcReanalyzeInformationEvent
//  BcNetworkBusyEvent
//  BcAuthorizationDeniedEvent
//  BcRouteBusyEvent
//  BcFacilitySelectedEvent
//  BcRemoteInformationEvent
//  BcLocalInfoRequestEvent
//  BcLocalInfoReportEvent
//  BcRemoteInfoRequestEvent
//  BcRemoteInfoReportEvent
//  BcRemoteServiceEvent
//  BcLocalDisconnectEvent
//  BcDisconnectTimeoutEvent
}
