//==============================================================================
//
//  PotsCwmService.cpp
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
#include "PotsCwtService.h"
#include "Event.h"
#include "EventHandler.h"
#include "ServiceSM.h"
#include "State.h"
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "BcCause.h"
#include "Context.h"
#include "Debug.h"
#include "GlobalAddress.h"
#include "IpPortRegistry.h"
#include "LocalAddress.h"
#include "NwTypes.h"
#include "PotsCircuit.h"
#include "PotsMultiplexer.h"
#include "PotsProfile.h"
#include "SbAppIds.h"
#include "SbEvents.h"
#include "SbTypes.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "TimerProtocol.h"
#include "TlvParameter.h"
#include "Tones.h"

using namespace NetworkBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCwmState : public State
{
public:
   static const Id FCWMS = ServiceSM::Null;

   static const Id Null       = FCWMS + 0;
   static const Id Initiating = FCWMS + 1;
   static const Id Active     = FCWMS + 2;
protected:
   explicit PotsCwmState(Id stid);
   virtual ~PotsCwmState();
};

class PotsCwmNull : public PotsCwmState
{
   friend class Singleton< PotsCwmNull >;

   PotsCwmNull();
   ~PotsCwmNull() = default;
};

class PotsCwmInitiating : public PotsCwmState
{
   friend class Singleton< PotsCwmInitiating >;

   PotsCwmInitiating();
   ~PotsCwmInitiating() = default;
};

class PotsCwmActive : public PotsCwmState
{
   friend class Singleton< PotsCwmActive >;

   PotsCwmActive();
   ~PotsCwmActive() = default;
};

class PotsCwmEvent : public Event
{
public:
   static const Id Facility        = NextId + 0;
   static const Id Notify          = NextId + 1;
   static const Id ToneTimeout     = NextId + 2;
   static const Id Renotify        = NextId + 3;
   static const Id Flipflop        = NextId + 4;
   static const Id Reanswer        = NextId + 5;
   static const Id ReanswerTimeout = NextId + 6;
   static const Id Reconnect       = NextId + 7;
   static const Id LocalRelease    = NextId + 8;
   static const Id ActiveRelease   = NextId + 9;
   static const Id InactiveRelease = NextId + 10;
   static const Id Quiesce         = NextId + 11;
   static const Id Relay           = NextId + 12;
   virtual ~PotsCwmEvent();
protected:
   PotsCwmEvent(Id eid, ServiceSM& owner);
};

class PotsCwmFacilityEvent : public PotsCwmEvent
{
public:
   explicit PotsCwmFacilityEvent(ServiceSM& owner);
   ~PotsCwmFacilityEvent();
};

class PotsCwmNotifyEvent : public PotsCwmEvent
{
public:
   explicit PotsCwmNotifyEvent(ServiceSM& owner);
   ~PotsCwmNotifyEvent();
};

class PotsCwmToneTimeoutEvent : public PotsCwmEvent
{
public:
   explicit PotsCwmToneTimeoutEvent(ServiceSM& owner);
   ~PotsCwmToneTimeoutEvent();
};

class PotsCwmRenotifyEvent : public PotsCwmEvent
{
public:
   explicit PotsCwmRenotifyEvent(ServiceSM& owner);
   ~PotsCwmRenotifyEvent();
};

class PotsCwmFlipflopEvent : public PotsCwmEvent
{
public:
   explicit PotsCwmFlipflopEvent(ServiceSM& owner);
   ~PotsCwmFlipflopEvent();
};

class PotsCwmReanswerEvent : public PotsCwmEvent
{
public:
   explicit PotsCwmReanswerEvent(ServiceSM& owner);
   ~PotsCwmReanswerEvent();
};

class PotsCwmReanswerTimeoutEvent : public PotsCwmEvent
{
public:
   explicit PotsCwmReanswerTimeoutEvent(ServiceSM& owner);
   ~PotsCwmReanswerTimeoutEvent();
};

class PotsCwmReconnectEvent : public PotsCwmEvent
{
public:
   explicit PotsCwmReconnectEvent(ServiceSM& owner);
   ~PotsCwmReconnectEvent();
};

class PotsCwmLocalReleaseEvent : public PotsCwmEvent
{
public:
   explicit PotsCwmLocalReleaseEvent(ServiceSM& owner);
   ~PotsCwmLocalReleaseEvent();
};

class PotsCwmRemoteReleaseEvent : public PotsCwmEvent
{
public:
   virtual ~PotsCwmRemoteReleaseEvent();
   Cause::Ind GetCause() const { return cause_; }
   void Display(ostream& stream,
      const string& prefix, const Flags& options) const override;
protected:
   PotsCwmRemoteReleaseEvent(Id eid, ServiceSM& owner, Cause::Ind cause);
private:
   Cause::Ind cause_;
};

class PotsCwmActiveReleaseEvent : public PotsCwmRemoteReleaseEvent
{
public:
   PotsCwmActiveReleaseEvent(ServiceSM& owner, Cause::Ind cause);
   ~PotsCwmActiveReleaseEvent();
};

class PotsCwmInactiveReleaseEvent : public PotsCwmRemoteReleaseEvent
{
public:
   PotsCwmInactiveReleaseEvent(ServiceSM& owner, Cause::Ind cause);
   ~PotsCwmInactiveReleaseEvent();
};

class PotsCwmQuiesceEvent : public PotsCwmEvent
{
public:
   explicit PotsCwmQuiesceEvent(ServiceSM& owner);
   ~PotsCwmQuiesceEvent();
};

class PotsCwmRelayEvent : public PotsCwmEvent
{
public:
   explicit PotsCwmRelayEvent(ServiceSM& owner);
   ~PotsCwmRelayEvent();
};

class PotsCwmEventHandler : public EventHandler
{
public:
   static const Id InAnalyzeNetworkMessage = NextId + 0;
   static const Id InFacility              = NextId + 1;
   static const Id AcAnalyzeUserMessage    = NextId + 2;
   static const Id AcAnalyzeNetworkMessage = NextId + 3;
   static const Id AcNotify                = NextId + 4;
   static const Id AcToneTimeout           = NextId + 5;
   static const Id AcRenotify              = NextId + 6;
   static const Id AcFlipflop              = NextId + 7;
   static const Id AcReanswer              = NextId + 8;
   static const Id AcReanswerTimeout       = NextId + 9;
   static const Id AcReconnect             = NextId + 10;
   static const Id AcLocalRelease          = NextId + 11;
   static const Id AcActiveRelease         = NextId + 12;
   static const Id AcInactiveRelease       = NextId + 13;
   static const Id AcQuiesce               = NextId + 14;
   static const Id AcRelay                 = NextId + 15;
protected:
   PotsCwmEventHandler() = default;
   virtual ~PotsCwmEventHandler() = default;
};

class PotsCwmInAnalyzeNetworkMessage : public PotsCwmEventHandler
{
   friend class Singleton< PotsCwmInAnalyzeNetworkMessage >;

   PotsCwmInAnalyzeNetworkMessage() = default;
   ~PotsCwmInAnalyzeNetworkMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwmInFacility : public PotsCwmEventHandler
{
   friend class Singleton< PotsCwmInFacility >;

   PotsCwmInFacility() = default;
   ~PotsCwmInFacility() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwmAcAnalyzeUserMessage : public PotsCwmEventHandler
{
   friend class Singleton< PotsCwmAcAnalyzeUserMessage >;

   PotsCwmAcAnalyzeUserMessage() = default;
   ~PotsCwmAcAnalyzeUserMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwmAcAnalyzeNetworkMessage : public PotsCwmEventHandler
{
   friend class Singleton< PotsCwmAcAnalyzeNetworkMessage >;

   PotsCwmAcAnalyzeNetworkMessage() = default;
   ~PotsCwmAcAnalyzeNetworkMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwmAcNotify : public PotsCwmEventHandler
{
   friend class Singleton< PotsCwmAcNotify >;

   PotsCwmAcNotify() = default;
   ~PotsCwmAcNotify() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwmAcToneTimeout : public PotsCwmEventHandler
{
   friend class Singleton< PotsCwmAcToneTimeout >;

   PotsCwmAcToneTimeout() = default;
   ~PotsCwmAcToneTimeout() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwmAcRenotify : public PotsCwmEventHandler
{
   friend class Singleton< PotsCwmAcRenotify >;

   PotsCwmAcRenotify() = default;
   ~PotsCwmAcRenotify() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwmAcFlipflop : public PotsCwmEventHandler
{
   friend class Singleton< PotsCwmAcFlipflop >;

   PotsCwmAcFlipflop() = default;
   ~PotsCwmAcFlipflop() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwmAcReanswer : public PotsCwmEventHandler
{
   friend class Singleton< PotsCwmAcReanswer >;

   PotsCwmAcReanswer() = default;
   ~PotsCwmAcReanswer() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwmAcReanswerTimeout : public PotsCwmEventHandler
{
   friend class Singleton< PotsCwmAcReanswerTimeout >;

   PotsCwmAcReanswerTimeout() = default;
   ~PotsCwmAcReanswerTimeout() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwmAcReconnect : public PotsCwmEventHandler
{
   friend class Singleton< PotsCwmAcReconnect >;

   PotsCwmAcReconnect() = default;
   ~PotsCwmAcReconnect() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwmAcLocalRelease : public PotsCwmEventHandler
{
   friend class Singleton< PotsCwmAcLocalRelease >;

   PotsCwmAcLocalRelease() = default;
   ~PotsCwmAcLocalRelease() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwmAcActiveRelease : public PotsCwmEventHandler
{
   friend class Singleton< PotsCwmAcActiveRelease >;

   PotsCwmAcActiveRelease() = default;
   ~PotsCwmAcActiveRelease() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwmAcInactiveRelease : public PotsCwmEventHandler
{
   friend class Singleton< PotsCwmAcInactiveRelease >;

   PotsCwmAcInactiveRelease() = default;
   ~PotsCwmAcInactiveRelease() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwmAcQuiesce : public PotsCwmEventHandler
{
   friend class Singleton< PotsCwmAcQuiesce >;

   PotsCwmAcQuiesce() = default;
   ~PotsCwmAcQuiesce() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwmAcRelay : public PotsCwmEventHandler
{
   friend class Singleton< PotsCwmAcRelay >;

   PotsCwmAcRelay() = default;
   ~PotsCwmAcRelay() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwmSsm : public ServiceSM
{
public:
   enum PsmRole
   {
      User,      // subscriber (UPSM)
      Active,    // connected (NPSM)
      Inactive,  // unanswered or held (NPSM)
      Transient  // was sent a Facility Nack (third NPSM)
   };

   static const uint32_t ToneTimeout = 6;        // (was 1; changed for tests)
   static const uint32_t RenotifyTimeout = 6;    // (was 10; changed for tests)
   static const uint32_t ReconnectTimeout = 10;
   static const uint32_t ReanswerTimeout = 30;

   static const TimerId ToneTimeoutId = 1;
   static const TimerId RenotifyTimeoutId = 2;
   static const TimerId ReconnectTimeoutId = 3;
   static const TimerId ReanswerTimeoutId = 4;

   enum Substate    // States marked '*' are transient, waiting for
   {                // a specific message from one of the two calls.
      Initiating,   // expecting active call to send Facility Ack/Nack*
      Denying,      // expecting waiting call to send Release*
      Pending,      // expecting waiting call to apply ringing*
      Notifying,    // applying first burst of CWT tone
      Notified,     // waiting to apply second burst of CWT tone
      Renotifying,  // applying second burst of CWT tone
      Renotified,   // waiting call unanswered
      Releasing,    // expecting waiting call to send Release*
      Answering,    // expecting waiting call to stop ringing*
      Answered,     // two calls: waiting call answered
      Ringing,      // one call: unanswered and ringing
      Holding,      // one call: answered and on hold
      Reringing     // one call: answered and reringing CWT subscriber
   };

   PotsCwmSsm();
   ~PotsCwmSsm();

   size_t CountCalls() const { return Mux()->CountCalls(); }
   void ClearTimer(TimerId tid);

   PsmRole CalcRole(const ProtocolSM* psm) const;
   Substate GetSubstate() const { return substate_; }
   bool IsOnhook() const { return onhook_; }
   EventHandler::Rc RelayFacilityMsg();
   EventHandler::Rc RelayMsg();
   EventHandler::Rc ReleaseActive(Cause::Ind cause, Event*& nextEvent);
   EventHandler::Rc ReleaseInactive(Cause::Ind cause, Event*& nextEvent);
   EventHandler::Rc StartCwtTone();
   EventHandler::Rc StopCwtTone();
   EventHandler::Rc Flipflop();
   EventHandler::Rc Reconnect();
   EventHandler::Rc Rering();
   EventHandler::Rc Reanswer();
   EventHandler::Rc StopReringing();
   void ReleaseCwt(Facility::Ind ind);
   void Display(ostream& stream,
      const string& prefix, const Flags& options) const override;
   void SetNextState(StateId stid) override;
private:
   PotsMuxSsm* Mux() const { return static_cast< PotsMuxSsm* >(Parent()); }
   PotsCallPsm* UPsm() const { return Mux()->UPsm(); }
   PotsMuxPsm* NPsm(PotsMuxSsm::CallId cid) const { return Mux()->NPsm(cid); }
   void StartTimer(TimerId tid, uint32_t secs);
   void StopTimer(TimerId tid);
   PotsMuxPsm* CreateNPsm() const;
   PotsMuxPsm* OtherNPsm(const ProtocolSM* npsm) const;
   static EventHandler::Rc SendFacilityNack(PotsMuxPsm* npsm, ServiceId sid);
   void SetSubstate(Substate state);
   void ConnectInactiveCall(Tone::Id tone);
   ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;
   EventHandler::Rc ProcessInitAck
      (Event& currEvent, Event*& nextEvent) override;
   EventHandler::Rc ProcessInitNack
      (Event& currEvent, Event*& nextEvent) override;
   EventHandler::Rc ProcessSip(Event& currEvent, Event*& nextEvent) override;

   Substate substate_;
   PotsMuxSsm::CallId activeCall_;
   TimerId tid_;
   bool onhook_;
};

//==============================================================================

fixed_string PotsCwmFacilityEventStr        = "PotsCwmFacilityEvent";
fixed_string PotsCwmNotifyEventStr          = "PotsCwmNotifyEvent";
fixed_string PotsCwmToneTimeoutEventStr     = "PotsCwmToneTimeoutEvent";
fixed_string PotsCwmRenotifyEventStr        = "PotsCwmRenotifyEvent";
fixed_string PotsCwmFlipflopEventStr        = "PotsCwmFlipflopEvent";
fixed_string PotsCwmReanswerEventStr        = "PotsCwmReanswerEvent";
fixed_string PotsCwmReanswerTimeoutEventStr = "PotsCwmReanswerTimeoutEvent";
fixed_string PotsCwmReconnectEventStr       = "PotsCwmReconnectEvent";
fixed_string PotsCwmLocalReleaseEventStr    = "PotsCwmLocalReleaseEvent";
fixed_string PotsCwmActiveReleaseEventStr   = "PotsCwmActiveReleaseEvent";
fixed_string PotsCwmInactiveReleaseEventStr = "PotsCwmInactiveReleaseEvent";
fixed_string PotsCwmQuiesceEventStr         = "PotsCwmQuiesceEvent";
fixed_string PotsCwmRelayEventStr           = "PotsCwmRelayEvent";

//------------------------------------------------------------------------------

PotsCwmService::PotsCwmService() : Service(PotsCwmServiceId, true)
{
   Debug::ft("PotsCwmService.ctor");

   Singleton< PotsCwmNull >::Instance();
   Singleton< PotsCwmInitiating >::Instance();
   Singleton< PotsCwmActive >::Instance();

   BindHandler(*Singleton< PotsCwmInAnalyzeNetworkMessage >::Instance(),
      PotsCwmEventHandler::InAnalyzeNetworkMessage);
   BindHandler(*Singleton< PotsCwmInFacility >::Instance(),
      PotsCwmEventHandler::InFacility);

   BindHandler(*Singleton< PotsCwmAcAnalyzeUserMessage >::Instance(),
      PotsCwmEventHandler::AcAnalyzeUserMessage);
   BindHandler(*Singleton< PotsCwmAcAnalyzeNetworkMessage >::Instance(),
      PotsCwmEventHandler::AcAnalyzeNetworkMessage);
   BindHandler(*Singleton< PotsCwmAcNotify >::Instance(),
      PotsCwmEventHandler::AcNotify);
   BindHandler(*Singleton< PotsCwmAcToneTimeout >::Instance(),
      PotsCwmEventHandler::AcToneTimeout);
   BindHandler(*Singleton< PotsCwmAcRenotify >::Instance(),
      PotsCwmEventHandler::AcRenotify);
   BindHandler(*Singleton< PotsCwmAcFlipflop >::Instance(),
      PotsCwmEventHandler::AcFlipflop);
   BindHandler(*Singleton< PotsCwmAcReanswer >::Instance(),
      PotsCwmEventHandler::AcReanswer);
   BindHandler(*Singleton< PotsCwmAcReanswerTimeout >::Instance(),
      PotsCwmEventHandler::AcReanswerTimeout);
   BindHandler(*Singleton< PotsCwmAcReconnect >::Instance(),
      PotsCwmEventHandler::AcReconnect);
   BindHandler(*Singleton< PotsCwmAcLocalRelease >::Instance(),
      PotsCwmEventHandler::AcLocalRelease);
   BindHandler(*Singleton< PotsCwmAcActiveRelease >::Instance(),
      PotsCwmEventHandler::AcActiveRelease);
   BindHandler(*Singleton< PotsCwmAcInactiveRelease >::Instance(),
      PotsCwmEventHandler::AcInactiveRelease);
   BindHandler(*Singleton< PotsCwmAcQuiesce >::Instance(),
      PotsCwmEventHandler::AcQuiesce);
   BindHandler(*Singleton< PotsCwmAcRelay >::Instance(),
      PotsCwmEventHandler::AcRelay);

   BindEventName(PotsCwmFacilityEventStr, PotsCwmEvent::Facility);
   BindEventName(PotsCwmNotifyEventStr, PotsCwmEvent::Notify);
   BindEventName(PotsCwmToneTimeoutEventStr, PotsCwmEvent::ToneTimeout);
   BindEventName(PotsCwmRenotifyEventStr, PotsCwmEvent::Renotify);
   BindEventName(PotsCwmFlipflopEventStr, PotsCwmEvent::Flipflop);
   BindEventName(PotsCwmReanswerEventStr, PotsCwmEvent::Reanswer);
   BindEventName(PotsCwmReanswerTimeoutEventStr, PotsCwmEvent::ReanswerTimeout);
   BindEventName(PotsCwmReconnectEventStr, PotsCwmEvent::Reconnect);
   BindEventName(PotsCwmLocalReleaseEventStr, PotsCwmEvent::LocalRelease);
   BindEventName(PotsCwmActiveReleaseEventStr, PotsCwmEvent::ActiveRelease);
   BindEventName(PotsCwmInactiveReleaseEventStr, PotsCwmEvent::InactiveRelease);
   BindEventName(PotsCwmQuiesceEventStr, PotsCwmEvent::Quiesce);
   BindEventName(PotsCwmRelayEventStr, PotsCwmEvent::Relay);
}

//------------------------------------------------------------------------------

PotsCwmService::~PotsCwmService()
{
   Debug::ftnt("PotsCwmService.dtor");
}

//------------------------------------------------------------------------------

ServiceSM* PotsCwmService::AllocModifier() const
{
   Debug::ft("PotsCwmService.AllocModifier");

   return new PotsCwmSsm;
}

//==============================================================================

PotsCwmState::PotsCwmState(Id stid) : State(PotsCwmServiceId, stid)
{
   Debug::ft("PotsCwmState.ctor");
}

//------------------------------------------------------------------------------

PotsCwmState::~PotsCwmState()
{
   Debug::ftnt("PotsCwmState.dtor");
}

//------------------------------------------------------------------------------

PotsCwmNull::PotsCwmNull() : PotsCwmState(PotsCwmState::Null)
{
   Debug::ft("PotsCwmNull.ctor");
}

//------------------------------------------------------------------------------

PotsCwmInitiating::PotsCwmInitiating() : PotsCwmState(PotsCwmState::Initiating)
{
   Debug::ft("PotsCwmInitiating.ctor");

   BindMsgAnalyzer
      (PotsCwmEventHandler::InAnalyzeNetworkMessage, Service::NetworkPort);
   BindEventHandler
      (PotsCwmEventHandler::InFacility, PotsCwmEvent::Facility);
}

//------------------------------------------------------------------------------

PotsCwmActive::PotsCwmActive() : PotsCwmState(PotsCwmState::Active)
{
   Debug::ft("PotsCwmActive.ctor");

   BindMsgAnalyzer
      (PotsCwmEventHandler::AcAnalyzeUserMessage, Service::UserPort);
   BindMsgAnalyzer
      (PotsCwmEventHandler::AcAnalyzeNetworkMessage, Service::NetworkPort);
   BindEventHandler
      (PotsCwmEventHandler::AcNotify, PotsCwmEvent::Notify);
   BindEventHandler
      (PotsCwmEventHandler::AcToneTimeout, PotsCwmEvent::ToneTimeout);
   BindEventHandler
      (PotsCwmEventHandler::AcRenotify, PotsCwmEvent::Renotify);
   BindEventHandler
      (PotsCwmEventHandler::AcFlipflop, PotsCwmEvent::Flipflop);
   BindEventHandler
      (PotsCwmEventHandler::AcReanswer, PotsCwmEvent::Reanswer);
   BindEventHandler
      (PotsCwmEventHandler::AcReanswerTimeout, PotsCwmEvent::ReanswerTimeout);
   BindEventHandler
      (PotsCwmEventHandler::AcReconnect, PotsCwmEvent::Reconnect);
   BindEventHandler
      (PotsCwmEventHandler::AcLocalRelease, PotsCwmEvent::LocalRelease);
   BindEventHandler
      (PotsCwmEventHandler::AcActiveRelease, PotsCwmEvent::ActiveRelease);
   BindEventHandler
      (PotsCwmEventHandler::AcInactiveRelease, PotsCwmEvent::InactiveRelease);
   BindEventHandler
      (PotsCwmEventHandler::AcQuiesce, PotsCwmEvent::Quiesce);
   BindEventHandler
      (PotsCwmEventHandler::AcRelay, PotsCwmEvent::Relay);
}

//==============================================================================

PotsCwmEvent::PotsCwmEvent(Id eid, ServiceSM& owner) : Event(eid, &owner)
{
   Debug::ft("PotsCwmEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwmEvent::~PotsCwmEvent()
{
   Debug::ftnt("PotsCwmEvent.dtor");
}

//------------------------------------------------------------------------------

PotsCwmFacilityEvent::PotsCwmFacilityEvent(ServiceSM& owner) :
   PotsCwmEvent(Facility, owner)
{
   Debug::ft("PotsCwmFacilityEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwmFacilityEvent::~PotsCwmFacilityEvent()
{
   Debug::ftnt("PotsCwmFacilityEvent.dtor");
}

//------------------------------------------------------------------------------

PotsCwmNotifyEvent::PotsCwmNotifyEvent(ServiceSM& owner) :
   PotsCwmEvent(Notify, owner)
{
   Debug::ft("PotsCwmNotifyEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwmNotifyEvent::~PotsCwmNotifyEvent()
{
   Debug::ftnt("PotsCwmNotifyEvent.dtor");
}

//------------------------------------------------------------------------------

PotsCwmToneTimeoutEvent::PotsCwmToneTimeoutEvent(ServiceSM& owner) :
   PotsCwmEvent(ToneTimeout, owner)
{
   Debug::ft("PotsCwmToneTimeoutEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwmToneTimeoutEvent::~PotsCwmToneTimeoutEvent()
{
   Debug::ftnt("PotsCwmToneTimeoutEvent.dtor");
}

//------------------------------------------------------------------------------

PotsCwmRenotifyEvent::PotsCwmRenotifyEvent(ServiceSM& owner) :
   PotsCwmEvent(Renotify, owner)
{
   Debug::ft("PotsCwmRenotifyEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwmRenotifyEvent::~PotsCwmRenotifyEvent()
{
   Debug::ftnt("PotsCwmRenotifyEvent.dtor");
}

//------------------------------------------------------------------------------

PotsCwmFlipflopEvent::PotsCwmFlipflopEvent(ServiceSM& owner) :
   PotsCwmEvent(Flipflop, owner)
{
   Debug::ft("PotsCwmFlipflopEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwmFlipflopEvent::~PotsCwmFlipflopEvent()
{
   Debug::ftnt("PotsCwmFlipflopEvent.dtor");
}

//------------------------------------------------------------------------------

PotsCwmReanswerEvent::PotsCwmReanswerEvent(ServiceSM& owner) :
   PotsCwmEvent(Reanswer, owner)
{
   Debug::ft("PotsCwmReanswerEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwmReanswerEvent::~PotsCwmReanswerEvent()
{
   Debug::ftnt("PotsCwmReanswerEvent.dtor");
}

//------------------------------------------------------------------------------

PotsCwmReanswerTimeoutEvent::PotsCwmReanswerTimeoutEvent(ServiceSM& owner) :
   PotsCwmEvent(ReanswerTimeout, owner)
{
   Debug::ft("PotsCwmReanswerTimeoutEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwmReanswerTimeoutEvent::~PotsCwmReanswerTimeoutEvent()
{
   Debug::ftnt("PotsCwmReanswerTimeoutEvent.dtor");
}

//------------------------------------------------------------------------------

PotsCwmReconnectEvent::PotsCwmReconnectEvent(ServiceSM& owner) :
   PotsCwmEvent(Reconnect, owner)
{
   Debug::ft("PotsCwmReconnectEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwmReconnectEvent::~PotsCwmReconnectEvent()
{
   Debug::ftnt("PotsCwmReconnectEvent.dtor");
}

//------------------------------------------------------------------------------

PotsCwmLocalReleaseEvent::PotsCwmLocalReleaseEvent(ServiceSM& owner) :
   PotsCwmEvent(LocalRelease, owner)
{
   Debug::ft("PotsCwmLocalReleaseEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwmLocalReleaseEvent::~PotsCwmLocalReleaseEvent()
{
   Debug::ftnt("PotsCwmLocalReleaseEvent.dtor");
}

//------------------------------------------------------------------------------

PotsCwmRemoteReleaseEvent::PotsCwmRemoteReleaseEvent
   (Id eid, ServiceSM& owner, Cause::Ind cause) :
   PotsCwmEvent(eid, owner),
   cause_(cause)
{
   Debug::ft("PotsCwmRemoteReleaseEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwmRemoteReleaseEvent::~PotsCwmRemoteReleaseEvent()
{
   Debug::ftnt("PotsCwmRemoteReleaseEvent.dtor");
}

//------------------------------------------------------------------------------

void PotsCwmRemoteReleaseEvent::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   PotsCwmEvent::Display(stream, prefix, options);

   stream << prefix << "cause : " << cause_ << CRLF;
}

//------------------------------------------------------------------------------

PotsCwmActiveReleaseEvent::PotsCwmActiveReleaseEvent
   (ServiceSM& owner, Cause::Ind cause) :
   PotsCwmRemoteReleaseEvent(ActiveRelease, owner, cause)
{
   Debug::ft("PotsCwmActiveReleaseEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwmActiveReleaseEvent::~PotsCwmActiveReleaseEvent()
{
   Debug::ftnt("PotsCwmActiveReleaseEvent.dtor");
}

//------------------------------------------------------------------------------

PotsCwmInactiveReleaseEvent::PotsCwmInactiveReleaseEvent
   (ServiceSM& owner, Cause::Ind cause) :
   PotsCwmRemoteReleaseEvent(InactiveRelease, owner, cause)
{
   Debug::ft("PotsCwmInactiveReleaseEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwmInactiveReleaseEvent::~PotsCwmInactiveReleaseEvent()
{
   Debug::ftnt("PotsCwmInactiveReleaseEvent.dtor");
}

//------------------------------------------------------------------------------

PotsCwmQuiesceEvent::PotsCwmQuiesceEvent(ServiceSM& owner) :
   PotsCwmEvent(Quiesce, owner)
{
   Debug::ft("PotsCwmQuiesceEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwmQuiesceEvent::~PotsCwmQuiesceEvent()
{
   Debug::ftnt("PotsCwmQuiesceEvent.dtor");
}

//------------------------------------------------------------------------------

PotsCwmRelayEvent::PotsCwmRelayEvent(ServiceSM& owner) :
   PotsCwmEvent(Relay, owner)
{
   Debug::ft("PotsCwmRelayEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwmRelayEvent::~PotsCwmRelayEvent()
{
   Debug::ftnt("PotsCwmRelayEvent.dtor");
}

//==============================================================================

PotsCwmSsm::PotsCwmSsm() : ServiceSM(PotsCwmServiceId),
   substate_(Initiating),
   activeCall_(0),
   tid_(NIL_ID),
   onhook_(false)
{
   Debug::ft("PotsCwmSsm.ctor");
}

//------------------------------------------------------------------------------

PotsCwmSsm::~PotsCwmSsm()
{
   Debug::ftnt("PotsCwmSsm.dtor");
}

//------------------------------------------------------------------------------

ServicePortId PotsCwmSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft("PotsCwmSsm.CalcPort");

   return Parent()->CalcPort(ame);
}

//------------------------------------------------------------------------------

PotsCwmSsm::PsmRole PotsCwmSsm::CalcRole(const ProtocolSM* psm) const
{
   Debug::ft("PotsCwmSsm.CalcRole");

   if(UPsm() == psm) return User;
   if(NPsm(activeCall_) == psm) return Active;
   if(NPsm(1 - activeCall_) == psm) return Inactive;
   return Transient;
}

//------------------------------------------------------------------------------

fn_name PotsCwmSsm_ClearTimer = "PotsCwmSsm.ClearTimer";

void PotsCwmSsm::ClearTimer(TimerId tid)
{
   Debug::ft(PotsCwmSsm_ClearTimer);

   if(tid_ != tid)
   {
      Debug::SwLog(PotsCwmSsm_ClearTimer, "TimerId mismatch", pack2(tid_, tid));
      return;
   }

   tid_ = NIL_ID;
}

//------------------------------------------------------------------------------

void PotsCwmSsm::ConnectInactiveCall(Tone::Id tone)
{
   Debug::ft("PotsCwmSsm.ConnectInactiveCall");

   //  Make the inactive call the active one.
   //
   activeCall_ = 1 - activeCall_;

   auto hPsm = NPsm(activeCall_);

   if(hPsm == nullptr)
   {
      Context::Kill("PSM not found", activeCall_);
      return;
   }

   //  Normally the UPSM sends and receives media.  During reringing,
   //  however, it sends and receives silence.
   //
   auto upsm = UPsm();

   upsm->SetOgTone(tone);
   upsm->SetIcTone(tone);
   upsm->SetOgPsm(hPsm);

   //  If the UPSM is sending media, also send media to the active call, even
   //  during ringing.  The only time this doesn't occur is during reringing,
   //  when we want to keep sending held tone to the far end.
   //
   if(tone == Tone::Media)
   {
      hPsm->SetOgTone(Tone::Media);
   }
}

//------------------------------------------------------------------------------

PotsMuxPsm* PotsCwmSsm::CreateNPsm() const
{
   Debug::ft("PotsCwmSsm.CreateNPsm");

   auto port = Mux()->Profile()->GetCircuit()->TsPort();
   return new PotsMuxPsm(port);
}

//------------------------------------------------------------------------------

void PotsCwmSsm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   ServiceSM::Display(stream, prefix, options);

   stream << prefix << "substate   : " << substate_ << CRLF;
   stream << prefix << "activeCall : " << activeCall_ << CRLF;
   stream << prefix << "tid        : " << tid_ << CRLF;
   stream << prefix << "onhook     : " << onhook_ << CRLF;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmSsm::Flipflop()
{
   Debug::ft("PotsCwmSsm.Flipflop");

   auto actNPsm = NPsm(activeCall_);
   auto hldNPsm = NPsm(1 - activeCall_);

   switch(substate_)
   {
   case Notifying:
   case Renotifying:
      //
      //  CWT tone is being applied.  Media will be reconnected below
      //  (it is always done in case the original call has released).
      //
      UPsm()->SetOgTone(Tone::Media);
      //  [[fallthrough]]
   case Notified:
   case Renotified:
      //
      //  A timer (either CWT tone or renotification) is running.  The
      //  waiting call has not yet been answered.  Send it an offhook
      //  so that it transitions into that state, and continue...
      //
      StopTimer(tid_);
      hldNPsm->SendSignal(PotsSignal::Offhook);
      SetSubstate(Answering);
      //  [[fallthrough]]
   case Answered:
      //
      //  Hold the active call and connect to the held call.
      //
      if(actNPsm != nullptr) actNPsm->SetOgTone(Tone::Held);
      ConnectInactiveCall(Tone::Media);
      break;

   case Holding:
      //
      //  Connect to the held call and cancel CWT.
      //
      StopTimer(ReconnectTimeoutId);
      ConnectInactiveCall(Tone::Media);
      ReleaseCwt(PotsCwtFacility::Retrieved);
      break;

   default:
      Context::Kill("invalid substate", substate_);
   }

   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsCwmSsm_OtherNPsm = "PotsCwmSsm.OtherNPsm";

PotsMuxPsm* PotsCwmSsm::OtherNPsm(const ProtocolSM* npsm) const
{
   Debug::ft(PotsCwmSsm_OtherNPsm);

   auto fid = npsm->GetFactory();

   if(fid != PotsMuxFactoryId)
   {
      Debug::SwLog(PotsCwmSsm_OtherNPsm, "invalid FactoryId", fid);
      return nullptr;
   }

   for(auto i = 0; i <= PotsMuxSsm::MaxCallId; ++i)
   {
      if(NPsm(i) == npsm)
      {
         return NPsm(PotsMuxSsm::MaxCallId - i);
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

fn_name PotsCwmSsm_ProcessInitAck = "PotsCwmSsm.ProcessInitAck";

EventHandler::Rc PotsCwmSsm::ProcessInitAck(Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsCwmSsm_ProcessInitAck);

   auto hldNPsm = static_cast< PotsMuxPsm* >(Context::ContextPsm());
   auto mux = Mux();
   auto muxUPsm = mux->UPsm();

   //  There are two CWT initiation scenarios:
   //  1. The multiplexer was just created, so hldNPsm is the first PSM.
   //  2. The multiplexer was in the Passive state, which it enters after
   //     its service is finished and a basic call remains.  In this case,
   //     a UPSM and NPSM already exist.
   //
   PotsMuxPsm* actNPsm = nullptr;

   if(muxUPsm == nullptr)
   {
      //  Create actNPsm, which will communicate with the target call.
      //  Find PEER, the address of the active call's UPSM, and reroute
      //  that UPSM so that it communicates with actNPsm.
      //
      actNPsm = CreateNPsm();

      if(actNPsm == nullptr)
      {
         Debug::SwLog(PotsCwmSsm_ProcessInitAck, "failed to create nPsm", 0);
         SendFacilityNack(hldNPsm, PotsCwbServiceId);
         return EventHandler::Suspend;
      }

      auto prof = mux->Profile();
      auto peer = prof->ObjAddr();
      GlobalAddress peerPrevRemAddr;
      auto psm = actNPsm->JoinPeer(peer, peerPrevRemAddr);

      //  If we rerouted the active call's UPSM, create our own UPSM, which
      //  will communicate with the POTS shelf.
      //
      if(psm != nullptr)
      {
         auto bcUPsm = static_cast< PotsCallPsm* >(psm);
         auto port = prof->GetCircuit()->TsPort();

         muxUPsm = new PotsCallPsm(port);

         //  We just created our UPSM, so it doesn't have a port.  It won't
         //  get one until it sends a message to the POTS shelf, which won't
         //  happen until we connect CWT tone.  But if the active call denies
         //  CWT, we won't send a message to the user at all.  Messages from
         //  the user would then continue to be routed to the active POTS call,
         //  bypassing the multiplexer.  We must therefore force a port to be
         //  allocated for this PSM now, so that PotsCallFactory.PortAllocated
         //  will register it as the user's address.
         //
         if(muxUPsm->EnsurePort() == nullptr)
         {
            muxUPsm->Destroy();
            SendFacilityNack(hldNPsm, PotsCwbServiceId);
            return EventHandler::Suspend;
         }

         muxUPsm->MakeEdge(port);
         mux->SetUPsm(*muxUPsm);

         //  Synch our UPSM's state with that of the active call's UPSM, and
         //  synch the active NPSM's media info with that of our UPSM.
         //
         bcUPsm->Synch(*muxUPsm);
         muxUPsm->SynchRelay(*actNPsm);

         //  Configure the active NPSM and our UPSM so that they are listening
         //  to each other's media streams.  Because their media info has been
         //  synched, this will not result in any messaging.
         //
         muxUPsm->CreateMedia(*actNPsm);

         //  Save the NPSMs and make note of which one is active.
         //
         mux->SetNPsm(0, *hldNPsm);
         mux->SetNPsm(1, *actNPsm);
         activeCall_ = 1;
      }
      else
      {
         Debug::SwLog(PotsCwmSsm_ProcessInitAck, "JoinPeer failed", 0);
         SendFacilityNack(hldNPsm, PotsCwbServiceId);
         return EventHandler::Suspend;
      }
   }
   else
   {
      activeCall_ = (mux->NPsm(0) == nullptr ? 1 : 0);
      mux->SetNPsm(1 - activeCall_, *hldNPsm);
      actNPsm = mux->NPsm(activeCall_);
   }

   //  Set the CWT service ID for each call.
   //
   hldNPsm->SetRemService(PotsCwbServiceId);
   actNPsm->SetRemService(PotsCwaServiceId);

   //  Configure hldNPsm so that it and the UPSM can listen to
   //  each other if the waiting call is answered.
   //
   hldNPsm->SetOgPsm(muxUPsm);
   hldNPsm->SetIcTone(Tone::Media);

   //  Relay CWT's Facility Initiation message to actNPsm, which
   //  will send it to the active call to initiate CWT.
   //
   auto msg = static_cast< PotsMessage* >(Context::ContextMsg());

   if(msg->Relay(*actNPsm))
   {
      auto pfi = msg->FindType< PotsFacilityInfo >(PotsParameter::Facility);
      pfi->sid = PotsCwaServiceId;
      SetNextState(PotsCwmState::Initiating);
      return EventHandler::Suspend;
   }

   Context::Kill("failed to relay message", 0);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmSsm::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsCwmSsm.ProcessInitNack");

   //  This has to handle the case where CWT occurs during CWT, when the
   //  nack is sent to the third NPsm.  This PSM is not registered with
   //  the multiplexer but is the context PSM.
   //
   auto npsm = static_cast< PotsMuxPsm* >(Context::ContextPsm());

   SendFacilityNack(npsm, PotsCwbServiceId);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmSsm::ProcessSip(Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsCwmSsm.ProcessSip");

   auto& ire = static_cast< InitiationReqEvent& >(currEvent);

   ire.DenyRequest();
   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmSsm::Reanswer()
{
   Debug::ft("PotsCwmSsm.Reanswer");

   if(substate_ != Reringing)
   {
      Context::Kill("invalid substate", substate_);
      return EventHandler::Suspend;
   }

   auto upsm = UPsm();

   onhook_ = false;
   StopTimer(ReanswerTimeoutId);
   upsm->EnableMedia();
   upsm->ApplyRinging(false);

   NPsm(activeCall_)->SetOgTone(Tone::Media);

   ReleaseCwt(PotsCwtFacility::Reanswered);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmSsm::Reconnect()
{
   Debug::ft("PotsCwmSsm.Reconnect");

   if(substate_ != Holding)
   {
      Context::Kill("invalid substate", substate_);
   }

   //  Reconnect the remaining call.
   //
   ConnectInactiveCall(Tone::Media);
   ReleaseCwt(PotsCwtFacility::Reconnected);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmSsm::RelayFacilityMsg()
{
   Debug::ft("PotsCwmSsm.RelayFacilityMsg");

   if(CurrState() != PotsCwmState::Initiating)
   {
      Context::Kill("invalid substate", substate_);
   }

   auto pmsg = static_cast< PotsMessage* >(Context::ContextMsg());
   auto pfi = pmsg->FindType< PotsFacilityInfo >(PotsParameter::Facility);
   auto icPsm = pmsg->Psm();
   auto ogPsm = OtherNPsm(icPsm);

   switch(pfi->ind)
   {
   case PotsCwtFacility::InitiationAck:
      SetSubstate(Pending);
      break;

   case PotsCwtFacility::InitiationNack:
      SetSubstate(Denying);
      break;

   default:
      Context::Kill("invalid facility indicator", pfi->sid);
   }

   if(ogPsm == nullptr)
   {
      Context::Kill("null outgoing PSM", pfi->sid);
      return EventHandler::Suspend;
   }

   if(!pmsg->Relay(*ogPsm))
   {
      Context::Kill("failed to relay message", pfi->sid);
      return EventHandler::Suspend;
   }

   pfi->sid = PotsCwbServiceId;
   SetNextState(PotsCwmState::Active);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmSsm::RelayMsg()
{
   Debug::ft("PotsCwmSsm.RelayMsg");

   auto pmsg = static_cast< PotsMessage* >(Context::ContextMsg());
   auto sid = pmsg->GetSignal();
   auto pptr = pmsg->FindParm(PotsParameter::Ring);
   auto mux = Mux();

   switch(substate_)
   {
   case Notifying:
   case Notified:
   case Renotifying:
   case Renotified:
   case Answering:
   case Answered:
   case Ringing:
   case Reringing:
      break;

   default:
      Context::Kill("invalid substate", pack2(sid, substate_));
   }

   switch(sid)
   {
   case PotsSignal::Onhook:
      onhook_ = true;
      break;

   case PotsSignal::Offhook:
      onhook_ = false;
      break;

   case PotsSignal::Supervise:
      //
      //  If a ring parameter is present, it should be stopping ringing.
      //  This occurs immediately after the waiting call is answered.
      //  Advance to the next state and delete the ring parameter, given
      //  that we applied CWT tone instead of relaying the parameter that
      //  would have started ringing by the waiting call.
      //
      if(pptr != nullptr)
      {
         auto ring = reinterpret_cast< PotsRingInfo* >(pptr->bytes);

         if(!ring->on)
         {
            if(substate_ == Answering)
            {
               if(CountCalls() == 2)
                  SetSubstate(Answered);
               else
                  ReleaseCwt(PotsCwtFacility::Answered);
            }
            else
            {
               Context::Kill("invalid substate", substate_);
            }
         }
         else
         {
            Context::Kill("invalid substate", substate_);
         }

         pmsg->DeleteParm(*pptr);
      }
      break;

   default:
      Context::Kill("invalid signal", sid);
      return EventHandler::Suspend;
   }

   auto icPsm = pmsg->Psm();
   auto uPsm = mux->UPsm();
   ProtocolSM* ogPsm;

   if(icPsm == uPsm)
      ogPsm = NPsm(activeCall_);
   else
      ogPsm = uPsm;

   if(!pmsg->Relay(*ogPsm))
   {
      Context::Kill("failed to relay message", sid);
   }

   //d If our UPSM doesn't have addresses yet, supply them.  Don't pass PMSG to
   //  AddressesUnknown, because its remote factory is wrong (mux, not shelf).
   //
   if((ogPsm == uPsm) && ogPsm->AddressesUnknown(nullptr))
   {
      auto& self = IpPortRegistry::LocalAddr();
      auto& peer = IpPortRegistry::LocalAddr();
      GlobalAddress locAddr(self, PotsCallIpPort, PotsCallFactoryId);
      GlobalAddress remAddr(peer, PotsShelfIpPort, PotsShelfFactoryId);

      pmsg->SetSender(locAddr);
      pmsg->SetReceiver(remAddr);
   }

   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmSsm::ReleaseActive(Cause::Ind cause, Event*& nextEvent)
{
   Debug::ft("PotsCwmSsm.ReleaseActive");

   auto upsm = UPsm();

   switch(substate_)
   {
   case Notified:
   case Renotified:
      upsm->SetOgTone(Tone::Silence);
      //  [[fallthrough]]
   case Notifying:
   case Renotifying:
      //
      //  There is still a waiting call, so allow CWT tone to finish.
      //
      break;

   case Answered:
      //
      //  Connect silence and start the reconnect timer.  If it expires,
      //  the subscriber will be reconnected to the held call.
      //
      upsm->SetOgTone(Tone::Silence);
      StartTimer(ReconnectTimeoutId, ReconnectTimeout);
      SetSubstate(Holding);
      break;

   case Reringing:
      StopTimer(ReanswerTimeoutId);
      //  [[fallthrough]]
   case Ringing:
      //
      //  The only remaining call has released.  Relay the Release and idle.
      //
      nextEvent = new PotsMuxRelayEvent(*Mux());
      SetNextState(Null);
      return EventHandler::Revert;

   default:
      Context::Kill("invalid substate", substate_);
      return EventHandler::Suspend;
   }

   upsm->SendCause(cause);
   upsm->SetIcTone(Tone::Silence);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

void PotsCwmSsm::ReleaseCwt(Facility::Ind ind)
{
   Debug::ft("PotsCwmSsm.ReleaseCwt");

   auto actNPsm = NPsm(activeCall_);
   auto hldNPsm = NPsm(1 - activeCall_);

   switch(ind)
   {
   case PotsCwtFacility::Unanswered:
      //
      //  We can't go to the Null state immediately: the waiting call
      //  replies to this message with a Release, which is needed to
      //  free its NPSM.
      //
      hldNPsm->SendSignal(PotsSignal::Facility);
      hldNPsm->SendFacility(PotsCwbServiceId, ind);

      if(actNPsm != nullptr)
      {
         actNPsm->SendSignal(PotsSignal::Facility);
         actNPsm->SendFacility(PotsCwaServiceId, ind);
      }

      SetSubstate(Releasing);
      return;

   case PotsCwtFacility::Answered:
   case PotsCwtFacility::Retrieved:
   case PotsCwtFacility::Reconnected:
   case PotsCwtFacility::Reanswered:
   case PotsCwtFacility::InactiveReleased:
   case PotsCwtFacility::Alerted:
      actNPsm->SendSignal(PotsSignal::Facility);
      actNPsm->SendFacility(ind);
      break;

   default:
      Context::Kill("invalid facility indicator", ind);
      return;
   }

   SetNextState(Null);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmSsm::ReleaseInactive
   (Cause::Ind cause, Event*& nextEvent)
{
   Debug::ft("PotsCwmSsm.ReleaseInactive");

   auto upsm = UPsm();

   switch(substate_)
   {
   case Denying:
      //
      //  CWT was denied.  The would-be waiting call has sent
      //  a Release after receiving the Facility Nack.
      //
      SetNextState(Null);
      return EventHandler::Suspend;

   case Notifying:
   case Renotifying:
      //
      //  Stop CWT tone (reconnect media) and continue...
      //
      upsm->SetOgTone(Tone::Media);
      //  [[fallthrough]]
   case Notified:
   case Renotified:
      //
      //  A timer (CWT tone or notification) is running.
      //
      StopTimer(tid_);

      //  If this was the last call, notify the user that it
      //  is over and apply disconnect treatment.
      //
      if(CountCalls() == 1)
      {
         upsm->SendCause(cause);
         break;
      }
      //  [[fallthrough]]
   case Answered:
      //
      //  The active call is still connected and CWT is over.
      //
      upsm->SendCause(cause);
      ReleaseCwt(PotsCwtFacility::InactiveReleased);
      return EventHandler::Suspend;

   case Releasing:
      //
      //  The waiting call was ignored.  If there are still two calls, we sent
      //  Facility Release messages to both and the waiting call has replied
      //  with a Release that will free its NPSM.  When this transaction ends,
      //  we're back to a basic call.
      //
      if(CountCalls() == 2)
      {
         upsm->SendCause(cause);
         SetNextState(Null);
         return EventHandler::Suspend;
      }
      break;

   case Holding:
      //
      //  This call was on hold after being answered, and its far end user
      //  has released before we were reconnected to it.
      //
      StopTimer(ReconnectTimeoutId);
      break;

   default:
      Context::Kill("invalid substate", substate_);
      return EventHandler::Suspend;
   }

   //  If we get here, the last call has just released, so we need to apply
   //  disconnect treatment.  To do this, we need to create a new call.  We
   //  do this by using the Disconnect service, which immediately moves the
   //  call into the Exception state to apply a treatment.
   //
   auto npsm = CreateNPsm();
   if(npsm == nullptr) Context::Kill("failed to allocate PSM", substate_);

   Mux()->SetNPsm(activeCall_, *npsm);
   npsm->SetIcTone(Tone::Media);
   npsm->SendSignal(PotsSignal::Facility);
   npsm->SendFacility(PotsDiscServiceId, Facility::InitiationReq);
   npsm->SendCause(cause);
   upsm->SetOgTone(Tone::Media);
   upsm->SetOgPsm(npsm);
   SetNextState(Null);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmSsm::Rering()
{
   Debug::ft("PotsCwmSsm.Rering");

   switch(substate_)
   {
   case Notifying:
   case Notified:
   case Renotifying:
   case Renotified:
      //
      //  A timer is running, so stop it and continue...
      //
      StopTimer(tid_);

      //  The waiting call hasn't been answered, but we have already sent
      //  it an Alerting, so we have to suppress the one that will arrive
      //  when we apply ringing.  The waiting call will connect media on
      //  answer, so we can enable it here.
      //
      ConnectInactiveCall(Tone::Media);
      SetSubstate(Ringing);
      break;

   case Holding:
      //
      //  The far-end subscriber released the active call, and now the CWT
      //  subscriber has released before being automatically reconnected to
      //  the remaining call.  The remaining call rerings the CWT subscriber.
      //  Whichever call remains was answered, and has therefore connected
      //  media, so we must connect silence until reanswer.
      //
      StopTimer(ReconnectTimeoutId);
      //  [[fallthrough]]
   case Answered:
      //
      //  The CWT subscriber just released the active call.  The remaining
      //  call was previously answered and now rerings the CWT subscriber.
      //
      ConnectInactiveCall(Tone::Silence);
      StartTimer(ReanswerTimeoutId, ReanswerTimeout);
      SetSubstate(Reringing);
      break;

   default:
      Context::Kill("invalid substate", substate_);
   }

   UPsm()->ApplyRinging(true);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmSsm::SendFacilityNack(PotsMuxPsm* npsm, ServiceId sid)
{
   Debug::ft("PotsCwmSsm.SendFacilityNack");

   npsm->SendSignal(PotsSignal::Facility);
   npsm->SendFacility(sid, Facility::InitiationNack);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

void PotsCwmSsm::SetNextState(StateId stid)
{
   Debug::ft("PotsCwmSsm.SetNextState");

   if(stid == Null)
   {
      UPsm()->ReportFlash(false);  //p account for TWC/CXF
   }

   ServiceSM::SetNextState(stid);
}

//------------------------------------------------------------------------------

void PotsCwmSsm::SetSubstate(Substate state)
{
   Debug::ft("PotsCwmSsm.SetSubstate");

   substate_ = state;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmSsm::StartCwtTone()
{
   Debug::ft("PotsCwmSsm.StartCwtTone");

   auto upsm = UPsm();

   switch(substate_)
   {
   case Pending:
      NPsm(0)->SendSignal(PotsSignal::Alerting);
      upsm->ReportFlash(true);
      SetSubstate(Notifying);
      break;

   case Notified:
      SetSubstate(Renotifying);
      break;

   case Renotified:
      //
      //  We've already provided CWT tone twice, so CWT is over.
      //
      ReleaseCwt(PotsCwtFacility::Unanswered);
      return EventHandler::Suspend;

   default:
      Context::Kill("invalid substate", substate_);
   }

   upsm->SetOgTone(Tone::CallWaiting);
   StartTimer(ToneTimeoutId, ToneTimeout);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsCwmSsm_StartTimer = "PotsCwmSsm.StartTimer";

void PotsCwmSsm::StartTimer(TimerId tid, uint32_t secs)
{
   Debug::ft(PotsCwmSsm_StartTimer);

   auto upsm = UPsm();

   if(tid_ != NIL_ID)
   {
      Debug::SwLog(PotsCwmSsm_StartTimer, "timer in use", pack2(tid_, tid));

      upsm->StopTimer(*this, tid_);
      tid_ = NIL_ID;
   }

   if(upsm->StartTimer(secs, *this, tid)) tid_ = tid;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmSsm::StopCwtTone()
{
   Debug::ft("PotsCwmSsm.StopCwtTone");

   switch(substate_)
   {
   case Notifying:
      StartTimer(RenotifyTimeoutId, RenotifyTimeout);
      SetSubstate(Notified);
      break;

   case Renotifying:
      SetSubstate(Renotified);
      StartTimer(RenotifyTimeoutId, RenotifyTimeout);
      break;

   default:
      Context::Kill("invalid substate", substate_);
   }

   UPsm()->SetOgTone(Tone::Media);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmSsm::StopReringing()
{
   Debug::ft("PotsCwmSsm.StopReringing");

   //  Release the remaining call and the subscriber.
   //
   if(substate_ != Reringing)
   {
      Context::Kill("invalid substate", substate_);
   }

   auto upsm = UPsm();
   auto npsm = NPsm(activeCall_);
   npsm->SendSignal(PotsSignal::Release);
   npsm->SendCause(Cause::NormalCallClearing);

   upsm->SendSignal(PotsSignal::Release);
   upsm->SendCause(Cause::NormalCallClearing);

   SetNextState(Null);
   Mux()->SetNextState(PotsMuxState::Null);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsCwmSsm_StopTimer = "PotsCwmSsm.StopTimer";

void PotsCwmSsm::StopTimer(TimerId tid)
{
   Debug::ft(PotsCwmSsm_StopTimer);

   if(tid_ != tid)
   {
      Debug::SwLog(PotsCwmSsm_StopTimer, "TimerId mismatch", pack2(tid_, tid));
      return;
   }

   UPsm()->StopTimer(*this, tid_);
   tid_ = NIL_ID;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmInAnalyzeNetworkMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwmInAnalyzeNetworkMessage.ProcessEvent");

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto pmsg = static_cast< Pots_NU_Message* >(ame.Msg());
   auto sid = pmsg->GetSignal();
   auto pfi = pmsg->FindType< PotsFacilityInfo >(PotsParameter::Facility);

   if((sid == PotsSignal::Facility) && (pfi != nullptr))
   {
      if(pfi->ind != Facility::InitiationReq)
      {
         nextEvent = new PotsCwmFacilityEvent(ssm);
         return Continue;
      }

      Context::Kill("invalid facility indicator", pfi->ind);
      return Suspend;
   }

   Context::Kill("invalid signal", sid);
   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmInFacility::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwmInFacility.ProcessEvent");

   auto& mux = static_cast< PotsCwmSsm& >(ssm);

   return mux.RelayFacilityMsg();
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmAcAnalyzeUserMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwmAcAnalyzeUserMessage.ProcessEvent");

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto sid = ame.Msg()->GetSignal();
   auto& cwt = static_cast< PotsCwmSsm& >(ssm);
   auto cwts = cwt.GetSubstate();

   switch(sid)
   {
   case Signal::Timeout:
   {
      auto tmsg = static_cast< TlvMessage* >(ame.Msg());
      auto toi = tmsg->FindType< TimeoutInfo >(Parameter::Timeout);

      switch(toi->tid)
      {
      case PotsCwmSsm::ToneTimeoutId:
         cwt.ClearTimer(PotsCwmSsm::ToneTimeoutId);
         nextEvent = new PotsCwmToneTimeoutEvent(cwt);
         return Continue;

      case PotsCwmSsm::RenotifyTimeoutId:
         cwt.ClearTimer(PotsCwmSsm::RenotifyTimeoutId);
         nextEvent = new PotsCwmRenotifyEvent(cwt);
         return Continue;

      case PotsCwmSsm::ReconnectTimeoutId:
         cwt.ClearTimer(PotsCwmSsm::ReconnectTimeoutId);
         nextEvent = new PotsCwmReconnectEvent(cwt);
         return Continue;

      case PotsCwmSsm::ReanswerTimeoutId:
         cwt.ClearTimer(PotsCwmSsm::ReanswerTimeoutId);
         nextEvent = new PotsCwmReanswerTimeoutEvent(cwt);
         return Continue;
      }

      Context::Kill("invalid TimerId", toi->tid);
      return Suspend;
   }

   case PotsSignal::Flash:
      nextEvent = new PotsCwmFlipflopEvent(cwt);
      return Continue;

   case PotsSignal::Onhook:
      //
      //  If only one call remains, rering the user immediately, else
      //  relay the message and rering the user if we receive a Release.
      //
      if(cwt.CountCalls() == 1)
         nextEvent = new PotsCwmLocalReleaseEvent(cwt);
      else
         nextEvent = new PotsCwmRelayEvent(cwt);
      return Continue;

   case PotsSignal::Alerting:
      //
      //  This subsequent alerting occurs after we apply ringing.  Don't
      //  relay it, because it will confuse whichever call is ringing us.
      //
      switch(cwts)
      {
      case PotsCwmSsm::Ringing:
         //
         //  The waiting call is in the Term Alerting state, so CWT no
         //  longer needs to be active.
         //
         nextEvent = new PotsCwmQuiesceEvent(cwt);
         return Continue;

      case PotsCwmSsm::Reringing:
         return Suspend;

      default:
         Context::Kill("invalid substate", cwts);
      }

      return Suspend;

   case PotsSignal::Offhook:
      //
      //  If we are reringing the user, this is a reanswer.  If not,
      //  it is a resume after a suspend, and only has to be relayed.
      //
      if(cwts == PotsCwmSsm::Reringing)
         nextEvent = new PotsCwmReanswerEvent(cwt);
      else
         nextEvent = new PotsCwmRelayEvent(cwt);
      return Continue;
   }

   Context::Kill("invalid signal", sid);
   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmAcAnalyzeNetworkMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwmAcAnalyzeNetworkMessage.ProcessEvent");

   //  Message received from NPSM while in Active state.
   //
   auto& cwt = static_cast< PotsCwmSsm& >(ssm);
   auto cwts = cwt.GetSubstate();
   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto pmsg = static_cast< Pots_UN_Message* >(ame.Msg());
   auto npsm = static_cast< PotsMuxPsm* >(pmsg->Psm());
   auto sid = pmsg->GetSignal();
   auto pri = pmsg->FindType< PotsRingInfo >(PotsParameter::Ring);
   auto pci = pmsg->FindType< CauseInfo >(PotsParameter::Cause);

   switch(sid)
   {
   case PotsSignal::Supervise:
      //
      //  If ringing is to be applied, connect CWT tone instead.
      //
      if((pri != nullptr) && pri->on)
      {
         nextEvent = new PotsCwmNotifyEvent(ssm);
         return Continue;
      }

      //  If we get here, relay a message from the active call and discard a
      //  message from the inactive call.
      //
      if(cwt.CalcRole(npsm) == PotsCwmSsm::Active)
      {
         nextEvent = new PotsCwmRelayEvent(ssm);
         return Continue;
      }

      return Suspend;

   case PotsSignal::Release:
      //
      //  This can be several things:
      //  o The active call being released by the CWT subscriber.
      //  o The active call being released by the far-end subscriber.
      //  o The inactive call being released by the far-end subscriber or by
      //    an answer timeout.
      //  o A transient call being released.  This occurs immediately after a
      //    third NPSM was created to initiate a multiplexer service and the
      //    request was denied.  As soon as the basic call that initiated the
      //    request receives the Facility Nack, it sends us a Release when it
      //    idles its UPSM.
      //
      switch(cwt.CalcRole(npsm))
      {
      case PotsCwmSsm::Active:
         switch(cwts)
         {
         case PotsCwmSsm::Notifying:
         case PotsCwmSsm::Notified:
         case PotsCwmSsm::Renotifying:
         case PotsCwmSsm::Renotified:
         case PotsCwmSsm::Answered:
            if(cwt.IsOnhook())
               nextEvent = new PotsCwmLocalReleaseEvent(ssm);
            else
               nextEvent = new PotsCwmActiveReleaseEvent(ssm, pci->cause);
            return Continue;

         case PotsCwmSsm::Ringing:
         case PotsCwmSsm::Reringing:
            nextEvent = new PotsCwmActiveReleaseEvent(ssm, pci->cause);
            return Continue;
         }

         Context::Kill("invalid substate", cwts);
         return Suspend;

      case PotsCwmSsm::Inactive:
         nextEvent = new PotsCwmInactiveReleaseEvent(ssm, pci->cause);
         return Continue;
      }

      return Suspend;

   case PotsSignal::Facility:
      //
      //  CWT modifiers on the basic calls do not send us a Facility signal,
      //  so pass this event to the multiplexer.  If it initiates a service,
      //  we will deny it.
      //
      return Pass;
   }

   Context::Kill("invalid signal", sid);
   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmAcNotify::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwmAcNotify.ProcessEvent");

   auto& cwt = static_cast< PotsCwmSsm& >(ssm);

   return cwt.StartCwtTone();
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmAcToneTimeout::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwmAcToneTimeout.ProcessEvent");

   auto& cwt = static_cast< PotsCwmSsm& >(ssm);

   return cwt.StopCwtTone();
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmAcRenotify::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwmAcRenotify.ProcessEvent");

   auto& cwt = static_cast< PotsCwmSsm& >(ssm);

   return cwt.StartCwtTone();
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmAcFlipflop::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwmAcFlipflop.ProcessEvent");

   auto& cwt = static_cast< PotsCwmSsm& >(ssm);

   return cwt.Flipflop();
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmAcReanswer::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwmAcReanswer.ProcessEvent");

   //  This occurs when being rerung by the remaining call.
   //
   auto& cwt = static_cast< PotsCwmSsm& >(ssm);

   return cwt.Reanswer();
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmAcReanswerTimeout::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwmAcReanswerTimeout.ProcessEvent");

   auto& cwt = static_cast< PotsCwmSsm& >(ssm);

   return cwt.StopReringing();
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmAcReconnect::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwmAcReconnect.ProcessEvent");

   auto& cwt = static_cast< PotsCwmSsm& >(ssm);

   return cwt.Reconnect();
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmAcLocalRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwmAcLocalRelease.ProcessEvent");

   auto& cwt = static_cast< PotsCwmSsm& >(ssm);

   return cwt.Rering();
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmAcActiveRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwmAcActiveRelease.ProcessEvent");

   auto& cwt = static_cast< PotsCwmSsm& >(ssm);
   auto& are = static_cast< PotsCwmActiveReleaseEvent& >(currEvent);

   return cwt.ReleaseActive(are.GetCause(), nextEvent);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmAcInactiveRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwmAcInactiveRelease.ProcessEvent");

   auto& cwt = static_cast< PotsCwmSsm& >(ssm);
   auto& ire = static_cast< PotsCwmInactiveReleaseEvent& >(currEvent);

   return cwt.ReleaseInactive(ire.GetCause(), nextEvent);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmAcQuiesce::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwmAcQuiesce.ProcessEvent");

   auto& cwt = static_cast< PotsCwmSsm& >(ssm);
   auto cwts = cwt.GetSubstate();

   if(cwts == PotsCwmSsm::Ringing)
   {
      cwt.ReleaseCwt(PotsCwtFacility::Alerted);
      return Suspend;
   }

   Context::Kill("invalid substate", cwts);
   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwmAcRelay::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwmAcRelay.ProcessEvent");

   auto& cwt = static_cast< PotsCwmSsm& >(ssm);

   return cwt.RelayMsg();
}
}
