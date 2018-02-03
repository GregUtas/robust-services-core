//==============================================================================
//
//  ProxyBcSessions.h
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
#ifndef PROXYBCSESSIONS_H_INCLUDED
#define PROXYBCSESSIONS_H_INCLUDED

#include "BcSessions.h"
#include <cstddef>
#include "BcCause.h"
#include "BcProgress.h"
#include "BcProtocol.h"
#include "NbTypes.h"
#include "SbTypes.h"
#include "Trigger.h"

//------------------------------------------------------------------------------

namespace CallBase
{
//  Proxy call service.
//
//  Each concrete basic call subclass that supports proxy calls must define
//  a singleton instance.
//
//  Two examples of services that use proxy calls are call forwarding and
//  call transfer.  A proxy call is one in which a user is logically, but
//  not physically, present.  The user may have been physically present
//  earlier during the call but was subsequently removed.  The user remains
//  logically present because one of the user's services set up the call and
//  must therefore continue to relay messages between the users who remain
//  in the call.  Although it is not included in this implementation, the
//  user also remains a logical part of the call because the user would be
//  responsible for any chargeable portion of the overall call that was set
//  up by the user's services.
//
class ProxyBcService : public BcService
{
public:
   static const ServicePortId FirstPortId = BcService::NextPortId;

   static const ServicePortId ProxyPort  = FirstPortId + 0;  // proxy UPSM
   static const ServicePortId NextPortId = FirstPortId + 1;

   //  Overridden to provide a name for proxy ports.
   //
   virtual const char* PortName(PortId pid) const override;
protected:
   //  Protected because this class is virtual.  By default, basic calls
   //  support modifier services.  The constructor registers event names
   //  and event handlers that are inherited by all subclasses.
   //
   explicit ProxyBcService(Id sid, bool modifiable = true);

   //  Protected because subclasses should be singletons.
   //
   virtual ~ProxyBcService();
};

//------------------------------------------------------------------------------
//
//  Proxy call states, which are the same as those in basic calls.
//
//  Each concrete basic call subclass that supports proxy calls must define
//  a singleton instance of each state.
//
class ProxyBcNull : public BcNull
{
protected:
   explicit ProxyBcNull(ServiceId sid);
   virtual ~ProxyBcNull();
};

class ProxyBcAuthorizingOrigination : public BcAuthorizingOrigination
{
protected:
   explicit ProxyBcAuthorizingOrigination(ServiceId sid);
   virtual ~ProxyBcAuthorizingOrigination();
};

class ProxyBcCollectingInformation : public BcCollectingInformation
{
protected:
   explicit ProxyBcCollectingInformation(ServiceId sid);
   virtual ~ProxyBcCollectingInformation();
};

class ProxyBcAnalyzingInformation : public BcAnalyzingInformation
{
protected:
   explicit ProxyBcAnalyzingInformation(ServiceId sid);
   virtual ~ProxyBcAnalyzingInformation();
};

class ProxyBcSelectingRoute : public BcSelectingRoute
{
protected:
   explicit ProxyBcSelectingRoute(ServiceId sid);
   virtual ~ProxyBcSelectingRoute();
};

class ProxyBcAuthorizingCallSetup : public BcAuthorizingCallSetup
{
protected:
   explicit ProxyBcAuthorizingCallSetup(ServiceId sid);
   virtual ~ProxyBcAuthorizingCallSetup();
};

class ProxyBcSendingCall : public BcSendingCall
{
protected:
   explicit ProxyBcSendingCall(ServiceId sid);
   virtual ~ProxyBcSendingCall();
};

class ProxyBcOrigAlerting : public BcOrigAlerting
{
protected:
   explicit ProxyBcOrigAlerting(ServiceId sid);
   virtual ~ProxyBcOrigAlerting();
};

class ProxyBcAuthorizingTermination : public BcAuthorizingTermination
{
protected:
   explicit ProxyBcAuthorizingTermination(ServiceId sid);
   virtual ~ProxyBcAuthorizingTermination();
};

class ProxyBcSelectingFacility : public BcSelectingFacility
{
protected:
   explicit ProxyBcSelectingFacility(ServiceId sid);
   virtual ~ProxyBcSelectingFacility();
};

class ProxyBcPresentingCall : public BcPresentingCall
{
protected:
   explicit ProxyBcPresentingCall(ServiceId sid);
   virtual ~ProxyBcPresentingCall();
};

class ProxyBcTermAlerting : public BcTermAlerting
{
protected:
   explicit ProxyBcTermAlerting(ServiceId sid);
   virtual ~ProxyBcTermAlerting();
};

class ProxyBcActive : public BcActive
{
protected:
   explicit ProxyBcActive(ServiceId sid);
   virtual ~ProxyBcActive();
};

class ProxyBcLocalSuspending : public BcLocalSuspending
{
protected:
   explicit ProxyBcLocalSuspending(ServiceId sid);
   virtual ~ProxyBcLocalSuspending();
};

class ProxyBcRemoteSuspending : public BcRemoteSuspending
{
protected:
   explicit ProxyBcRemoteSuspending(ServiceId sid);
   virtual ~ProxyBcRemoteSuspending();
};

class ProxyBcDisconnecting : public BcDisconnecting
{
protected:
   explicit ProxyBcDisconnecting(ServiceId sid);
   virtual ~ProxyBcDisconnecting();
};

class ProxyBcException : public BcException
{
protected:
   explicit ProxyBcException(ServiceId sid);
   virtual ~ProxyBcException();
};

//------------------------------------------------------------------------------
//
//  Proxy call events.
//
class ProxyBcEvent : public BcEvent
{
public:
   static const Id FirstId = BcEvent::NextId;

   static const Id ReleaseUser   = FirstId + 0;
   static const Id ProxyProgress = FirstId + 1;
   static const Id ProxyAnswer   = FirstId + 2;
   static const Id ProxyRelease  = FirstId + 3;
   static const Id NextId        = FirstId + 4;
private:
   //  Private because this class only has static members.  Its purpose is
   //  to define event identifiers, but the events themselves are derived
   //  from various basic call events.
   //
   ProxyBcEvent();
};

//  This event is used by services such as call transfer and call forwarding
//  on no reply, in order to release the user when redirecting the call.  It
//  is supported in the basic call states SC, OA, PC, TA, AC, LS, RS, and EX.
//  CAUSE indicates why the user is being released.
//
class ProxyBcReleaseUserEvent : public BcReleaseEvent
{
public:
   ProxyBcReleaseUserEvent(ServiceSM& owner, Cause::Ind cause);
   virtual ~ProxyBcReleaseUserEvent();
};

//  This event is raised when a proxy UPSM reports a CPG and a non-proxy UPSM
//  is still present on the call.  It is used for all progress indicators.
//
class ProxyBcProgressEvent : public BcProgressEvent
{
public:
   ProxyBcProgressEvent(ServiceSM& owner, Progress::Ind progress);
   virtual ~ProxyBcProgressEvent();
};

//  This event is raised when a proxy UPSM reports an ANM and a non-proxy UPSM
//  is still present on the call.
//
class ProxyBcAnswerEvent : public BcEvent
{
public:
   explicit ProxyBcAnswerEvent(ServiceSM& owner);
   virtual ~ProxyBcAnswerEvent();
};

//  This event is raised when a proxy UPSM a REL and a non-proxy UPSM is still
//  present on the call.  It is used for all cause values.
//
class ProxyBcReleaseEvent : public BcReleaseEvent
{
public:
   ProxyBcReleaseEvent(ServiceSM& owner, Cause::Ind cause);
   virtual ~ProxyBcReleaseEvent();
};

//------------------------------------------------------------------------------
//
//  Proxy call event handlers.
//
class ProxyBcEventHandler : public BcEventHandler
{
public:
   static const Id FirstId = BcEventHandler::NextId;

   //  Subclasses must implement a ReleaseUser event handler that
   //  o continues to provide ringback if the call has reached the TA state
   //  o releases the user by sending a message on the UPSM
   //  o sets the next SNP to UserReleasedSnp (see below)
   //  o morphs the basic call SSM to a proxy basic call SSM
   //
   static const Id ReleaseUser         = FirstId + 0;

   //  If the user is released, with only proxy PSMs remaining on the call,
   //  the proxy basic call SSM may need to handle subsequent alerting and
   //  answer events.  For example, subsequent alerting occurs during Call
   //  Forwarding on No Reply, and subsequent answer occurs when more than
   //  one proxy may report answer, as in Preset Conference.
   //
   static const Id TaLocalAlerting     = FirstId + 1;
   static const Id AcLocalAnswer       = FirstId + 2;

   //  The remaining event handlers are supported here.  They handle calls
   //  in which proxy UPSMs co-exist with the user's UPSM.
   //
   static const Id AnalyzeProxyMessage = FirstId + 3;
   static const Id ProxyProgress       = FirstId + 4;
   static const Id ProxyAnswer         = FirstId + 5;
   static const Id ProxyRelease        = FirstId + 6;
   static const Id NextId              = FirstId + 7;
protected:
   ProxyBcEventHandler();
   virtual ~ProxyBcEventHandler();
};

//  When a proxy call contains only proxy UPSMs, the message analyzer for
//  AnalyzeLocalMessage is used to analyze a message from a proxy UPSM.
//  However, a separate message analyzer is used if the subscriber's UPSM
//  still exists.  This occurs when the subscriber has yet to be released
//  (for example, when redirecting a call to another destination while
//  continuing to alert the subscriber).  It can also occur if a call is
//  re-presented to the subscriber (for example, if it remains unanswered
//  after being transferred).  In such cases, messages from the subscriber's
//  UPSM and messages from proxy UPSMs are analyzed separately.  This is
//  supported in the basic call states PC, TA, AC, LS, and RS.
//
class ProxyBcAnalyzeProxyMessage : public ProxyBcEventHandler
{
   friend class Singleton< ProxyBcAnalyzeProxyMessage >;
private:
   ProxyBcAnalyzeProxyMessage();
   ~ProxyBcAnalyzeProxyMessage();
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class ProxyBcProgressHandler : public ProxyBcEventHandler
{
   friend class Singleton< ProxyBcProgressHandler >;
private:
   ProxyBcProgressHandler();
   ~ProxyBcProgressHandler();
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class ProxyBcAnswerHandler : public ProxyBcEventHandler
{
   friend class Singleton< ProxyBcAnswerHandler >;
private:
   ProxyBcAnswerHandler();
   ~ProxyBcAnswerHandler();
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class ProxyBcReleaseHandler : public ProxyBcEventHandler
{
   friend class Singleton< ProxyBcReleaseHandler >;
private:
   ProxyBcReleaseHandler();
   ~ProxyBcReleaseHandler();
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

//------------------------------------------------------------------------------
//
//  Proxy call triggers.
//
class ProxyBcTrigger : public Trigger
{
public:
   static const Id FirstId = BcTrigger::NextId;

   //  This SNP indicates that the subscriber's UPSM is being released as
   //  a result of redirecting the call.  It occurs during services such as
   //  call forwarding on no reply and call transfer.  It allows services
   //  that depend on the subscriber's UPSM to remove themselves from the
   //  call, even though the call is continuing.
   //
   static const Id UserReleasedSnp = FirstId + 0;

   //  This SAP indicates that a proxy UPSM has answered the call.  It is
   //  defined because the default behavior is to release all other UPSMs
   //  (including the subscriber) and connect the call to the proxy UPSM
   //  that answered.
   //
   static const Id ProxyAnswerSap  = FirstId + 1;

   //  This SNP indicates that a proxy UPSM has been awarded the call and
   //  that all other UPSMs are about to be released.
   //
   static const Id ProxyAnswerSnp  = FirstId + 2;
   static const Id NextId          = FirstId + 3;
private:
   //  Private because this class only has static members.  It defines
   //  trigger identifiers to which active modifier SSMs can react, but
   //  no actual triggers (for Initiators) use these identifiers as yet.
   //
   ProxyBcTrigger();
};

//------------------------------------------------------------------------------
//
//  Proxy call protocol state machine.
//
class ProxyBcPsm : public BcPsm
{
public:
   //  Creates a PSM from an adjacent layer.  The arguments are the same
   //  as those for the base class.
   //
   ProxyBcPsm(ProtocolLayer& adj, bool upper);

   //  Creates a PSM that will send an initial message.
   //
   ProxyBcPsm();

   //  Public because applications may need to delete instances.
   //  Not subclassed.
   //
   ~ProxyBcPsm();

   //  When a message is queued on a proxy PSM, the default behavior is to
   //  save it, send it, and then move it to the next proxy PSM to be sent
   //  again.  The message is therefore broadcast to all proxy PSMs on the
   //  the call, starting with the one where the message was queued.  If
   //  SetExclude(true) is invoked on a proxy PSM, however, it is omitted
   //  during broadcasting.  If there is also a message queued on such a PSM,
   //  it is only sent on that PSM instead of being broadcast.  At the end of
   //  the transaction, the flag is cleared, once again making the PSM part
   //  of the broadcast group.
   //
   void SetExclude(bool on) { exclude_ = on; }

   //  Returns true if the PSM is excluded from broadcasting.
   //
   bool IsExcluded() const { return exclude_; }

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Overridden to not start a timer when sending an IAM.
   //
   virtual bool UsesIamTimer() const override { return false; }

   //  Overridden to support broadcasting when a message is sent.
   //
   virtual OutgoingRc ProcessOgMsg(Message& msg) override;

   //  Overridden to indicate that all messages should be internal.
   //
   virtual Message::Route Route() const override;

   //  Set to remove the PSM from the broadcast group during the current
   //  transaction.
   //
   bool exclude_;
};

//------------------------------------------------------------------------------
//
//  Proxy call service state machine.
//
class ProxyBcSsm : public BcSsm
{
public:
   //  Returns the number of proxy UPSMs on the call.
   //
   size_t ProxyCount() const { return proxyCount_; }

   //  The following functions return EventHandler::Continue after creating
   //  the specified event (returned in nextEvent) and invoking SetNextSnp,
   //  SetNextState, and SetNextSap as appropriate.
   //
   virtual EventHandler::Rc RaiseReleaseUser
      (Event*& nextEvent, Cause::Ind cause);
   virtual EventHandler::Rc RaiseProxyProgress
      (Event*& nextEvent, Progress::Ind progress);
   virtual EventHandler::Rc RaiseProxyAnswer(Event*& nextEvent);
   virtual EventHandler::Rc RaiseProxyRelease
      (Event*& nextEvent, Cause::Ind cause);

   //  Creates a proxy PSM that will send an IAM to originate a call (during
   //  redirection, for example).
   //
   ProxyBcPsm* AllocOgProxy();

   //  Returns the first proxy PSM.
   //
   ProxyBcPsm* FirstProxy() const;

   //  Updates PPSM to the next proxy PSM.
   //
   void NextProxy(ProxyBcPsm*& ppsm) const;

   //  Returns the first proxy PSM that is willing to broadcast.
   //
   ProxyBcPsm* FirstBroadcast() const;

   //  Updates PPSM to the next proxy PSM that is willing to broadcast.
   //
   void NextBroadcast(ProxyBcPsm*& ppsm) const;

   //  Relays the context message to TARGET.
   //
   void Relay(BcPsm& target) const;

   //  Releases all proxy PSMs by sending them a CIP REL containing CAUSE.
   //  If SKIP is not nullptr, it is excluded when the REL is broadcast.
   //
   void ReleaseProxies(ProxyBcPsm* skip, Cause::Ind cause) const;

   //  Overridden to set proxyCount_ to 1 when registering a proxy OBC's UPSM.
   //
   virtual void SetUPsm(MediaPsm& psm) override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
protected:
   //  Protected because this class is virtual.
   //
   explicit ProxyBcSsm(ServiceId sid);

   //  Protected to restrict deletion. Virtual to allow subclassing.
   //
   virtual ~ProxyBcSsm();

   //  Overridden to return ProxyBcService::ProxyPort if the message arrived
   //  on a proxy UPSM when the subscriber's UPSM also exists.
   //
   virtual ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;

   //  Overridden to re-include any excluded proxy PSM in the next message
   //  broadcast.
   //
   virtual void EndOfTransaction() override;

   //  Overridden to handle deletion of a proxy PSM.
   //
   virtual void PsmDeleted(ProtocolSM& exPsm) override;
private:
   //  The number of proxy PSMs on the call.
   //
   size_t proxyCount_;
};

//------------------------------------------------------------------------------
//
//  Proxy call factory.
//
class ProxyBcFactory : public CipFactory
{
   friend class Singleton< ProxyBcFactory >;
private:
   //  Private because this singleton is not subclassed.
   //
   ProxyBcFactory();

   //  Private because this singleton is not subclassed.
   //
   ~ProxyBcFactory();

   //  Overridden to return a CLI parameter that identifies this factory.
   //
   virtual CliText* CreateText() const override;

   //  Overridden to create the type of root SSM associated with the
   //  RouteResult parameter in MSG, which must be an incoming CIP IAM.
   //
   virtual RootServiceSM* AllocRoot
      (const Message& msg, ProtocolSM& psm) const override;

   //  Overridden to create a ProxyBcPsm when a CIP IAM arrives to
   //  originate a new proxy call.
   //
   virtual ProtocolSM* AllocIcPsm
      (const Message& msg, ProtocolLayer& lower) const override;
};
}
#endif
