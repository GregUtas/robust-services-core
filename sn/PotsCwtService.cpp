//==============================================================================
//
//  PotsCwtService.cpp
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
#include "PotsCwtService.h"
#include "Event.h"
#include "EventHandler.h"
#include "ServiceSM.h"
#include "State.h"
#include <ostream>
#include <string>
#include "BcCause.h"
#include "BcProgress.h"
#include "BcProtocol.h"
#include "BcSessions.h"
#include "Clock.h"
#include "Context.h"
#include "Debug.h"
#include "GlobalAddress.h"
#include "IpPortRegistry.h"
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
private:
   PotsCwaNull();
};

class PotsCwaActive : public PotsCwaState
{
   friend class Singleton< PotsCwaActive >;
private:
   PotsCwaActive();
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
private:
   PotsCwbNull();
};

class PotsCwbPending : public PotsCwbState
{
   friend class Singleton< PotsCwbPending >;
private:
   PotsCwbPending();
};

class PotsCwbActive : public PotsCwbState
{
   friend class Singleton< PotsCwbActive >;
private:
   PotsCwbActive();
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
private:
   PotsCwtAcAnalyzeUserMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwtAcRelease : public PotsCwtEventHandler
{
   friend class Singleton< PotsCwtAcRelease >;
private:
   PotsCwtAcRelease() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwtPeAnalyzeUserMessage : public PotsCwtEventHandler
{
   friend class Singleton< PotsCwtPeAnalyzeUserMessage >;
private:
   PotsCwtPeAnalyzeUserMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwtPeAck : public PotsCwtEventHandler
{
   friend class Singleton< PotsCwtPeAck >;
private:
   PotsCwtPeAck() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwtPeRelease : public PotsCwtEventHandler
{
   friend class Singleton< PotsCwtPeRelease >;
private:
   PotsCwtPeRelease() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

class PotsCwtPrPresentCall : public PotsCwtEventHandler
{
   friend class Singleton< PotsCwtPrPresentCall >;
private:
   PotsCwtPrPresentCall() = default;
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

fn_name PotsCwtInitiator_ctor = "PotsCwtInitiator.ctor";

PotsCwtInitiator::PotsCwtInitiator() :
   Initiator(PotsCwbServiceId, PotsCallServiceId,
   BcTrigger::LocalBusySap, PotsLocalBusySap::PotsCwtPriority)
{
   Debug::ft(PotsCwtInitiator_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCwtInitiator_ProcessEvent = "PotsCwtInitiator.ProcessEvent";

EventHandler::Rc PotsCwtInitiator::ProcessEvent
   (const ServiceSM& parentSsm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsCwtInitiator_ProcessEvent);

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

fn_name PotsCwtService_ctor = "PotsCwtService.ctor";

PotsCwtService::PotsCwtService(Id sid) : Service(sid, false, true)
{
   Debug::ft(PotsCwtService_ctor);

   BindHandler(*Singleton< PotsCwtAcAnalyzeUserMessage >::Instance(),
      PotsCwtEventHandler::AcAnalyzeUserMessage);
   BindHandler(*Singleton< PotsCwtAcRelease >::Instance(),
      PotsCwtEventHandler::AcRelease);

   BindEventName(PotsCwtReleaseEventStr, PotsCwtEvent::Release);
}

//------------------------------------------------------------------------------

fn_name PotsCwtService_dtor = "PotsCwtService.dtor";

PotsCwtService::~PotsCwtService()
{
   Debug::ft(PotsCwtService_dtor);
}

//==============================================================================

fn_name PotsCwaService_ctor = "PotsCwaService.ctor";

PotsCwaService::PotsCwaService() : PotsCwtService(PotsCwaServiceId)
{
   Debug::ft(PotsCwaService_ctor);

   Singleton< PotsCwaNull >::Instance();
   Singleton< PotsCwaActive >::Instance();
}

//------------------------------------------------------------------------------

fn_name PotsCwaService_dtor = "PotsCwaService.dtor";

PotsCwaService::~PotsCwaService()
{
   Debug::ft(PotsCwaService_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCwaService_AllocModifier = "PotsCwaService.AllocModifier";

ServiceSM* PotsCwaService::AllocModifier() const
{
   Debug::ft(PotsCwaService_AllocModifier);

   return new PotsCwaSsm;
}

//==============================================================================

fixed_string PotsCwtAckEventStr = "PotsCwtAckEvent";

fn_name PotsCwbService_ctor = "PotsCwbService.ctor";

PotsCwbService::PotsCwbService() : PotsCwtService(PotsCwbServiceId)
{
   Debug::ft(PotsCwbService_ctor);

   Singleton< PotsCwbNull >::Instance();
   Singleton< PotsCwbPending >::Instance();
   Singleton< PotsCwbActive >::Instance();

   BindHandler(*Singleton< PotsCwtPeAnalyzeUserMessage >::Instance(),
      PotsCwtEventHandler::PeAnalyzeUserMessage);
   BindHandler(*Singleton< PotsCwtPeAck >::Instance(),
      PotsCwtEventHandler::PeAck);
   BindHandler(*Singleton< PotsCwtPeRelease >::Instance(),
      PotsCwtEventHandler::PeRelease);

   BindEventName(PotsCwtAckEventStr, PotsCwtEvent::Ack);
}

//------------------------------------------------------------------------------

fn_name PotsCwbService_dtor = "PotsCwbService.dtor";

PotsCwbService::~PotsCwbService()
{
   Debug::ft(PotsCwbService_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCwbService_AllocModifier = "PotsCwbService.AllocModifier";

ServiceSM* PotsCwbService::AllocModifier() const
{
   Debug::ft(PotsCwbService_AllocModifier);

   return new PotsCwbSsm;
}

//==============================================================================

fn_name PotsCwaState_ctor = "PotsCwaState.ctor";

PotsCwaState::PotsCwaState(Id stid) : State(PotsCwaServiceId, stid)
{
   Debug::ft(PotsCwaState_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCwaState_dtor = "PotsCwaState.dtor";

PotsCwaState::~PotsCwaState()
{
   Debug::ft(PotsCwaState_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCwaNull_ctor = "PotsCwaNull.ctor";

PotsCwaNull::PotsCwaNull() : PotsCwaState(PotsCwaState::Null)
{
   Debug::ft(PotsCwaNull_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCwaActive_ctor = "PotsCwaActive.ctor";

PotsCwaActive::PotsCwaActive() : PotsCwaState(PotsCwaState::Active)
{
   Debug::ft(PotsCwaActive_ctor);

   BindMsgAnalyzer
      (PotsCwtEventHandler::AcAnalyzeUserMessage, Service::UserPort);
   BindEventHandler
      (PotsCwtEventHandler::AcRelease, PotsCwtEvent::Release);
}

//==============================================================================

fn_name PotsCwbState_ctor = "PotsCwbState.ctor";

PotsCwbState::PotsCwbState(Id stid) : State(PotsCwbServiceId, stid)
{
   Debug::ft(PotsCwbState_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCwbState_dtor = "PotsCwbState.dtor";

PotsCwbState::~PotsCwbState()
{
   Debug::ft(PotsCwbState_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCwbNull_ctor = "PotsCwbNull.ctor";

PotsCwbNull::PotsCwbNull() : PotsCwbState(PotsCwbState::Null)
{
   Debug::ft(PotsCwbNull_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCwbPending_ctor = "PotsCwbPending.ctor";

PotsCwbPending::PotsCwbPending() : PotsCwbState(PotsCwbState::Pending)
{
   Debug::ft(PotsCwbPending_ctor);

   BindMsgAnalyzer
      (PotsCwtEventHandler::PeAnalyzeUserMessage, Service::UserPort);
   BindEventHandler
      (PotsCwtEventHandler::PeAck, PotsCwtEvent::Ack);
   BindEventHandler
      (PotsCwtEventHandler::PeRelease, PotsCwtEvent::Release);
}

//------------------------------------------------------------------------------

fn_name PotsCwbActive_ctor = "PotsCwbActive.ctor";

PotsCwbActive::PotsCwbActive() : PotsCwbState(PotsCwbState::Active)
{
   Debug::ft(PotsCwbActive_ctor);

   BindMsgAnalyzer
      (PotsCwtEventHandler::AcAnalyzeUserMessage, Service::UserPort);
   BindEventHandler
      (PotsCwtEventHandler::AcRelease, PotsCwtEvent::Release);
}

//==============================================================================

fn_name PotsCwtEvent_ctor = "PotsCwtEvent.ctor";

PotsCwtEvent::PotsCwtEvent(Id eid, ServiceSM& owner) : Event(eid, &owner)
{
   Debug::ft(PotsCwtEvent_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCwtEvent_dtor = "PotsCwtEvent.dtor";

PotsCwtEvent::~PotsCwtEvent()
{
   Debug::ft(PotsCwtEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCwtAckEvent_ctor = "PotsCwtAckEvent.ctor";

PotsCwtAckEvent::PotsCwtAckEvent(ServiceSM& owner) : PotsCwtEvent(Ack, owner)
{
   Debug::ft(PotsCwtAckEvent_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCwtAckEvent_dtor = "PotsCwtAckEvent.dtor";

PotsCwtAckEvent::~PotsCwtAckEvent()
{
   Debug::ft(PotsCwtAckEvent_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCwtReleaseEvent_ctor = "PotsCwtReleaseEvent.ctor";

PotsCwtReleaseEvent::PotsCwtReleaseEvent(ServiceSM& owner, Facility::Ind ind) :
   PotsCwtEvent(Release, owner),
   ind_(ind)
{
   Debug::ft(PotsCwtReleaseEvent_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCwtReleaseEvent_dtor = "PotsCwtReleaseEvent.dtor";

PotsCwtReleaseEvent::~PotsCwtReleaseEvent()
{
   Debug::ft(PotsCwtReleaseEvent_dtor);
}

//------------------------------------------------------------------------------

void PotsCwtReleaseEvent::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   PotsCwtEvent::Display(stream, prefix, options);

   stream << prefix << "ind : " << ind_ << CRLF;
}

//==============================================================================

fn_name PotsCwtSsm_ctor = "PotsCwtSsm.ctor";

PotsCwtSsm::PotsCwtSsm(ServiceId sid) : ServiceSM(sid)
{
   Debug::ft(PotsCwtSsm_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCwtSsm_dtor = "PotsCwtSsm.dtor";

PotsCwtSsm::~PotsCwtSsm()
{
   Debug::ft(PotsCwtSsm_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCwtSsm_CalcPort = "PotsCwtSsm.CalcPort";

ServicePortId PotsCwtSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft(PotsCwtSsm_CalcPort);

   return Parent()->CalcPort(ame);
}

//------------------------------------------------------------------------------

fn_name PotsCwtSsm_Cancel = "PotsCwtSsm.Cancel";

void PotsCwtSsm::Cancel()
{
   Debug::ft(PotsCwtSsm_Cancel);

   SetNextState(Null);
}

//------------------------------------------------------------------------------

fn_name PotsCwtSsm_ProcessSap = "PotsCwtSsm.ProcessSap";

EventHandler::Rc PotsCwtSsm::ProcessSap(Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsCwtSsm_ProcessSap);

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

fn_name PotsCwtSsm_ProcessSnp = "PotsCwtSsm.ProcessSnp";

EventHandler::Rc PotsCwtSsm::ProcessSnp(Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsCwtSsm_ProcessSnp);

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

fn_name PotsCwaSsm_ctor = "PotsCwaSsm.ctor";

PotsCwaSsm::PotsCwaSsm() : PotsCwtSsm(PotsCwaServiceId)
{
   Debug::ft(PotsCwaSsm_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCwaSsm_dtor = "PotsCwaSsm.dtor";

PotsCwaSsm::~PotsCwaSsm()
{
   Debug::ft(PotsCwaSsm_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCwaSsm_ProcessInitAck = "PotsCwaSsm.ProcessInitAck";

EventHandler::Rc PotsCwaSsm::ProcessInitAck
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsCwaSsm_ProcessInitAck);

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

fn_name PotsCwaSsm_ProcessInitNack = "PotsCwaSsm.ProcessInitNack";

EventHandler::Rc PotsCwaSsm::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsCwaSsm_ProcessInitNack);

   auto pssm = static_cast< PotsBcSsm* >(Parent());
   auto upsm = PotsCallPsm::Cast(pssm->UPsm());

   upsm->SendFacility(PotsCwmServiceId, Facility::InitiationNack);
   return EventHandler::Suspend;
}

//==============================================================================

fn_name PotsCwbSsm_ctor = "PotsCwbSsm.ctor";

PotsCwbSsm::PotsCwbSsm() : PotsCwtSsm(PotsCwbServiceId),
   sap_(nullptr),
   tid_(NIL_ID)
{
   Debug::ft(PotsCwbSsm_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsCwbSsm_dtor = "PotsCwbSsm.dtor";

PotsCwbSsm::~PotsCwbSsm()
{
   Debug::ft(PotsCwbSsm_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsCwbSsm_Cancel = "PotsCwbSsm.Cancel";

void PotsCwbSsm::Cancel()
{
   Debug::ft(PotsCwbSsm_Cancel);

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
      Debug::SwLog(PotsCwbSsm_ClearTimer, tid_, tid);
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
      Debug::SwLog(PotsCwbSsm_FreeContext, 0, 0);
   }
}

//------------------------------------------------------------------------------

fn_name PotsCwbSsm_ProcessInitAck = "PotsCwbSsm.ProcessInitAck";

EventHandler::Rc PotsCwbSsm::ProcessInitAck
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsCwbSsm_ProcessInitAck);

   auto& init = static_cast< InitiationReqEvent& >(currEvent);
   auto sap = init.GetSapEvent();
   auto pssm = static_cast< PotsBcSsm* >(Parent());
   auto stid = pssm->CurrState();

   if(stid != BcState::SelectingFacility)
   {
      Context::Kill(PotsCwbSsm_ProcessInitAck, stid, 0);
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

fn_name PotsCwbSsm_ProcessInitNack = "PotsCwbSsm.ProcessInitNack";

EventHandler::Rc PotsCwbSsm::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsCwbSsm_ProcessInitNack);

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
      Debug::SwLog(PotsCwbSsm_RestoreContext, 0, 0);
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
      Debug::SwLog(PotsCwbSsm_StartTimer, tid_, tid);

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
      Debug::SwLog(PotsCwbSsm_StopTimer, tid_, tid);
      return;
   }

   upsm->StopTimer(*this, tid_);
   tid_ = NIL_ID;
}

//==============================================================================

fn_name PotsCwtPeAnalyzeUserMessage_ProcessEvent =
   "PotsCwtPeAnalyzeUserMessage.ProcessEvent";

EventHandler::Rc PotsCwtPeAnalyzeUserMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsCwtPeAnalyzeUserMessage_ProcessEvent);

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

fn_name PotsCwtPeAck_ProcessEvent = "PotsCwtPeAck.ProcessEvent";

EventHandler::Rc PotsCwtPeAck::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsCwtPeAck_ProcessEvent);

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

fn_name PotsCwtPeRelease_ProcessEvent = "PotsCwtPeRelease.ProcessEvent";

EventHandler::Rc PotsCwtPeRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsCwtPeRelease_ProcessEvent);

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
            Debug::SwLog(PotsCwtAcAnalyzeUserMessage_ProcessEvent, pfi->ind, 0);
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
      Debug::SwLog(PotsCwtAcRelease_ProcessEvent, ind, 0);
   }

   return Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsCwtPrPresentCall_ProcessEvent = "PotsCwtPrPresentCall.ProcessEvent";

EventHandler::Rc PotsCwtPrPresentCall::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsCwtPrPresentCall_ProcessEvent);

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
