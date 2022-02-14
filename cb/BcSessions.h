//==============================================================================
//
//  BcSessions.h
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
#ifndef BCSESSIONS_H_INCLUDED
#define BCSESSIONS_H_INCLUDED

#include "Event.h"
#include "EventHandler.h"
#include "MediaSsm.h"
#include "Service.h"
#include "SsmFactory.h"
#include "State.h"
#include "Trigger.h"
#include <iosfwd>
#include <string>
#include "BcAddress.h"
#include "BcCause.h"
#include "BcProgress.h"
#include "BcRouting.h"
#include "NbTypes.h"
#include "SbTypes.h"
#include "SysTypes.h"

namespace MediaBase
{
   class MediaPsm;
}

namespace CallBase
{
   class CipMessage;
   class CipPsm;
}

using namespace MediaBase;
using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace CallBase
{
//  Basic call service.
//
//  Each concrete basic call subclass must define a singleton instance.
//
class BcService : public Service
{
protected:
   //  Protected because this class is virtual.  By default, basic calls
   //  support modifier services.  The constructor registers event names
   //  and event handlers that are inherited by all subclasses.
   //
   explicit BcService(Id sid, bool modifiable = true);

   //  Protected because subclasses should be singletons.
   //
   virtual ~BcService();
};

//------------------------------------------------------------------------------
//
//  Basic call states.
//
//  State identifiers are defined within a pure virtual class that serves as
//  a base class for all basic call states.  This allows state identifiers
//  to appear as, for example, BcState::AuthorizingOrigination.
//
//  Basic call modifiers require a uniform call model across all basic call
//  subclasses, so subclasses should not define additional states.
//
//  Each state indicates the call model to which it applies (OBC=originating,
//  TBC=terminating, XBC=both).  It also provides a two-letter abbreviation
//  for use in class or file names.
//
class BcState : public State
{
public:
   static const Id FirstId = ServiceSM::Null;

   static const Id Null                   = FirstId + 0;   // XBC NU
   static const Id AuthorizingOrigination = FirstId + 1;   // OBC AO
   static const Id CollectingInformation  = FirstId + 2;   // OBC CI
   static const Id AnalyzingInformation   = FirstId + 3;   // OBC AI
   static const Id SelectingRoute         = FirstId + 4;   // OBC SR
   static const Id AuthorizingCallSetup   = FirstId + 5;   // OBC AS
   static const Id SendingCall            = FirstId + 6;   // OBC SC
   static const Id OrigAlerting           = FirstId + 7;   // OBC OA
   static const Id AuthorizingTermination = FirstId + 8;   // TBC AT
   static const Id SelectingFacility      = FirstId + 9;   // TBC SF
   static const Id PresentingCall         = FirstId + 10;  // TBC PC
   static const Id TermAlerting           = FirstId + 11;  // TBC TA
   static const Id Active                 = FirstId + 12;  // XBC AC
   static const Id LocalSuspending        = FirstId + 13;  // XBC LS
   static const Id RemoteSuspending       = FirstId + 14;  // XBC RS
   static const Id Disconnecting          = FirstId + 15;  // XBC DI
   static const Id Exception              = FirstId + 16;  // XBC EX
   static const Id MaxBcId                = FirstId + 16;
protected:
   //  Protected because this class is virtual.
   //
   BcState(ServiceId sid, Id stid);

   //  Protected because subclasses should be singletons.
   //
   virtual ~BcState();
};

//  Each concrete basic call subclass must define a singleton instance of
//  each state.  The constructor for each class registers the event handler
//  identifier that is appropriate for each event identifier.
//
class BcNull : public BcState
{
protected:
   explicit BcNull(ServiceId sid);
   virtual ~BcNull() = default;
};

class BcAuthorizingOrigination : public BcState
{
protected:
   explicit BcAuthorizingOrigination(ServiceId sid);
   virtual ~BcAuthorizingOrigination() = default;
};

class BcCollectingInformation : public BcState
{
protected:
   explicit BcCollectingInformation(ServiceId sid);
   virtual ~BcCollectingInformation() = default;
};

class BcAnalyzingInformation : public BcState
{
protected:
   explicit BcAnalyzingInformation(ServiceId sid);
   virtual ~BcAnalyzingInformation() = default;
};

class BcSelectingRoute : public BcState
{
protected:
   explicit BcSelectingRoute(ServiceId sid);
   virtual ~BcSelectingRoute() = default;
};

class BcAuthorizingCallSetup : public BcState
{
protected:
   explicit BcAuthorizingCallSetup(ServiceId sid);
   virtual ~BcAuthorizingCallSetup() = default;
};

class BcSendingCall : public BcState
{
protected:
   explicit BcSendingCall(ServiceId sid);
   virtual ~BcSendingCall() = default;
};

class BcOrigAlerting : public BcState
{
protected:
   explicit BcOrigAlerting(ServiceId sid);
   virtual ~BcOrigAlerting() = default;
};

class BcAuthorizingTermination : public BcState
{
protected:
   explicit BcAuthorizingTermination(ServiceId sid);
   virtual ~BcAuthorizingTermination() = default;
};

class BcSelectingFacility : public BcState
{
protected:
   explicit BcSelectingFacility(ServiceId sid);
   virtual ~BcSelectingFacility() = default;
};

class BcPresentingCall : public BcState
{
protected:
   explicit BcPresentingCall(ServiceId sid);
   virtual ~BcPresentingCall() = default;
};

class BcTermAlerting : public BcState
{
protected:
   explicit BcTermAlerting(ServiceId sid);
   virtual ~BcTermAlerting() = default;
};

class BcActive : public BcState
{
protected:
   explicit BcActive(ServiceId sid);
   virtual ~BcActive() = default;
};

class BcLocalSuspending : public BcState
{
protected:
   explicit BcLocalSuspending(ServiceId sid);
   virtual ~BcLocalSuspending() = default;
};

class BcRemoteSuspending : public BcState
{
protected:
   explicit BcRemoteSuspending(ServiceId sid);
   virtual ~BcRemoteSuspending() = default;
};

class BcDisconnecting : public BcState
{
protected:
   explicit BcDisconnecting(ServiceId sid);
   virtual ~BcDisconnecting() = default;
};

class BcException : public BcState
{
protected:
   explicit BcException(ServiceId sid);
   virtual ~BcException() = default;
};

//------------------------------------------------------------------------------
//
//  Basic call events.
//
//  Event identifiers are defined within a pure virtual class that serves as
//  a base class for all basic call events.  This allows event identifiers to
//  appear as, for example, BcEvent::AuthorizeOrigination.
//
//  Each event specifies the call model to which it applies.
//  Identifiers defined by subclasses must start at BcEvent::NextId.
//b Identifiers not used in the POTS implementation are commented out.
//
class BcEvent : public Event
{
public:
   static const Id FirstId = Event::NextId;

   static const Id Originate            = FirstId + 0;   // OBC
   static const Id AuthorizeOrigination = FirstId + 1;   // OBC
   static const Id OriginationDenied    = FirstId + 2;   // OBC
   static const Id CollectInformation   = FirstId + 3;   // OBC
   static const Id CollectionTimeout    = FirstId + 4;   // OBC
   static const Id LocalInformation     = FirstId + 5;   // OBC
   static const Id AnalyzeInformation   = FirstId + 6;   // OBC
   static const Id InvalidInformation   = FirstId + 7;   // OBC
   static const Id SelectRoute          = FirstId + 8;   // OBC
// static const Id ReanalyzeInformation = FirstId + 9;   // OBC
// static const Id NetworkBusy          = FirstId + 10;  // OBC
   static const Id AuthorizeCallSetup   = FirstId + 11;  // OBC
// static const Id AuthorizationDenied  = FirstId + 12;  // OBC
   static const Id SendCall             = FirstId + 13;  // OBC
// static const Id RouteBusy            = FirstId + 14;  // OBC
   static const Id RemoteProgress       = FirstId + 15;  // OBC
   static const Id RemoteBusy           = FirstId + 16;  // OBC
   static const Id RemoteAlerting       = FirstId + 17;  // OBC
   static const Id RemoteNoAnswer       = FirstId + 18;  // OBC
   static const Id RemoteAnswer         = FirstId + 19;  // OBC
   static const Id Terminate            = FirstId + 20;  // TBC
   static const Id AuthorizeTermination = FirstId + 21;  // TBC
   static const Id TerminationDenied    = FirstId + 22;  // TBC
   static const Id SelectFacility       = FirstId + 23;  // TBC
// static const Id FacilitySelected     = FirstId + 24;  // TBC
   static const Id LocalBusy            = FirstId + 25;  // TBC
   static const Id PresentCall          = FirstId + 26;  // TBC
// static const Id RemoteInformation    = FirstId + 27;  // TBC
   static const Id FacilityFailure      = FirstId + 28;  // TBC
   static const Id LocalProgress        = FirstId + 29;  // TBC
   static const Id LocalAlerting        = FirstId + 30;  // TBC
   static const Id LocalNoAnswer        = FirstId + 31;  // TBC
   static const Id LocalAnswer          = FirstId + 32;  // TBC
// static const Id LocalInfoRequest     = FirstId + 33;  // XBC
// static const Id LocalInfoReport      = FirstId + 34;  // XBC
// static const Id RemoteInfoRequest    = FirstId + 35;  // XBC
// static const Id RemoteInfoReport     = FirstId + 36;  // XBC
   static const Id LocalSuspend         = FirstId + 37;  // XBC
   static const Id LocalResume          = FirstId + 38;  // XBC
   static const Id RemoteSuspend        = FirstId + 39;  // XBC
   static const Id RemoteResume         = FirstId + 40;  // XBC
// static const Id RemoteService        = FirstId + 41;  // XBC
// static const Id LocalDisconnect      = FirstId + 42;  // XBC
   static const Id LocalRelease         = FirstId + 43;  // XBC
   static const Id RemoteRelease        = FirstId + 44;  // XBC
// static const Id DisconnectTimeout    = FirstId + 45;  // XBC
   static const Id ReleaseCall          = FirstId + 46;  // XBC
   static const Id ApplyTreatment       = FirstId + 47;  // XBC

   static const Id NextId = FirstId + 50;

   //  Virtual to allow subclassing.
   //
   virtual ~BcEvent();
protected:
   //  Protected because this class is virtual.
   //
   BcEvent(Id eid, ServiceSM& owner);
};

//  BcProgressEvent is a common base class for all events associated with
//  call progress.  It contains a progress indicator.
//
class BcProgressEvent : public BcEvent
{
public:
   virtual ~BcProgressEvent();
   Progress::Ind GetProgress() const { return progress_; }
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   BcProgressEvent(Id eid, ServiceSM& owner, Progress::Ind progress);
private:
   Progress::Ind progress_;
};

//  BcReleaseEvent is a common base class for all events associated with
//  call takedown.  It provides the reason why the call is being released.
//
class BcReleaseEvent : public BcEvent
{
public:
   virtual ~BcReleaseEvent();
   Cause::Ind GetCause() const { return cause_; }
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   BcReleaseEvent(Id eid, ServiceSM& owner, Cause::Ind cause);
private:
   Cause::Ind cause_;
};

//  The remaining events are concrete and would rarely be subclassed.
//b Events not used in the POTS implementation are commented out.

class BcOriginateEvent : public BcEvent
{
public:
   explicit BcOriginateEvent(ServiceSM& owner);
   virtual ~BcOriginateEvent();
};

class BcAuthorizeOriginationEvent : public BcEvent
{
public:
   explicit BcAuthorizeOriginationEvent(ServiceSM& owner);
   virtual ~BcAuthorizeOriginationEvent();
};

class BcOriginationDeniedEvent : public BcReleaseEvent
{
public:
   BcOriginationDeniedEvent(ServiceSM& owner, Cause::Ind cause);
   virtual ~BcOriginationDeniedEvent();
};

class BcCollectInformationEvent : public BcEvent
{
public:
   explicit BcCollectInformationEvent(ServiceSM& owner);
   virtual ~BcCollectInformationEvent();
};

class BcCollectionTimeoutEvent : public BcReleaseEvent
{
public:
   BcCollectionTimeoutEvent(ServiceSM& owner, Cause::Ind cause);
   virtual ~BcCollectionTimeoutEvent();
};

class BcLocalInformationEvent : public BcEvent
{
public:
   explicit BcLocalInformationEvent(ServiceSM& owner);
   virtual ~BcLocalInformationEvent();
};

class BcAnalyzeInformationEvent : public BcEvent
{
public:
   explicit BcAnalyzeInformationEvent(ServiceSM& owner);
   virtual ~BcAnalyzeInformationEvent();
};

class BcInvalidInformationEvent : public BcReleaseEvent
{
public:
   BcInvalidInformationEvent(ServiceSM& owner, Cause::Ind cause);
   virtual ~BcInvalidInformationEvent();
};

class BcSelectRouteEvent : public BcEvent
{
public:
   explicit BcSelectRouteEvent(ServiceSM& owner);
   virtual ~BcSelectRouteEvent();
};

// class BcReanalyzeInformationEvent : public BcEvent
// {
// public:
//    explicit BcReanalyzeInformationEvent(ServiceSM& owner);
//    virtual ~BcReanalyzeInformationEvent();
// };

// class BcNetworkBusyEvent : public BcReleaseEvent
// {
// public:
//    BcNetworkBusyEvent(ServiceSM& owner, Cause::Ind cause);
//    virtual ~BcNetworkBusyEvent();
// };

class BcAuthorizeCallSetupEvent : public BcEvent
{
public:
   explicit BcAuthorizeCallSetupEvent(ServiceSM& owner);
   virtual ~BcAuthorizeCallSetupEvent();
};

// class BcAuthorizationDeniedEvent : public BcReleaseEvent
// {
// public:
//    BcAuthorizationDeniedEvent(ServiceSM& owner, Cause::Ind cause);
//    virtual ~BcAuthorizationDeniedEvent();
// };

class BcSendCallEvent : public BcEvent
{
public:
   explicit BcSendCallEvent(ServiceSM& owner);
   virtual ~BcSendCallEvent();
};

// class BcRouteBusyEvent : public BcReleaseEvent
// {
// public:
//    BcRouteBusyEvent(ServiceSM& owner, Cause::Ind cause);
//    virtual ~BcRouteBusyEvent();
// };

class BcRemoteBusyEvent : public BcReleaseEvent
{
public:
   BcRemoteBusyEvent(ServiceSM& owner, Cause::Ind cause);
   virtual ~BcRemoteBusyEvent();
};

class BcRemoteAlertingEvent : public BcEvent
{
public:
   explicit BcRemoteAlertingEvent(ServiceSM& owner);
   virtual ~BcRemoteAlertingEvent();
};

class BcRemoteNoAnswerEvent : public BcReleaseEvent
{
public:
   BcRemoteNoAnswerEvent(ServiceSM& owner, Cause::Ind cause);
   virtual ~BcRemoteNoAnswerEvent();
};

class BcRemoteProgressEvent : public BcProgressEvent
{
public:
   BcRemoteProgressEvent(ServiceSM& owner, Progress::Ind progress);
   virtual ~BcRemoteProgressEvent();
};

class BcRemoteAnswerEvent : public BcEvent
{
public:
   explicit BcRemoteAnswerEvent(ServiceSM& owner);
   virtual ~BcRemoteAnswerEvent();
};

class BcTerminateEvent : public BcEvent
{
public:
   explicit BcTerminateEvent(ServiceSM& owner);
   virtual ~BcTerminateEvent();
};

class BcAuthorizeTerminationEvent : public BcEvent
{
public:
   explicit BcAuthorizeTerminationEvent(ServiceSM& owner);
   virtual ~BcAuthorizeTerminationEvent();
};

class BcTerminationDeniedEvent : public BcReleaseEvent
{
public:
   BcTerminationDeniedEvent(ServiceSM& owner, Cause::Ind cause);
   virtual ~BcTerminationDeniedEvent();
};

class BcSelectFacilityEvent : public BcEvent
{
public:
   explicit BcSelectFacilityEvent(ServiceSM& owner);
   virtual ~BcSelectFacilityEvent();
};

// class BcFacilitySelectedEvent : public BcEvent
// {
// public:
//    BcFacilitySelectedEvent(ServiceSM& owner);
//    virtual ~BcFacilitySelectedEvent();
// };

class BcLocalBusyEvent : public BcReleaseEvent
{
public:
   BcLocalBusyEvent(ServiceSM& owner, Cause::Ind cause);
   virtual ~BcLocalBusyEvent();
};

class BcPresentCallEvent : public BcEvent
{
public:
   explicit BcPresentCallEvent(ServiceSM& owner);
   virtual ~BcPresentCallEvent();
};

// class BcRemoteInformationEvent : public BcEvent
// {
// public:
//    BcRemoteInformationEvent(ServiceSM& owner);
//    virtual ~BcRemoteInformationEvent();
// };

class BcFacilityFailureEvent : public BcReleaseEvent
{
public:
   BcFacilityFailureEvent(ServiceSM& owner, Cause::Ind cause);
   virtual ~BcFacilityFailureEvent();
};

class BcLocalProgressEvent : public BcProgressEvent
{
public:
   BcLocalProgressEvent(ServiceSM& owner, Progress::Ind progress);
   virtual ~BcLocalProgressEvent();
};

class BcLocalAlertingEvent : public BcEvent
{
public:
   explicit BcLocalAlertingEvent(ServiceSM& owner);
   virtual ~BcLocalAlertingEvent();
};

class BcLocalNoAnswerEvent : public BcReleaseEvent
{
public:
   BcLocalNoAnswerEvent(ServiceSM& owner, Cause::Ind cause);
   virtual ~BcLocalNoAnswerEvent();
};

class BcLocalAnswerEvent : public BcEvent
{
public:
   explicit BcLocalAnswerEvent(ServiceSM& owner);
   virtual ~BcLocalAnswerEvent();
};

// class BcLocalInfoRequestEvent : public BcEvent
// {
// public:
//    BcLocalInfoRequestEvent(ServiceSM& owner);
//    virtual ~BcLocalInfoRequestEvent();
// };

// class BcLocalInfoReportEvent : public BcEvent
// {
// public:
//    explicit BcLocalInfoReportEvent(ServiceSM& owner);
//    virtual ~BcLocalInfoReportEvent();
// };

// class BcRemoteInfoRequestEvent : public BcEvent
// {
// public:
//    explicit BcRemoteInfoRequestEvent(ServiceSM& owner);
//    virtual ~BcRemoteInfoRequestEvent();
// };

// class BcRemoteInfoReportEvent : public BcEvent
// {
// public:
//    explicit BcRemoteInfoReportEvent(ServiceSM& owner);
//    virtual ~BcRemoteInfoReportEvent();
// };

class BcLocalSuspendEvent : public BcEvent
{
public:
   explicit BcLocalSuspendEvent(ServiceSM& owner);
   virtual ~BcLocalSuspendEvent();
};

class BcLocalResumeEvent : public BcEvent
{
public:
   explicit BcLocalResumeEvent(ServiceSM& owner);
   virtual ~BcLocalResumeEvent();
};

class BcRemoteSuspendEvent : public BcEvent
{
public:
   explicit BcRemoteSuspendEvent(ServiceSM& owner);
   virtual ~BcRemoteSuspendEvent();
};

class BcRemoteResumeEvent : public BcEvent
{
public:
   explicit BcRemoteResumeEvent(ServiceSM& owner);
   virtual ~BcRemoteResumeEvent();
};

// class BcRemoteServiceEvent : public BcEvent
// {
// public:
//    explicit BcRemoteServiceEvent(ServiceSM& owner);
//    virtual ~BcRemoteServiceEvent();
// };

// class BcLocalDisconnectEvent : public BcReleaseEvent
// {
// public:
//    BcLocalDisconnectEvent(ServiceSM& owner, Cause::Ind cause);
//    virtual ~BcLocalDisconnectEvent();
// };

class BcLocalReleaseEvent : public BcReleaseEvent
{
public:
   BcLocalReleaseEvent(ServiceSM& owner, Cause::Ind cause);
   virtual ~BcLocalReleaseEvent();
};

class BcRemoteReleaseEvent : public BcReleaseEvent
{
public:
   BcRemoteReleaseEvent(ServiceSM& owner, Cause::Ind cause);
   virtual ~BcRemoteReleaseEvent();
};

// class BcDisconnectTimeoutEvent : public BcReleaseEvent
// {
// public:
//    BcDisconnectTimeoutEvent(ServiceSM& owner, Cause::Ind cause);
//    virtual ~BcDisconnectTimeoutEvent();
// };

class BcReleaseCallEvent : public BcReleaseEvent
{
public:
   BcReleaseCallEvent(ServiceSM& owner, Cause::Ind cause);
   virtual ~BcReleaseCallEvent();
};

class BcApplyTreatmentEvent : public BcReleaseEvent
{
public:
   BcApplyTreatmentEvent(ServiceSM& owner, Cause::Ind cause);
   virtual ~BcApplyTreatmentEvent();
};

//------------------------------------------------------------------------------
//
//  Basic call event handlers.
//
//  These are the standard state-event combinations for the basic call model.
//  The names are formed by "state abbreviation + event name".
//
//  If a subclass reuses or subclasses a basic call state (defined above),
//  these event handler identifiers are automatically registered against
//  the appropriate event identifiers when the subclass registers its state.
//
//  Each identifier specifies the call model to which it applies.
//  Identifiers defined by subclasses must start at BcEventHandler::NextId.
//b Identifiers not used in the POTS implementation are commented out.
//
class BcEventHandler : public EventHandler
{
public:
   static const Id FirstNu = EventHandler::NextId;

   static const Id NuAnalyzeLocalMessage  = FirstNu + 0;   // OBC
   static const Id NuAnalyzeRemoteMessage = FirstNu + 1;   // TBC
   static const Id NuOriginate            = FirstNu + 2;   // OBC
   static const Id NuTerminate            = FirstNu + 3;   // TBC
   static const Id NuReleaseCall          = FirstNu + 4;   // XBC
   static const Id FirstAo                = FirstNu + 5;

   static const Id AoAnalyzeLocalMessage  = FirstAo + 0;   // OBC
   static const Id AoAuthorizeOrigination = FirstAo + 1;   // OBC
   static const Id AoOriginationDenied    = FirstAo + 2;   // OBC
// static const Id AoLocalDisconnect      = FirstAo + 3;   // OBC
   static const Id AoLocalRelease         = FirstAo + 4;   // OBC
   static const Id AoReleaseCall          = FirstAo + 5;   // OBC
   static const Id FirstCi                = FirstAo + 6;

   static const Id CiAnalyzeLocalMessage  = FirstCi + 0;   // OBC
   static const Id CiCollectInformation   = FirstCi + 1;   // OBC
   static const Id CiCollectionTimeout    = FirstCi + 2;   // OBC
   static const Id CiLocalInformation     = FirstCi + 3;   // OBC
// static const Id CiLocalDisconnect      = FirstCi + 4;   // OBC
   static const Id CiLocalRelease         = FirstCi + 5;   // OBC
   static const Id CiReleaseCall          = FirstCi + 6;   // OBC
   static const Id FirstAi                = FirstCi + 7;

   static const Id AiAnalyzeLocalMessage  = FirstAi + 0;   // OBC
   static const Id AiAnalyzeInformation   = FirstAi + 1;   // OBC
   static const Id AiInvalidInformation   = FirstAi + 2;   // OBC
// static const Id AiReanalyzeInformation = FirstAi + 3;   // OBC
// static const Id AiLocalDisconnect      = FirstAi + 4;   // OBC
   static const Id AiLocalRelease         = FirstAi + 5;   // OBC
   static const Id AiReleaseCall          = FirstAi + 6;   // OBC
   static const Id FirstSr                = FirstAi + 7;

   static const Id SrAnalyzeLocalMessage  = FirstSr + 0;   // OBC
   static const Id SrSelectRoute          = FirstSr + 1;   // OBC
// static const Id SrReanalyzeInformation = FirstSr + 2;   // OBC
// static const Id SrNetworkBusy          = FirstSr + 3;   // OBC
// static const Id SrLocalDisconnect      = FirstSr + 4;   // OBC
   static const Id SrLocalRelease         = FirstSr + 5;   // OBC
   static const Id SrReleaseCall          = FirstSr + 6;   // OBC
   static const Id FirstAs                = FirstSr + 7;

   static const Id AsAnalyzeLocalMessage  = FirstAs + 0;   // OBC
   static const Id AsAuthorizeCallSetup   = FirstAs + 1;   // OBC
// static const Id AsAuthorizationDenied  = FirstAs + 2;   // OBC
// static const Id AsLocalDisconnect      = FirstAs + 3;   // OBC
   static const Id AsLocalRelease         = FirstAs + 4;   // OBC
   static const Id AsReleaseCall          = FirstAs + 5;   // OBC
   static const Id FirstSc                = FirstAs + 6;

   static const Id ScAnalyzeLocalMessage  = FirstSc + 0;   // OBC
   static const Id ScAnalyzeRemoteMessage = FirstSc + 1;   // OBC
   static const Id ScSendCall             = FirstSc + 2;   // OBC
// static const Id ScRouteBusy            = FirstSc + 3;   // OBC
// static const Id ScLocalInformation     = FirstSc + 4;   // OBC
   static const Id ScRemoteProgress       = FirstSc + 5;   // OBC
   static const Id ScRemoteAlerting       = FirstSc + 6;   // OBC
   static const Id ScRemoteAnswer         = FirstSc + 7;   // OBC
   static const Id ScRemoteBusy           = FirstSc + 8;   // OBC
// static const Id ScRemoteNoAnswer       = FirstSc + 9;   // OBC
   static const Id ScRemoteRelease        = FirstSc + 10;  // OBC
// static const Id ScLocalDisconnect      = FirstSc + 11;  // OBC
   static const Id ScLocalRelease         = FirstSc + 12;  // OBC
   static const Id ScReleaseCall          = FirstSc + 13;  // OBC
   static const Id FirstOa                = FirstSc + 14;

   static const Id OaAnalyzeLocalMessage  = FirstOa + 0;   // OBC
   static const Id OaAnalyzeRemoteMessage = FirstOa + 1;   // OBC
   static const Id OaRemoteAnswer         = FirstOa + 2;   // OBC
   static const Id OaRemoteNoAnswer       = FirstOa + 3;   // OBC
   static const Id OaRemoteRelease        = FirstOa + 4;   // OBC
// static const Id OaLocalDisconnect      = FirstOa + 5;   // OBC
   static const Id OaLocalRelease         = FirstOa + 6;   // OBC
   static const Id OaReleaseCall          = FirstOa + 7;   // OBC
   static const Id FirstAt                = FirstOa + 8;

   static const Id AtAnalyzeRemoteMessage = FirstAt + 0;   // TBC
   static const Id AtAuthorizeTermination = FirstAt + 1;   // TBC
   static const Id AtTerminationDenied    = FirstAt + 2;   // TBC
// static const Id AtRemoteInformation    = FirstAt + 3;   // TBC
   static const Id AtRemoteRelease        = FirstAt + 4;   // TBC
   static const Id AtReleaseCall          = FirstAt + 5;   // TBC
   static const Id FirstSf                = FirstAt + 6;

   static const Id SfAnalyzeLocalMessage  = FirstSf + 0;   // TBC
   static const Id SfAnalyzeRemoteMessage = FirstSf + 1;   // TBC
   static const Id SfSelectFacility       = FirstSf + 2;   // TBC
// static const Id SfFacilityFailure      = FirstSf + 3;   // TBC
   static const Id SfLocalBusy            = FirstSf + 4;   // TBC
// static const Id SfRemoteInformation    = FirstSf + 5;   // TBC
   static const Id SfRemoteRelease        = FirstSf + 6;   // TBC
   static const Id SfReleaseCall          = FirstSf + 7;   // TBC
   static const Id FirstPc                = FirstSf + 8;

   static const Id PcAnalyzeLocalMessage  = FirstPc + 0;   // TBC
   static const Id PcAnalyzeRemoteMessage = FirstPc + 1;   // TBC
   static const Id PcPresentCall          = FirstPc + 2;   // TBC
   static const Id PcFacilityFailure      = FirstPc + 3;   // TBC
   static const Id PcLocalProgress        = FirstPc + 4;   // TBC
   static const Id PcLocalAlerting        = FirstPc + 5;   // TBC
   static const Id PcLocalAnswer          = FirstPc + 6;   // TBC
// static const Id PcLocalNoAnswer        = FirstPc + 7;   // TBC
   static const Id PcLocalRelease         = FirstPc + 8;   // TBC
// static const Id PcRemoteInformation    = FirstPc + 9;   // TBC
   static const Id PcRemoteRelease        = FirstPc + 10;  // TBC
   static const Id PcReleaseCall          = FirstPc + 11;  // TBC
   static const Id FirstTa                = FirstPc + 12;

   static const Id TaAnalyzeLocalMessage  = FirstTa + 0;   // TBC
   static const Id TaAnalyzeRemoteMessage = FirstTa + 1;   // TBC
   static const Id TaLocalAnswer          = FirstTa + 2;   // TBC
   static const Id TaLocalNoAnswer        = FirstTa + 3;   // TBC
   static const Id TaLocalRelease         = FirstTa + 4;   // TBC
   static const Id TaRemoteRelease        = FirstTa + 5;   // TBC
   static const Id TaReleaseCall          = FirstTa + 6;   // TBC
   static const Id FirstAc                = FirstTa + 7;

   static const Id AcAnalyzeLocalMessage  = FirstAc + 0;   // XBC
   static const Id AcAnalyzeRemoteMessage = FirstAc + 1;   // XBC
   static const Id AcLocalSuspend         = FirstAc + 2;   // XBC
// static const Id AcLocalDisconnect      = FirstAc + 3;   // XBC
   static const Id AcLocalRelease         = FirstAc + 4;   // XBC
   static const Id AcRemoteSuspend        = FirstAc + 5;   // XBC
   static const Id AcRemoteRelease        = FirstAc + 6;   // XBC
   static const Id AcReleaseCall          = FirstAc + 7;   // XBC
   static const Id FirstLs                = FirstAc + 8;

   static const Id LsAnalyzeLocalMessage  = FirstLs + 0;   // XBC
   static const Id LsAnalyzeRemoteMessage = FirstLs + 1;   // XBC
   static const Id LsLocalResume          = FirstLs + 2;   // XBC
   static const Id LsLocalRelease         = FirstLs + 3;   // XBC
   static const Id LsRemoteRelease        = FirstLs + 4;   // XBC
   static const Id LsReleaseCall          = FirstLs + 5;   // XBC
   static const Id FirstRs                = FirstLs + 6;

   static const Id RsAnalyzeLocalMessage  = FirstRs + 0;   // XBC
   static const Id RsAnalyzeRemoteMessage = FirstRs + 1;   // XBC
// static const Id RsLocalDisconnect      = FirstRs + 2;   // XBC
   static const Id RsLocalRelease         = FirstRs + 3;   // XBC
   static const Id RsRemoteResume         = FirstRs + 4;   // XBC
   static const Id RsRemoteRelease        = FirstRs + 5;   // XBC
   static const Id RsReleaseCall          = FirstRs + 6;   // XBC
   static const Id FirstDi                = FirstRs + 7;

// static const Id DiAnalyzeLocalMessage  = FirstDi + 0;   // XBC
// static const Id DiLocalRelease         = FirstDi + 1;   // XBC
// static const Id DiDisconnectTimeout    = FirstDi + 2;   // XBC
// static const Id DiReleaseCall          = FirstDi + 3;   // XBC
   static const Id FirstEx                = FirstDi + 4;

   static const Id ExAnalyzeLocalMessage  = FirstEx + 0;   // XBC
   static const Id ExApplyTreatment       = FirstEx + 1;   // XBC
// static const Id ExLocalDisconnect      = FirstEx + 2;   // XBC
   static const Id ExLocalRelease         = FirstEx + 3;   // XBC
   static const Id ExReleaseCall          = FirstEx + 4;   // XBC
   static const Id FirstUn                = FirstEx + 5;

// static const Id LocalProgress          = FirstUn + 0;   // XBC except PC
// static const Id RemoteProgress         = FirstUn + 1;   // XBC except SC
// static const Id LocalInfoRequest       = FirstUn + 2;   // XBC
// static const Id LocalInfoReport        = FirstUn + 3;   // XBC
// static const Id RemoteInfoRequest      = FirstUn + 4;   // XBC
// static const Id RemoteInfoReport       = FirstUn + 5;   // XBC
// static const Id RemoteService          = FirstUn + 6;   // XBC

   static const Id NextId                 = FirstUn + 7;
protected:
   BcEventHandler() = default;
   virtual ~BcEventHandler() = default;
};

//------------------------------------------------------------------------------
//
//  Basic call triggers.
//
//  Trigger identifiers are defined within a pure virtual class that serves
//  as a base class for all basic call triggers.  This allows identifiers
//  to appear as, for example, BcTrigger::AuthorizeTerminationSap.
//
//  Each concrete basic call subclass must define a singleton instance of
//  each trigger* that is required by a modifier's Initiator* and register
//  it against its service.
//
//  Each identifier specifies the call model to which it applies.
//  Identifiers defined by subclasses must start at BcTrigger::NextId.
//b Identifiers not used in the POTS implementation are commented out.
//
class BcTrigger : public Trigger
{
public:
   static const Id FirstId = NIL_ID + 1;

   static const Id OriginateSnp            = FirstId + 0;   // OBC
   static const Id AuthorizeOriginationSap = FirstId + 1;   // OBC
   static const Id OriginationDeniedSap    = FirstId + 2;   // OBC
   static const Id OriginatedSnp           = FirstId + 3;   // OBC
   static const Id CollectInformationSap   = FirstId + 4;   // OBC
   static const Id CollectionTimeoutSap    = FirstId + 5;   // OBC
   static const Id LocalInformationSap     = FirstId + 6;   // OBC
   static const Id InformationCollectedSnp = FirstId + 7;   // OBC
   static const Id AnalyzeInformationSap   = FirstId + 8;   // OBC
// static const Id ReanalyzeInformationSap = FirstId + 9;   // OBC
   static const Id InvalidInformationSap   = FirstId + 10;  // OBC
   static const Id InformationAnalyzedSnp  = FirstId + 11;  // OBC
   static const Id SelectRouteSap          = FirstId + 12;  // OBC
// static const Id NetworkBusySap          = FirstId + 13;  // OBC
   static const Id RouteSelectedSnp        = FirstId + 14;  // OBC
   static const Id AuthorizeCallSetupSap   = FirstId + 15;  // OBC
// static const Id AuthorizationDeniedSap  = FirstId + 16;  // OBC
   static const Id CallSetupAuthorizedSnp  = FirstId + 17;  // OBC
   static const Id SendCallSap             = FirstId + 18;  // OBC
   static const Id SendCallSnp             = FirstId + 19;  // OBC
// static const Id RouteBusySap            = FirstId + 20;  // OBC
   static const Id RemoteBusySap           = FirstId + 21;  // OBC
   static const Id RemoteProgressSnp       = FirstId + 22;  // XBC
   static const Id RemoteAlertingSnp       = FirstId + 23;  // OBC
   static const Id RemoteNoAnswerSap       = FirstId + 24;  // OBC
   static const Id RemoteAnswerSnp         = FirstId + 25;  // OBC
   static const Id TerminateSnp            = FirstId + 26;  // TBC
   static const Id AuthorizeTerminationSap = FirstId + 27;  // TBC
   static const Id TerminationDeniedSap    = FirstId + 28;  // TBC
   static const Id TerminatedSnp           = FirstId + 29;  // TBC
   static const Id SelectFacilitySap       = FirstId + 30;  // TBC
   static const Id FacilitySelectedSnp     = FirstId + 31;  // TBC
   static const Id LocalBusySap            = FirstId + 32;  // TBC
   static const Id PresentCallSap          = FirstId + 33;  // TBC
   static const Id PresentCallSnp          = FirstId + 34;  // TBC
   static const Id FacilityFailureSap      = FirstId + 35;  // TBC
   static const Id LocalProgressSnp        = FirstId + 36;  // TBC
   static const Id LocalAlertingSnp        = FirstId + 37;  // TBC
   static const Id LocalNoAnswerSap        = FirstId + 38;  // TBC
   static const Id LocalAnswerSap          = FirstId + 39;  // TBC
   static const Id LocalAnswerSnp          = FirstId + 40;  // TBC
// static const Id LocalDisconnectSap      = FirstId + 41;  // XBC
// static const Id LocalDisconnectSnp      = FirstId + 42;  // XBC
   static const Id LocalReleaseSap         = FirstId + 43;  // XBC
   static const Id LocalReleaseSnp         = FirstId + 44;  // XBC
   static const Id RemoteReleaseSap        = FirstId + 45;  // XBC
   static const Id RemoteReleaseSnp        = FirstId + 46;  // XBC
   static const Id ReleaseCallSap          = FirstId + 47;  // XBC
   static const Id ApplyTreatmentSap       = FirstId + 48;  // XBC
   static const Id CallClearedSnp          = FirstId + 49;  // XBC

   static const Id NextId                  = FirstId + 52;
protected:
   explicit BcTrigger(Id tid);
   virtual ~BcTrigger();
};

//------------------------------------------------------------------------------
//
//  Basic call service state machine.
//
class BcSsm : public MediaSsm
{
public:
   enum Model
   {
      XbcModel,  // unspecified call model
      ObcModel,  // originating call model
      TbcModel   // terminating call model
   };

   //  Returns the call model.
   //
   Model GetModel() const { return model_; }

   //  Sets the call model.
   //
   void SetModel(Model model);

   //  Returns the CIP PSM.
   //
   CipPsm* NPsm() const { return nPsm_; }

   //  Returns the user-side PSM.
   //
   MediaPsm* UPsm() const { return uPsm_; }

   //  Returns the digits dialed thus far.
   //
   const DigitString& DialedDigits() const { return dialed_; }
   DigitString& DialedDigits() { return dialed_; }

   //  Returns the outcome of analyzing the dialed digits.
   //
   const AnalysisResult& GetAnalysis() const { return analysis_; }

   //  The following functions return EventHandler::Continue after creating
   //  the specified event (returned in nextEvent) and invoking SetNextSnp,
   //  SetNextState, and SetNextSap as appropriate.
   //
   virtual EventHandler::Rc RaiseAuthorizeOrigination(Event*& nextEvent);
   virtual EventHandler::Rc RaiseCollectInformation(Event*& nextEvent);
   virtual EventHandler::Rc RaiseLocalInformation(Event*& nextEvent);
   virtual EventHandler::Rc RaiseCollectionTimeout(Event*& nextEvent);
   virtual EventHandler::Rc RaiseAnalyzeInformation(Event*& nextEvent);
   virtual EventHandler::Rc RaiseInvalidInformation(Event*& nextEvent);
   virtual EventHandler::Rc RaiseSelectRoute(Event*& nextEvent);
   virtual EventHandler::Rc RaiseAuthorizeCallSetup(Event*& nextEvent);
   virtual EventHandler::Rc RaiseSendCall(Event*& nextEvent);
   virtual EventHandler::Rc RaiseRemoteBusy(Event*& nextEvent);
   virtual EventHandler::Rc RaiseRemoteProgress
      (Event*& nextEvent, Progress::Ind progress);
   virtual EventHandler::Rc RaiseRemoteAlerting(Event*& nextEvent);
   virtual EventHandler::Rc RaiseRemoteNoAnswer(Event*& nextEvent);
   virtual EventHandler::Rc RaiseRemoteAnswer(Event*& nextEvent);
   virtual EventHandler::Rc RaiseAuthorizeTermination(Event*& nextEvent);
   virtual EventHandler::Rc RaiseSelectFacility(Event*& nextEvent);
   virtual EventHandler::Rc RaisePresentCall(Event*& nextEvent);
   virtual EventHandler::Rc RaiseLocalBusy(Event*& nextEvent);
   virtual EventHandler::Rc RaiseFacilityFailure(Event*& nextEvent);
   virtual EventHandler::Rc RaiseLocalProgress
      (Event*& nextEvent, Progress::Ind progress);
   virtual EventHandler::Rc RaiseLocalAlerting(Event*& nextEvent);
   virtual EventHandler::Rc RaiseLocalAnswer(Event*& nextEvent);
   virtual EventHandler::Rc RaiseLocalNoAnswer(Event*& nextEvent);
   virtual EventHandler::Rc RaiseLocalSuspend(Event*& nextEvent);
   virtual EventHandler::Rc RaiseLocalResume(Event*& nextEvent);
   virtual EventHandler::Rc RaiseRemoteSuspend(Event*& nextEvent);
   virtual EventHandler::Rc RaiseRemoteResume(Event*& nextEvent);

   //  The following functions are similar to the ones defined above but
   //  also place CAUSE in the created event.  For RaiseApplyTreatment,
   //  the cause only has to be specified the first time a treatment is
   //  applied, with Cause::NilInd used to advance to the next treatment.
   //
   virtual EventHandler::Rc RaiseLocalRelease
      (Event*& nextEvent, Cause::Ind cause);
   virtual EventHandler::Rc RaiseRemoteRelease
      (Event*& nextEvent, Cause::Ind cause);
   virtual EventHandler::Rc RaiseReleaseCall
      (Event*& nextEvent, Cause::Ind cause);
   virtual EventHandler::Rc RaiseApplyTreatment
      (Event*& nextEvent, Cause::Ind cause);

   //  Invoked to handle the Analyze Information event.  Analyzes the dialed
   //  digits and sets the next event accordingly.  The possible outcomes are
   //  Select Route, Initiation Request, or Invalid Information.
   //
   virtual EventHandler::Rc AnalyzeInformation(Event*& nextEvent);

   //  If the analysis result indicates that a service code was dialed, sets
   //  nextEvent to an Initiation Request for the corresponding service.
   //
   virtual EventHandler::Rc RequestService(Event*& nextEvent);

   //  Invoked to handle the Select Route event.  The analysis result should
   //  indicate that a call should be set up to a destination, in which case
   //  nextEvent is set to Authorize Call Setup.  If the destination does not
   //  exist, or if no destination was set, nextEvent will be Call Takedown.
   //
   virtual EventHandler::Rc SelectRoute(Event*& nextEvent);

   //  Must be overridden by a subclass that runs a timer on the CIP PSM.  If
   //  a timeout message (MSG) arrives, the message analyzers defined by this
   //  SSM invoke this function so that the subclass can set nextEvent.
   //
   virtual EventHandler::Rc AnalyzeNPsmTimeout
      (const TlvMessage& msg, Event*& nextEvent);

   //  Performs actions associated with a Local Alerting event.  These include
   //  sending a CIP CPG(Alerting), applying ringback tone, and setting the
   //  next state and SNP.
   //
   virtual EventHandler::Rc HandleLocalAlerting();

   //  Performs actions associated with a Local Answer event.  These include
   //  sending a CIP ANM, connecting media, and setting the next state and SNP.
   //
   virtual EventHandler::Rc HandleLocalAnswer();

   //  Invokes ClearCall with the cause specified by the Remote Release event.
   //
   virtual EventHandler::Rc HandleRemoteRelease(Event& currEvent);

   //  Sends a CIP REL if the CIP PSM exists and is not idle.  Also sets the
   //  next state and SNP.  Should be overridden by subclasses to invoke the
   //  base class version and then send a call takedown message on the UPSM
   //  if it exists and is not idle.
   //
   virtual EventHandler::Rc ClearCall(Cause::Ind cause);

   //  Invoked in the Send Call state to build a CIP IAM.  May be overridden
   //  by subclasses that need to send more parameters, but the base class
   //  version should be invoked.
   //
   virtual CipMessage* BuildCipIam();

   //  Invoked to build a CIP CPG with the specified progress indicator.  May
   //  be overridden by subclasses that need to send more parameters, but the
   //  base class version should be invoked.
   //
   virtual CipMessage* BuildCipCpg(Progress::Ind progress);

   //  Displays the number of calls in each state.
   //
   static void DisplayStateCounts
      (std::ostream& stream, const std::string& prefix);

   //  Resets the number of calls in each state during a restart.
   //
   static void ResetStateCounts(RestartLevel level);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to raise BcReleaseCallEvent when a protocol error occurs.
   //
   Event* RaiseProtocolError(ProtocolSM& psm, ProtocolSM::Error err) override;

   //  Overridden to track the number of calls in each state.
   //
   void SetNextState(StateId stid) override;
protected:
   //  Protected because this class is virtual.
   //
   explicit BcSsm(ServiceId sid);

   //  Protected to restrict deletion.  Virtual to allow subclassing.
   //
   virtual ~BcSsm();

   //  Allocates the CIP PSM.
   //
   CipPsm* AllocNPsm();

   //  Sets the CIP PSM.
   //
   void SetNPsm(CipPsm& psm);

   //  Sets the user-side PSM.
   //
   virtual void SetUPsm(MediaPsm& psm);

   //  Overridden to return Service::NetworkPort if the message arrived on
   //  the CIP PSM, and Service::UserPort if it arrived on the UPSM.  Must
   //  be overridden by a subclass that uses any other PSM.
   //
   ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;

   //  Overridden to handle deletion of the CIP PSM.
   //
   void PsmDeleted(const ProtocolSM& exPsm) override;
private:
   //  Builds a CIP REL containing CAUSE.
   //
   CipMessage* BuildCipRel(Cause::Ind cause);

   //  Whether the call model is OBC or TBC.
   //
   Model model_;

   //  The user-side PSM.
   //
   MediaPsm* uPsm_;

   //  The CIP PSM.
   //
   CipPsm* nPsm_;

   //  The digits dialed by the subscriber.
   //
   DigitString dialed_;

   //  The outcome of analyzing the dialed digits.
   //
   AnalysisResult analysis_;

   //  The destination associated with the analysis result.
   //
   RouteResult route_;
};

//------------------------------------------------------------------------------
//
//  Basic call factory.
//
//  Subclassed by each concrete basic call subclass, primarily to create its
//  messages and SSMs (AllocIcMsg and AllocRoot).
//
class BcFactory : public SsmFactory
{
public:
   //  Returns Cause::NilInd if RID is valid.  Must be overridden by factories
   //  that can appear in a RouteResult, because the default version returns
   //  Cause::ExchangeRoutingError.
   //
   virtual Cause::Ind VerifyRoute(RouteResult::Id rid) const;
protected:
   //  Protected because this class is virtual.
   //
   BcFactory(Id fid, ProtocolId prid, c_string name);

   //  Protected because subclasses should be singletons.
   //
   virtual ~BcFactory();
};

//------------------------------------------------------------------------------
//
//  Concrete basic call event handlers.
//
//  Although subclasses provide most event handlers, the base class provides
//  message analyzers for CIP.
//
class BcNuAnalyzeRemoteMessage : public BcEventHandler
{
   friend class Singleton< BcNuAnalyzeRemoteMessage >;

   BcNuAnalyzeRemoteMessage() = default;
   ~BcNuAnalyzeRemoteMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class BcScAnalyzeRemoteMessage : public BcEventHandler
{
   friend class Singleton< BcScAnalyzeRemoteMessage >;

   BcScAnalyzeRemoteMessage() = default;
   ~BcScAnalyzeRemoteMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class BcOaAnalyzeRemoteMessage : public BcEventHandler
{
   friend class Singleton< BcOaAnalyzeRemoteMessage >;

   BcOaAnalyzeRemoteMessage() = default;
   ~BcOaAnalyzeRemoteMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class BcPcAnalyzeRemoteMessage : public BcEventHandler
{
   friend class Singleton< BcPcAnalyzeRemoteMessage >;

   BcPcAnalyzeRemoteMessage() = default;
   ~BcPcAnalyzeRemoteMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class BcAcAnalyzeRemoteMessage : public BcEventHandler
{
   friend class Singleton< BcAcAnalyzeRemoteMessage >;

   BcAcAnalyzeRemoteMessage() = default;
   ~BcAcAnalyzeRemoteMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

//------------------------------------------------------------------------------
//
//  Basic call test factory.
//
//  This provides injection and verification of CIP messages to and from basic
//  call subclasses.
//
class TestCallFactory : public BcFactory
{
   friend class Singleton< TestCallFactory >;

   //  Private because this is a singleton.
   //
   TestCallFactory();

   //  Private because this is a singleton.
   //
   ~TestCallFactory();

   //  Overridden to create a CIP PSM when a CIP IAM arrives.
   //
   ProtocolSM* AllocIcPsm
      (const Message& msg, ProtocolLayer& lower) const override;

   //  Overridden to create the root SSM when MSG arrives on PSM to create a
   //  new test session.
   //
   RootServiceSM* AllocRoot(const Message& msg, ProtocolSM& psm) const override;

   //  Overridden to return Cause::NilInd when a call is routed to a test DN.
   //
   Cause::Ind VerifyRoute(RouteResult::Id rid) const override;
};
}
#endif
