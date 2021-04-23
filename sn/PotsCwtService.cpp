//==============================================================================
//
//  PotsCwtService.cpp
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
#include "PotsCwtService.h"
#include "Event.h"
#include "EventHandler.h"
#include "ServiceSM.h"
#include "State.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "BcCause.h"
#include "BcProgress.h"
#include "BcProtocol.h"
#include "BcSessions.h"
#include "Context.h"
#include "Debug.h"
#include "Duration.h"
#include "GlobalAddress.h"
#include "IpPortRegistry.h"
#include "LocalAddress.h"
#include "MsgPort.h"
#include "NwTypes.h"
#include "PotsCircuit.h"
#include "PotsFeatures.h"
#include "PotsProfile.h"
#include "PotsSessions.h"
#include "ProxyBcSessions.h"
#include "SbAppIds.h"
#include "SbEvents.h"
#include "SbTypes.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "TimerProtocol.h"
#include "Tones.h"

using namespace NetworkBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsCwaState : public State
{
public:
   static const Id FCWTS = ServiceSM::Null;

   static const Id Null   = FCWTS + 0;
   static const Id Active = FCWTS + 1;
protected:
   explicit PotsCwaState(Id stid);
   virtual ~PotsCwaState();
};

class PotsCwaNull : public PotsCwaState
{
   friend class Singleton< PotsCwaNull >;

   PotsCwaNull();
   ~PotsCwaNull() = default;
};

class PotsCwaActive : public PotsCwaState
{
   friend class Singleton< PotsCwaActive >;

   PotsCwaActive();
   ~PotsCwaActive() = default;
};

class PotsCwbState : public State
{
public:
   static const Id FCWTS = ServiceSM::Null;

   static const Id Null    = FCWTS + 0;
   static const Id Pending = FCWTS + 1;
   static const Id Active  = FCWTS + 2;
protected:
   explicit PotsCwbState(Id stid);
   virtual ~PotsCwbState();
};

class PotsCwbNull : public PotsCwbState
{
   friend class Singleton< PotsCwbNull >;

   PotsCwbNull();
   ~PotsCwbNull() = default;
};

class PotsCwbPending : public PotsCwbState
{
   friend class Singleton< PotsCwbPending >;

   PotsCwbPending();
   ~PotsCwbPending() = default;
};

class PotsCwbActive : public PotsCwbState
{
   friend class Singleton< PotsCwbActive >;

   PotsCwbActive();
   ~PotsCwbActive() = default;
};

class PotsCwtEvent : public Event
{
public:
   static const Id Ack     = NextId + 0;
   static const Id Release = NextId + 1;
   virtual ~PotsCwtEvent();
protected:
   PotsCwtEvent(Id eid, ServiceSM& owner);
};

class PotsCwtAckEvent : public PotsCwtEvent
{
public:
   explicit PotsCwtAckEvent(ServiceSM& owner);
   ~PotsCwtAckEvent();
};

class PotsCwtReleaseEvent : public PotsCwtEvent
{
public:
   PotsCwtReleaseEvent(ServiceSM& owner, Facility::Ind ind);
   ~PotsCwtReleaseEvent();
   Facility::Ind Ind() const { return ind_; }
   void Display(ostream& stream,
      const string& prefix, const Flags& options) const override;
private:
   Facility::Ind ind_;
};

class PotsCwtEventHandler : public EventHandler
{
public:
   static const Id PeAnalyzeUserMessage = NextId + 0;
   static const Id PeAck                = NextId + 1;
   static const Id PeRelease            = NextId + 2;
   static const Id AcAnalyzeUserMessage = NextId + 3;
   static const Id AcRelease            = NextId + 4;
protected:
   PotsCwtEventHandler() = default;
   virtual ~PotsCwtEventHandler() = default;
};

class PotsCwtAcAnalyzeUserMessage : public PotsCwtEventHandler
{
   friend class Singleton< PotsCwtAcAnalyzeUserMessage >;

   PotsCwtAcAnalyzeUserMessage() = default;
   ~PotsCwtAcAnalyzeUserMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwtAcRelease : public PotsCwtEventHandler
{
   friend class Singleton< PotsCwtAcRelease >;

   PotsCwtAcRelease() = default;
   ~PotsCwtAcRelease() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwtPeAnalyzeUserMessage : public PotsCwtEventHandler
{
   friend class Singleton< PotsCwtPeAnalyzeUserMessage >;

   PotsCwtPeAnalyzeUserMessage() = default;
   ~PotsCwtPeAnalyzeUserMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwtPeAck : public PotsCwtEventHandler
{
   friend class Singleton< PotsCwtPeAck >;

   PotsCwtPeAck() = default;
   ~PotsCwtPeAck() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwtPeRelease : public PotsCwtEventHandler
{
   friend class Singleton< PotsCwtPeRelease >;

   PotsCwtPeRelease() = default;
   ~PotsCwtPeRelease() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwtPrPresentCall : public PotsCwtEventHandler
{
   friend class Singleton< PotsCwtPrPresentCall >;

   PotsCwtPrPresentCall() = default;
   ~PotsCwtPrPresentCall() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwtSsm : public ServiceSM
{
public:
   virtual ~PotsCwtSsm();
protected:
   explicit PotsCwtSsm(ServiceId sid);
   virtual void Cancel();
   ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;
   EventHandler::Rc ProcessSap(Event& currEvent, Event*& nextEvent) override;
   EventHandler::Rc ProcessSnp(Event& currEvent, Event*& nextEvent) override;
};

class PotsCwaSsm : public PotsCwtSsm
{
public:
   PotsCwaSsm();
   ~PotsCwaSsm();
private:
   EventHandler::Rc ProcessInitAck
      (Event& currEvent, Event*& nextEvent) override;
   EventHandler::Rc ProcessInitNack
      (Event& currEvent, Event*& nextEvent) override;
};

class PotsCwbSsm : public PotsCwtSsm
{
public:
   static const secs_t InitiationTimeout = 6;

   static const TimerId InitiationTimeoutId = 1;

   PotsCwbSsm();
   ~PotsCwbSsm();
   void StartTimer(TimerId tid, secs_t duration);
   void StopTimer(TimerId tid);
   void ClearTimer(TimerId tid);
   void FreeContext();
   EventHandler::Rc RestoreContext(Event*& nextEvent);
   void Cancel() override;
   void Display(ostream& stream,
      const string& prefix, const Flags& options) const override;
private:
   EventHandler::Rc ProcessInitAck
      (Event& currEvent, Event*& nextEvent) override;
   EventHandler::Rc ProcessInitNack
      (Event& currEvent, Event*& nextEvent) override;

   AnalyzeSapEvent* sap_;
   TimerId tid_;
};

//==============================================================================

PotsCwtInitiator::PotsCwtInitiator() :
   Initiator(PotsCwbServiceId, PotsCallServiceId,
   BcTrigger::LocalBusySap, PotsLocalBusySap::PotsCwtPriority)
{
   Debug::ft("PotsCwtInitiator.ctor");
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwtInitiator::ProcessEvent
   (const ServiceSM& parentSsm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwtInitiator.ProcessEvent");

   auto& pssm = static_cast< const PotsBcSsm& >(parentSsm);
   auto prof = pssm.Profile();

   if(prof->HasFeature(CWT))
   {
      nextEvent = new InitiationReqEvent(*currEvent.Owner(), PotsCwbServiceId);
      return EventHandler::Initiate;
   }

   return EventHandler::Pass;
}

//==============================================================================

fixed_string PotsCwtReleaseEventStr = "PotsCwtReleaseEvent";

PotsCwtService::PotsCwtService(Id sid) : Service(sid, false, true)
{
   Debug::ft("PotsCwtService.ctor");

   BindHandler(*Singleton< PotsCwtAcAnalyzeUserMessage >::Instance(),
      PotsCwtEventHandler::AcAnalyzeUserMessage);
   BindHandler(*Singleton< PotsCwtAcRelease >::Instance(),
      PotsCwtEventHandler::AcRelease);

   BindEventName(PotsCwtReleaseEventStr, PotsCwtEvent::Release);
}

//------------------------------------------------------------------------------

PotsCwtService::~PotsCwtService()
{
   Debug::ftnt("PotsCwtService.dtor");
}

//==============================================================================

PotsCwaService::PotsCwaService() : PotsCwtService(PotsCwaServiceId)
{
   Debug::ft("PotsCwaService.ctor");

   Singleton< PotsCwaNull >::Instance();
   Singleton< PotsCwaActive >::Instance();
}

//------------------------------------------------------------------------------

PotsCwaService::~PotsCwaService()
{
   Debug::ftnt("PotsCwaService.dtor");
}

//------------------------------------------------------------------------------

ServiceSM* PotsCwaService::AllocModifier() const
{
   Debug::ft("PotsCwaService.AllocModifier");

   return new PotsCwaSsm;
}

//==============================================================================

fixed_string PotsCwtAckEventStr = "PotsCwtAckEvent";

PotsCwbService::PotsCwbService() : PotsCwtService(PotsCwbServiceId)
{
   Debug::ft("PotsCwbService.ctor");

   Singleton< PotsCwbNull >::Instance();
   Singleton< PotsCwbPending >::Instance();
   Singleton< PotsCwbActive >::Instance();

   BindHandler(*Singleton< PotsCwtPeAnalyzeUserMessage >::Instance(),
      PotsCwtEventHandler::PeAnalyzeUserMessage);
   BindHandler(*Singleton< PotsCwtPeAck >::Instance(),
      PotsCwtEventHandler::PeAck);
   BindHandler(*Singleton< PotsCwtPeRelease >::Instance(),
      PotsCwtEventHandler::PeRelease);
   Singleton< PotsCwtPrPresentCall >::Instance();

   BindEventName(PotsCwtAckEventStr, PotsCwtEvent::Ack);
}

//------------------------------------------------------------------------------

PotsCwbService::~PotsCwbService()
{
   Debug::ftnt("PotsCwbService.dtor");
}

//------------------------------------------------------------------------------

ServiceSM* PotsCwbService::AllocModifier() const
{
   Debug::ft("PotsCwbService.AllocModifier");

   return new PotsCwbSsm;
}

//==============================================================================

PotsCwaState::PotsCwaState(Id stid) : State(PotsCwaServiceId, stid)
{
   Debug::ft("PotsCwaState.ctor");
}

//------------------------------------------------------------------------------

PotsCwaState::~PotsCwaState()
{
   Debug::ftnt("PotsCwaState.dtor");
}

//------------------------------------------------------------------------------

PotsCwaNull::PotsCwaNull() : PotsCwaState(PotsCwaState::Null)
{
   Debug::ft("PotsCwaNull.ctor");
}

//------------------------------------------------------------------------------

PotsCwaActive::PotsCwaActive() : PotsCwaState(PotsCwaState::Active)
{
   Debug::ft("PotsCwaActive.ctor");

   BindMsgAnalyzer
      (PotsCwtEventHandler::AcAnalyzeUserMessage, Service::UserPort);
   BindEventHandler
      (PotsCwtEventHandler::AcRelease, PotsCwtEvent::Release);
}

//==============================================================================

PotsCwbState::PotsCwbState(Id stid) : State(PotsCwbServiceId, stid)
{
   Debug::ft("PotsCwbState.ctor");
}

//------------------------------------------------------------------------------

PotsCwbState::~PotsCwbState()
{
   Debug::ftnt("PotsCwbState.dtor");
}

//------------------------------------------------------------------------------

PotsCwbNull::PotsCwbNull() : PotsCwbState(PotsCwbState::Null)
{
   Debug::ft("PotsCwbNull.ctor");
}

//------------------------------------------------------------------------------

PotsCwbPending::PotsCwbPending() : PotsCwbState(PotsCwbState::Pending)
{
   Debug::ft("PotsCwbPending.ctor");

   BindMsgAnalyzer
      (PotsCwtEventHandler::PeAnalyzeUserMessage, Service::UserPort);
   BindEventHandler
      (PotsCwtEventHandler::PeAck, PotsCwtEvent::Ack);
   BindEventHandler
      (PotsCwtEventHandler::PeRelease, PotsCwtEvent::Release);
}

//------------------------------------------------------------------------------

PotsCwbActive::PotsCwbActive() : PotsCwbState(PotsCwbState::Active)
{
   Debug::ft("PotsCwbActive.ctor");

   BindMsgAnalyzer
      (PotsCwtEventHandler::AcAnalyzeUserMessage, Service::UserPort);
   BindEventHandler
      (PotsCwtEventHandler::AcRelease, PotsCwtEvent::Release);
}

//==============================================================================

PotsCwtEvent::PotsCwtEvent(Id eid, ServiceSM& owner) : Event(eid, &owner)
{
   Debug::ft("PotsCwtEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwtEvent::~PotsCwtEvent()
{
   Debug::ftnt("PotsCwtEvent.dtor");
}

//------------------------------------------------------------------------------

PotsCwtAckEvent::PotsCwtAckEvent(ServiceSM& owner) : PotsCwtEvent(Ack, owner)
{
   Debug::ft("PotsCwtAckEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwtAckEvent::~PotsCwtAckEvent()
{
   Debug::ftnt("PotsCwtAckEvent.dtor");
}

//------------------------------------------------------------------------------

PotsCwtReleaseEvent::PotsCwtReleaseEvent(ServiceSM& owner, Facility::Ind ind) :
   PotsCwtEvent(Release, owner),
   ind_(ind)
{
   Debug::ft("PotsCwtReleaseEvent.ctor");
}

//------------------------------------------------------------------------------

PotsCwtReleaseEvent::~PotsCwtReleaseEvent()
{
   Debug::ftnt("PotsCwtReleaseEvent.dtor");
}

//------------------------------------------------------------------------------

void PotsCwtReleaseEvent::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   PotsCwtEvent::Display(stream, prefix, options);

   stream << prefix << "ind : " << ind_ << CRLF;
}

//==============================================================================

PotsCwtSsm::PotsCwtSsm(ServiceId sid) : ServiceSM(sid)
{
   Debug::ft("PotsCwtSsm.ctor");
}

//------------------------------------------------------------------------------

PotsCwtSsm::~PotsCwtSsm()
{
   Debug::ftnt("PotsCwtSsm.dtor");
}

//------------------------------------------------------------------------------

ServicePortId PotsCwtSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft("PotsCwtSsm.CalcPort");

   return Parent()->CalcPort(ame);
}

//------------------------------------------------------------------------------

void PotsCwtSsm::Cancel()
{
   Debug::ft("PotsCwtSsm.Cancel");

   SetNextState(Null);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwtSsm::ProcessSap(Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsCwtSsm.ProcessSap");

   auto& sap = static_cast< AnalyzeSapEvent& >(currEvent);
   auto tid = sap.GetTrigger();

   if(tid == BcTrigger::ApplyTreatmentSap)
   {
      auto pssm = static_cast< PotsBcSsm* >(Parent());
      auto ate = static_cast< BcApplyTreatmentEvent* >(sap.CurrEvent());

      pssm->ClearCall(ate->GetCause());
      return EventHandler::Suspend;
   }

   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwtSsm::ProcessSnp(Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsCwtSsm.ProcessSnp");

   auto pssm = static_cast< PotsBcSsm* >(Parent());

   if(pssm->HasIdled())
   {
      Cancel();
   }
   else
   {
      auto& snp = static_cast< AnalyzeSnpEvent& >(currEvent);

      if(snp.GetTrigger() == ProxyBcTrigger::UserReleasedSnp)
      {
         Cancel();
      }
   }

   return EventHandler::Pass;
}

//==============================================================================

PotsCwaSsm::PotsCwaSsm() : PotsCwtSsm(PotsCwaServiceId)
{
   Debug::ft("PotsCwaSsm.ctor");
}

//------------------------------------------------------------------------------

PotsCwaSsm::~PotsCwaSsm()
{
   Debug::ftnt("PotsCwaSsm.dtor");
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwaSsm::ProcessInitAck
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsCwaSsm.ProcessInitAck");

   auto pssm = static_cast< PotsBcSsm* >(Parent());
   auto stid = pssm->CurrState();
   auto upsm = PotsCallPsm::Cast(pssm->UPsm());

   //  The target call must be answered for CWT to be allowed.
   //
   switch(stid)
   {
   case BcState::Active:
   case BcState::RemoteSuspending:
      upsm->SendFacility(PotsCwmServiceId, Facility::InitiationAck);
      upsm->MakeRelay();
      SetNextState(PotsCwaState::Active);
      break;
   default:
      upsm->SendFacility(PotsCwmServiceId, Facility::InitiationNack);
   }

   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwaSsm::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsCwaSsm.ProcessInitNack");

   auto pssm = static_cast< PotsBcSsm* >(Parent());
   auto upsm = PotsCallPsm::Cast(pssm->UPsm());

   upsm->SendFacility(PotsCwmServiceId, Facility::InitiationNack);
   return EventHandler::Suspend;
}

//==============================================================================

PotsCwbSsm::PotsCwbSsm() : PotsCwtSsm(PotsCwbServiceId),
   sap_(nullptr),
   tid_(NIL_ID)
{
   Debug::ft("PotsCwbSsm.ctor");
}

//------------------------------------------------------------------------------

PotsCwbSsm::~PotsCwbSsm()
{
   Debug::ftnt("PotsCwbSsm.dtor");
}

//------------------------------------------------------------------------------

void PotsCwbSsm::Cancel()
{
   Debug::ft("PotsCwbSsm.Cancel");

   if(tid_ != NIL_ID) StopTimer(tid_);
   PotsCwtSsm::Cancel();
}

//------------------------------------------------------------------------------

fn_name PotsCwbSsm_ClearTimer = "PotsCwbSsm.ClearTimer";

void PotsCwbSsm::ClearTimer(TimerId tid)
{
   Debug::ft(PotsCwbSsm_ClearTimer);

   if(tid_ != tid)
   {
      Debug::SwLog(PotsCwbSsm_ClearTimer, "TimerId mismatch", pack2(tid_, tid));
      return;
   }

   tid_ = NIL_ID;
}

//------------------------------------------------------------------------------

void PotsCwbSsm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   ServiceSM::Display(stream, prefix, options);

   stream << prefix << "sap : " << sap_ << CRLF;
   stream << prefix << "tid : " << tid_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name PotsCwbSsm_FreeContext = "PotsCwbSsm.FreeContext";

void PotsCwbSsm::FreeContext()
{
   Debug::ft(PotsCwbSsm_FreeContext);

   if(sap_ != nullptr)
   {
      sap_->FreeContext(true);
      sap_ = nullptr;
   }
   else
   {
      Debug::SwLog(PotsCwbSsm_FreeContext, "null SAP event", 0);
   }
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwbSsm::ProcessInitAck
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsCwbSsm.ProcessInitAck");

   auto& init = static_cast< InitiationReqEvent& >(currEvent);
   auto sap = init.GetSapEvent();
   auto pssm = static_cast< PotsBcSsm* >(Parent());
   auto stid = pssm->CurrState();

   if(stid != BcState::SelectingFacility)
   {
      Context::Kill("invalid state", stid);
      return EventHandler::Suspend;
   }

   //  Save the current context (the Local Busy event) so that we
   //  can resume handling it if CWT fails.
   //
   if(sap->SaveContext())
      sap_ = sap;
   else
      return EventHandler::Resume;

   //  See what type of SSM is associated with the PSM that is registered
   //  in the CWT subscriber's profile.
   //
   auto prof = pssm->Profile();
   auto addr = prof->ObjAddr();
   auto sid = MsgPort::Find(addr)->RootSsm()->Sid();
   auto port = prof->GetCircuit()->TsPort();
   auto upsm = new PotsCallPsm(port);

   pssm->SetUPsm(*upsm);
   upsm->SendFacility(PotsCwmServiceId, Facility::InitiationReq);

   //  Set the source and destination addresses in our Facility message.
   //
   auto msg = upsm->AccessOgMsg();
   auto host = IpPortRegistry::HostAddress();
   GlobalAddress locAddr(host, NilIpPort, PotsCallFactoryId);
   GlobalAddress remAddr(host, NilIpPort, PotsMuxFactoryId);

   msg->SetSender(locAddr);
   msg->SetReceiver(remAddr);

   if(sid == PotsMuxServiceId)
   {
      //  When this message creates a PSM, it must join the multiplexer's
      //  existing context.
      //
      msg->SetJoin(true);
   }

   StartTimer(InitiationTimeoutId, InitiationTimeout);
   SetNextState(PotsCwbState::Pending);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwbSsm::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsCwbSsm.ProcessInitNack");

   return EventHandler::Resume;
}

//------------------------------------------------------------------------------

fn_name PotsCwbSsm_RestoreContext = "PotsCwbSsm.RestoreContext";

EventHandler::Rc PotsCwbSsm::RestoreContext(Event*& nextEvent)
{
   Debug::ft(PotsCwbSsm_RestoreContext);

   auto rc = EventHandler::Suspend;

   if(sap_ != nullptr)
   {
      nextEvent = sap_->RestoreContext(rc);
      sap_ = nullptr;
   }
   else
   {
      Debug::SwLog(PotsCwbSsm_RestoreContext, "null SAP event", 0);
   }

   return rc;
}

//------------------------------------------------------------------------------

fn_name PotsCwbSsm_StartTimer = "PotsCwbSsm.StartTimer";

void PotsCwbSsm::StartTimer(TimerId tid, secs_t duration)
{
   Debug::ft(PotsCwbSsm_StartTimer);

   auto pssm = static_cast< PotsBcSsm* >(Parent());
   auto upsm = PotsCallPsm::Cast(pssm->UPsm());

   if(tid_ != NIL_ID)
   {
      Debug::SwLog(PotsCwbSsm_StartTimer, "timer in use", pack2(tid_, tid));

      upsm->StopTimer(*this, tid_);
      tid_ = NIL_ID;
   }

   if(upsm->StartTimer(duration, *this, tid)) tid_ = tid;
}

//------------------------------------------------------------------------------

fn_name PotsCwbSsm_StopTimer = "PotsCwbSsm.StopTimer";

void PotsCwbSsm::StopTimer(TimerId tid)
{
   Debug::ft(PotsCwbSsm_StopTimer);

   auto pssm = static_cast< PotsBcSsm* >(Parent());
   auto upsm = PotsCallPsm::Cast(pssm->UPsm());

   if(tid_ != tid)
   {
      Debug::SwLog(PotsCwbSsm_StopTimer, "TimerId mismatch", pack2(tid_, tid));
      return;
   }

   upsm->StopTimer(*this, tid_);
   tid_ = NIL_ID;
}

//==============================================================================

EventHandler::Rc PotsCwtPeAnalyzeUserMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwtPeAnalyzeUserMessage.ProcessEvent");

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto sid = ame.Msg()->GetSignal();
   auto& cwtssm = static_cast< PotsCwbSsm& >(ssm);

   switch(sid)
   {
   case PotsSignal::Facility:
      {
         auto pmsg = static_cast< Pots_UN_Message* >(ame.Msg());
         auto pfi = pmsg->FindType< PotsFacilityInfo >(PotsParameter::Facility);

         if(pfi->sid == PotsCwbServiceId)
         {
            cwtssm.StopTimer(PotsCwbSsm::InitiationTimeoutId);

            if(pfi->ind == Facility::InitiationAck)
            {
               nextEvent = new PotsCwtAckEvent(cwtssm);
            }
            else
            {
               nextEvent = new PotsCwtReleaseEvent
                  (cwtssm, Facility::InitiationNack);
            }

            return Continue;
         }
      }
      break;

   case Signal::Timeout:
      auto tmsg = static_cast< TlvMessage* >(ame.Msg());
      auto toi = tmsg->FindType< TimeoutInfo >(Parameter::Timeout);

      if((toi->owner == &ssm) &&
         (toi->tid == PotsCwbSsm::InitiationTimeoutId))
      {
         cwtssm.ClearTimer(PotsCwbSsm::InitiationTimeoutId);
         nextEvent = new PotsCwtReleaseEvent
            (cwtssm, PotsCwtFacility::InitiationTimeout);
         return Continue;
      }
   }

   return Pass;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwtPeAck::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwtPeAck.ProcessEvent");

   auto& cwtssm = static_cast< PotsCwbSsm& >(ssm);
   auto& pssm = static_cast< PotsBcSsm& >(*ssm.Parent());
   auto handler = Singleton< PotsCwtPrPresentCall >::Instance();

   cwtssm.FreeContext();
   pssm.SetNextState(BcState::PresentingCall);
   pssm.SetNextSap(BcTrigger::PresentCallSap);
   nextEvent = new ForceTransitionEvent(pssm, *handler);
   ssm.SetNextState(PotsCwbState::Active);
   return EventHandler::Revert;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwtPeRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwtPeRelease.ProcessEvent");

   auto& relevt = static_cast< PotsCwtReleaseEvent& >(currEvent);
   auto& cwtssm = static_cast< PotsCwbSsm& >(ssm);

   if(relevt.Ind() == PotsCwtFacility::InitiationTimeout)
   {
      auto pssm = static_cast< PotsBcSsm* >(cwtssm.Parent());
      auto upsm = PotsCallPsm::Cast(pssm->UPsm());
      upsm->SendFacility(PotsCwmServiceId, Facility::InitiationNack);
   }

   cwtssm.Cancel();
   return cwtssm.RestoreContext(nextEvent);
}

//------------------------------------------------------------------------------

fn_name PotsCwtAcAnalyzeUserMessage_ProcessEvent =
   "PotsCwtAcAnalyzeUserMessage.ProcessEvent";

EventHandler::Rc PotsCwtAcAnalyzeUserMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsCwtAcAnalyzeUserMessage_ProcessEvent);

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto sid = ame.Msg()->GetSignal();
   auto& cwtssm = static_cast< PotsCwtSsm& >(ssm);

   if(sid == PotsSignal::Facility)
   {
      auto pmsg = static_cast< Pots_UN_Message* >(ame.Msg());

      auto pfi = pmsg->FindType< PotsFacilityInfo >(PotsParameter::Facility);

      if((pfi->sid == PotsCwaServiceId) || (pfi->sid == PotsCwbServiceId))
      {
         switch(pfi->ind)
         {
         case PotsCwtFacility::Unanswered:
         case PotsCwtFacility::Answered:
         case PotsCwtFacility::Retrieved:
         case PotsCwtFacility::Reconnected:
         case PotsCwtFacility::Reanswered:
         case PotsCwtFacility::InactiveReleased:
         case PotsCwtFacility::Alerted:
            //
            //  All of these mean that POTS basic call can take over.
            //
            nextEvent = new PotsCwtReleaseEvent(cwtssm, pfi->ind);
            return Continue;
         default:
            Debug::SwLog(PotsCwtAcAnalyzeUserMessage_ProcessEvent,
               "unexpected Facility::Ind", pfi->ind);
         }

         return Suspend;
      }
   }

   return Pass;
}

//------------------------------------------------------------------------------

fn_name PotsCwtAcRelease_ProcessEvent = "PotsCwtAcRelease.ProcessEvent";

EventHandler::Rc PotsCwtAcRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsCwtAcRelease_ProcessEvent);

   auto& cwtssm = static_cast< PotsCwtSsm& >(ssm);
   auto& relevt = static_cast< PotsCwtReleaseEvent& >(currEvent);
   auto ind = relevt.Ind();
   auto pssm = static_cast< PotsBcSsm* >(cwtssm.Parent());

   switch(ind)
   {
   case PotsCwtFacility::Unanswered:
      if(pssm->CurrState() == BcState::TermAlerting)
      {
         //  This instance of CWT is running on the waiting call, which has
         //  gone unanswered.  Release the UPSM (and thus the multiplexer's
         //  NPSM).  Provide ringback until the normal answer timeout occurs.
         //  Note that the call will have no UPSM when it eventually clears,
         //  even though it is in the Term Alerting state, where it usually
         //  does have one.
         //
         auto npsm = pssm->NPsm();
         auto upsm = PotsCallPsm::Cast(pssm->UPsm());

         npsm->SetOgTone(Tone::Ringback);
         upsm->SendSignal(PotsSignal::Release);
         upsm->SendCause(Cause::AnswerTimeout);
      }
      //  [[fallthrough]]
   case PotsCwtFacility::Answered:
   case PotsCwtFacility::Retrieved:
   case PotsCwtFacility::Reconnected:
   case PotsCwtFacility::Reanswered:
   case PotsCwtFacility::InactiveReleased:
   case PotsCwtFacility::Alerted:
      cwtssm.SetNextState(ServiceSM::Null);
      break;
   default:
      Debug::SwLog(PotsCwtAcRelease_ProcessEvent,
         "unexpected Facility::Ind", ind);
   }

   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsCwtPrPresentCall::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsCwtPrPresentCall.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto upsm = PotsCallPsm::Cast(pssm.UPsm());
   auto npsm = pssm.NPsm();

   npsm->EnableMedia(*upsm);
   upsm->ApplyRinging(true);
   pssm.StartTimer
      (PotsProtocol::AlertingTimeoutId, PotsProtocol::AlertingTimeout);
   pssm.BuildCipCpg(Progress::EndOfSelection);
   pssm.SetNextSnp(BcTrigger::PresentCallSnp);
   return Suspend;
}
}
