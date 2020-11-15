//==============================================================================
//
//  BcEvents.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CallBase
{
BcEvent::BcEvent(Id eid, ServiceSM& owner) : Event(eid, &owner)
{
   Debug::ft("BcEvent.ctor");
}

BcEvent::~BcEvent()
{
   Debug::ftnt("BcEvent.dtor");
}

//------------------------------------------------------------------------------

BcProgressEvent::BcProgressEvent
   (Id eid, ServiceSM& owner, Progress::Ind progress) : BcEvent(eid, owner),
   progress_(progress)
{
   Debug::ft("BcProgressEvent.ctor");
}

BcProgressEvent::~BcProgressEvent()
{
   Debug::ftnt("BcProgressEvent.dtor");
}

void BcProgressEvent::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   BcEvent::Display(stream, prefix, options);

   stream << prefix << "progress : " << int(progress_) << CRLF;
}

//------------------------------------------------------------------------------

BcReleaseEvent::BcReleaseEvent(Id eid, ServiceSM& owner, Cause::Ind cause) :
   BcEvent(eid, owner),
   cause_(cause)
{
   Debug::ft("BcReleaseEvent.ctor");
}

BcReleaseEvent::~BcReleaseEvent()
{
   Debug::ftnt("BcReleaseEvent.dtor");
}

void BcReleaseEvent::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   BcEvent::Display(stream, prefix, options);

   stream << prefix << "cause : " << int(cause_) << CRLF;
}

//------------------------------------------------------------------------------

BcOriginateEvent::BcOriginateEvent(ServiceSM& owner) :
   BcEvent(Originate, owner)
{
   Debug::ft("BcOriginateEvent.ctor");
}

BcOriginateEvent::~BcOriginateEvent()
{
   Debug::ftnt("BcOriginateEvent.dtor");
}

//------------------------------------------------------------------------------

BcAuthorizeOriginationEvent::BcAuthorizeOriginationEvent(ServiceSM& owner) :
   BcEvent(AuthorizeOrigination, owner)
{
   Debug::ft("BcAuthorizeOriginationEvent.ctor");
}

BcAuthorizeOriginationEvent::~BcAuthorizeOriginationEvent()
{
   Debug::ftnt("BcAuthorizeOriginationEvent.dtor");
}

//------------------------------------------------------------------------------

BcOriginationDeniedEvent::BcOriginationDeniedEvent
   (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(OriginationDenied, owner, cause)
{
   Debug::ft("BcOriginationDeniedEvent.ctor");
}

BcOriginationDeniedEvent::~BcOriginationDeniedEvent()
{
   Debug::ftnt("BcOriginationDeniedEvent.dtor");
}

//------------------------------------------------------------------------------

BcCollectInformationEvent::BcCollectInformationEvent(ServiceSM& owner) :
   BcEvent(CollectInformation, owner)
{
   Debug::ft("BcCollectInformationEvent.ctor");
}

BcCollectInformationEvent::~BcCollectInformationEvent()
{
   Debug::ftnt("BcCollectInformationEvent.dtor");
}

//------------------------------------------------------------------------------

BcCollectionTimeoutEvent::BcCollectionTimeoutEvent
   (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(CollectionTimeout, owner, cause)
{
   Debug::ft("BcCollectionTimeoutEvent.ctor");
}

BcCollectionTimeoutEvent::~BcCollectionTimeoutEvent()
{
   Debug::ftnt("BcCollectionTimeoutEvent.dtor");
}

//------------------------------------------------------------------------------

BcLocalInformationEvent::BcLocalInformationEvent(ServiceSM& owner) :
   BcEvent(LocalInformation, owner)
{
   Debug::ft("BcLocalInformationEvent.ctor");
}

BcLocalInformationEvent::~BcLocalInformationEvent()
{
   Debug::ftnt("BcLocalInformationEvent.dtor");
}

//------------------------------------------------------------------------------

BcAnalyzeInformationEvent::BcAnalyzeInformationEvent(ServiceSM& owner) :
   BcEvent(AnalyzeInformation, owner)
{
   Debug::ft("BcAnalyzeInformationEvent.ctor");
}

BcAnalyzeInformationEvent::~BcAnalyzeInformationEvent()
{
   Debug::ftnt("BcAnalyzeInformationEvent.dtor");
}

//------------------------------------------------------------------------------

BcInvalidInformationEvent::BcInvalidInformationEvent
   (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(InvalidInformation, owner, cause)
{
   Debug::ft("BcInvalidInformationEvent.ctor");
}

BcInvalidInformationEvent::~BcInvalidInformationEvent()
{
   Debug::ftnt("BcInvalidInformationEvent.dtor");
}

//------------------------------------------------------------------------------

BcSelectRouteEvent::BcSelectRouteEvent(ServiceSM& owner) :
   BcEvent(SelectRoute, owner)
{
   Debug::ft("BcSelectRouteEvent.ctor");
}

BcSelectRouteEvent::~BcSelectRouteEvent()
{
   Debug::ftnt("BcSelectRouteEvent.dtor");
}

//------------------------------------------------------------------------------

BcAuthorizeCallSetupEvent::BcAuthorizeCallSetupEvent(ServiceSM& owner) :
   BcEvent(AuthorizeCallSetup, owner)
{
   Debug::ft("BcAuthorizeCallSetupEvent.ctor");
}

BcAuthorizeCallSetupEvent::~BcAuthorizeCallSetupEvent()
{
   Debug::ftnt("BcAuthorizeCallSetupEvent.dtor");
}

//------------------------------------------------------------------------------

BcSendCallEvent::BcSendCallEvent(ServiceSM& owner) : BcEvent(SendCall, owner)
{
   Debug::ft("BcSendCallEvent.ctor");
}

BcSendCallEvent::~BcSendCallEvent()
{
   Debug::ftnt("BcSendCallEvent.dtor");
}

//------------------------------------------------------------------------------

BcRemoteProgressEvent::BcRemoteProgressEvent
   (ServiceSM& owner, Progress::Ind progress) :
   BcProgressEvent(RemoteProgress, owner, progress)
{
   Debug::ft("BcRemoteProgressEvent.ctor");
}

BcRemoteProgressEvent::~BcRemoteProgressEvent()
{
   Debug::ftnt("BcRemoteProgressEvent.dtor");
}

//------------------------------------------------------------------------------

BcRemoteBusyEvent::BcRemoteBusyEvent (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(RemoteBusy, owner, cause)
{
   Debug::ft("BcRemoteBusyEvent.ctor");
}

BcRemoteBusyEvent::~BcRemoteBusyEvent()
{
   Debug::ftnt("BcRemoteBusyEvent.dtor");
}

//------------------------------------------------------------------------------

BcRemoteAlertingEvent::BcRemoteAlertingEvent(ServiceSM& owner) :
   BcEvent(RemoteAlerting, owner)
{
   Debug::ft("BcRemoteAlertingEvent.ctor");
}

BcRemoteAlertingEvent::~BcRemoteAlertingEvent()
{
   Debug::ftnt("BcRemoteAlertingEvent.dtor");
}

//------------------------------------------------------------------------------

BcRemoteNoAnswerEvent::BcRemoteNoAnswerEvent
   (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(RemoteNoAnswer, owner, cause)
{
   Debug::ft("BcRemoteNoAnswerEvent.ctor");
}

BcRemoteNoAnswerEvent::~BcRemoteNoAnswerEvent()
{
   Debug::ftnt("BcRemoteNoAnswerEvent.dtor");
}

//------------------------------------------------------------------------------

BcRemoteAnswerEvent::BcRemoteAnswerEvent(ServiceSM& owner) :
   BcEvent(RemoteAnswer, owner)
{
   Debug::ft("BcRemoteAnswerEvent.ctor");
}

BcRemoteAnswerEvent::~BcRemoteAnswerEvent()
{
   Debug::ftnt("BcRemoteAnswerEvent.dtor");
}

//------------------------------------------------------------------------------

BcTerminateEvent::BcTerminateEvent(ServiceSM& owner) :
   BcEvent(Terminate, owner)
{
   Debug::ft("BcTerminateEvent.ctor");
}

BcTerminateEvent::~BcTerminateEvent()
{
   Debug::ftnt("BcTerminateEvent.dtor");
}

//------------------------------------------------------------------------------

BcAuthorizeTerminationEvent::BcAuthorizeTerminationEvent(ServiceSM& owner) :
   BcEvent(AuthorizeTermination, owner)
{
   Debug::ft("BcAuthorizeTerminationEvent.ctor");
}

BcAuthorizeTerminationEvent::~BcAuthorizeTerminationEvent()
{
   Debug::ftnt("BcAuthorizeTerminationEvent.dtor");
}

//------------------------------------------------------------------------------

BcTerminationDeniedEvent::BcTerminationDeniedEvent
   (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(TerminationDenied, owner, cause)
{
   Debug::ft("BcTerminationDeniedEvent.ctor");
}

BcTerminationDeniedEvent::~BcTerminationDeniedEvent()
{
   Debug::ftnt("BcTerminationDeniedEvent.dtor");
}

//------------------------------------------------------------------------------

BcSelectFacilityEvent::BcSelectFacilityEvent(ServiceSM& owner) :
   BcEvent(SelectFacility, owner)
{
   Debug::ft("BcSelectFacilityEvent.ctor");
}

BcSelectFacilityEvent::~BcSelectFacilityEvent()
{
   Debug::ftnt("BcSelectFacilityEvent.dtor");
}

//------------------------------------------------------------------------------

BcLocalBusyEvent::BcLocalBusyEvent(ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(LocalBusy, owner, cause)
{
   Debug::ft("BcLocalBusyEvent.ctor");
}

BcLocalBusyEvent::~BcLocalBusyEvent()
{
   Debug::ftnt("BcLocalBusyEvent.dtor");
}

//------------------------------------------------------------------------------

BcPresentCallEvent::BcPresentCallEvent(ServiceSM& owner) :
   BcEvent(PresentCall, owner)
{
   Debug::ft("BcPresentCallEvent.ctor");
}

BcPresentCallEvent::~BcPresentCallEvent()
{
   Debug::ftnt("BcPresentCallEvent.dtor");
}

//------------------------------------------------------------------------------

BcFacilityFailureEvent::BcFacilityFailureEvent
   (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(FacilityFailure, owner, cause)
{
   Debug::ft("BcFacilityFailureEvent.ctor");
}

BcFacilityFailureEvent::~BcFacilityFailureEvent()
{
   Debug::ftnt("BcFacilityFailureEvent.dtor");
}

//------------------------------------------------------------------------------

BcLocalProgressEvent::BcLocalProgressEvent
   (ServiceSM& owner, Progress::Ind progress) :
   BcProgressEvent(LocalProgress, owner, progress)
{
   Debug::ft("BcLocalProgressEvent.ctor");
}

BcLocalProgressEvent::~BcLocalProgressEvent()
{
   Debug::ftnt("BcLocalProgressEvent.dtor");
}

//------------------------------------------------------------------------------

BcLocalAlertingEvent::BcLocalAlertingEvent(ServiceSM& owner) :
   BcEvent(LocalAlerting, owner)
{
   Debug::ft("BcLocalAlertingEvent.ctor");
}

BcLocalAlertingEvent::~BcLocalAlertingEvent()
{
   Debug::ftnt("BcLocalAlertingEvent.dtor");
}

//------------------------------------------------------------------------------

BcLocalNoAnswerEvent::BcLocalNoAnswerEvent(ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(LocalNoAnswer, owner, cause)
{
   Debug::ft("BcLocalNoAnswerEvent.ctor");
}

BcLocalNoAnswerEvent::~BcLocalNoAnswerEvent()
{
   Debug::ftnt("BcLocalNoAnswerEvent.dtor");
}

//------------------------------------------------------------------------------

BcLocalAnswerEvent::BcLocalAnswerEvent(ServiceSM& owner) :
   BcEvent(LocalAnswer, owner)
{
   Debug::ft("BcLocalAnswerEvent.ctor");
}

BcLocalAnswerEvent::~BcLocalAnswerEvent()
{
   Debug::ftnt("BcLocalAnswerEvent.dtor");
}

//------------------------------------------------------------------------------

BcLocalSuspendEvent::BcLocalSuspendEvent(ServiceSM& owner) :
   BcEvent(LocalSuspend, owner)
{
   Debug::ft("BcLocalSuspendEvent.ctor");
}

BcLocalSuspendEvent::~BcLocalSuspendEvent()
{
   Debug::ftnt("BcLocalSuspendEvent.dtor");
}

//------------------------------------------------------------------------------

BcLocalResumeEvent::BcLocalResumeEvent(ServiceSM& owner) :
   BcEvent(LocalResume, owner)
{
   Debug::ft("BcLocalResumeEvent.ctor");
}

BcLocalResumeEvent::~BcLocalResumeEvent()
{
   Debug::ftnt("BcLocalResumeEvent.dtor");
}

//------------------------------------------------------------------------------

BcRemoteSuspendEvent::BcRemoteSuspendEvent(ServiceSM& owner) :
   BcEvent(RemoteSuspend, owner)
{
   Debug::ft("BcRemoteSuspendEvent.ctor");
}

BcRemoteSuspendEvent::~BcRemoteSuspendEvent()
{
   Debug::ftnt("BcRemoteSuspendEvent.dtor");
}

//------------------------------------------------------------------------------

BcRemoteResumeEvent::BcRemoteResumeEvent(ServiceSM& owner) :
   BcEvent(RemoteResume, owner)
{
   Debug::ft("BcRemoteResumeEvent.ctor");
}

BcRemoteResumeEvent::~BcRemoteResumeEvent()
{
   Debug::ftnt("BcRemoteResumeEvent.dtor");
}

//------------------------------------------------------------------------------

BcLocalReleaseEvent::BcLocalReleaseEvent (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(LocalRelease, owner, cause)
{
   Debug::ft("BcLocalReleaseEvent.ctor");
}

BcLocalReleaseEvent::~BcLocalReleaseEvent()
{
   Debug::ftnt("BcLocalReleaseEvent.dtor");
}

//------------------------------------------------------------------------------

BcRemoteReleaseEvent::BcRemoteReleaseEvent(ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(RemoteRelease, owner, cause)
{
   Debug::ft("BcRemoteReleaseEvent.ctor");
}

BcRemoteReleaseEvent::~BcRemoteReleaseEvent()
{
   Debug::ftnt("BcRemoteReleaseEvent.dtor");
}

//------------------------------------------------------------------------------

BcReleaseCallEvent::BcReleaseCallEvent(ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(ReleaseCall, owner, cause)
{
   Debug::ft("BcReleaseCallEvent.ctor");
}

BcReleaseCallEvent::~BcReleaseCallEvent()
{
   Debug::ftnt("BcReleaseCallEvent.dtor");
}

//------------------------------------------------------------------------------

BcApplyTreatmentEvent::BcApplyTreatmentEvent
   (ServiceSM& owner, Cause::Ind cause) :
   BcReleaseEvent(ApplyTreatment, owner, cause)
{
   Debug::ft("BcApplyTreatmentEvent.ctor");
}

BcApplyTreatmentEvent::~BcApplyTreatmentEvent()
{
   Debug::ftnt("BcApplyTreatmentEvent.dtor");
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
