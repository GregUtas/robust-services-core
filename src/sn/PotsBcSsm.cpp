//==============================================================================
//
//  PotsBcSsm.cpp
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
#include <sstream>
#include <string>
#include "Algorithms.h"
#include "BcCause.h"
#include "BcProgress.h"
#include "BcProtocol.h"
#include "Debug.h"
#include "LocalAddress.h"
#include "Log.h"
#include "MsgHeader.h"
#include "PotsCircuit.h"
#include "PotsLogs.h"
#include "PotsProfile.h"
#include "PotsProtocol.h"
#include "PotsStatistics.h"
#include "SbAppIds.h"
#include "SbEvents.h"
#include "Singleton.h"
#include "SsmContext.h"
#include "Switch.h"
#include "TimerProtocol.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace PotsBase
{
fn_name PotsBcSsm_ctor = "PotsBcSsm.ctor";

PotsBcSsm::PotsBcSsm(ServiceId sid, const Message& msg, ProtocolSM& psm) :
   ProxyBcSsm(sid),
   prof_(nullptr),
   tid_(NIL_ID),
   trmt_(nullptr)
{
   Debug::ft(PotsBcSsm_ctor);

   //  MSG's receiving factory distinguishes whether a POTS subscriber is
   //  o originating a call: PSM is a PotsCallPsm
   //  o receiving a call: PSM is a CipPsm
   //  o redirecting a call: PSM is a ProxyBcPsm
   //
   auto fid = msg.Header()->rxAddr.fid;

   switch(fid)
   {
   case PotsCallFactoryId:
   {
      //  Make the POTS PSM an edge PSM and find the subscriber's profile.
      //
      auto& ppsm = static_cast< PotsCallPsm& >(psm);
      auto port = ppsm.TsPort();
      ppsm.MakeEdge(port);

      auto tsw = Singleton< Switch >::Instance();
      auto cct = static_cast< PotsCircuit* >(tsw->GetCircuit(port));
      auto prof = cct->Profile();
      SetProfile(prof);
      SetUPsm(ppsm);
      break;
   }

   case CipTbcFactoryId:
      SetNPsm(static_cast< CipPsm& >(psm));
      break;

   case ProxyCallFactoryId:
      SetUPsm(static_cast< MediaPsm& >(psm));
      break;

   default:
      Debug::SwLog(PotsBcSsm_ctor, "unexpected FactoryId", fid);
   }
}

//------------------------------------------------------------------------------

PotsBcSsm::~PotsBcSsm()
{
   Debug::ftnt("PotsBcSsm.dtor");

   auto upsm = PotsCallPsm::Cast(UPsm());

   if((upsm != nullptr) && (prof_ != nullptr))
   {
      //  This occurs during error recovery, when PsmDeleted has yet to
      //  be invoked because the context is being cleaned up top-down.
      //
      prof_->ClearObjAddr(upsm);
   }
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcSsm::AnalyzeMsg
   (const AnalyzeMsgEvent& ame, Event*& nextEvent)
{
   Debug::ft("PotsBcSsm.AnalyzeMsg");

   bool rel = true;
   auto errval = 0;
   auto sid = ame.Msg()->GetSignal();
   auto stid = CurrState();
   auto pmsg = static_cast< PotsMessage* >(ame.Msg());

   switch(sid)
   {
   case PotsSignal::Offhook:
      //
      //  An offhook can arrive in the CI state when a second offhook is
      //  sent while waiting for dial tone, yet the first offhook was not
      //  rejected by overload controls:
      //
      //  POTS call       POTS circuit
      //      :                      :
      //    NU|<------------offhook1-| queued for a while
      //      :                      :
      //      |-digits?-> <-offhook2-|
      //    CI|<-offhook2   digits?->|
      //
      //  An offhook can arrive in the AC state during glare.  If the POTS
      //  circuit is already offhook when told to apply ringing, it sends
      //  another offhook in case the previous one was rejected by overload
      //  controls:
      //
      //  POTS call       POTS circuit
      //      :                      :
      //    PC|-ring!->   <-offhook1-|
      //      |<-offhook1     ring!->|
      //    AC|<------------offhook2-|
      //
      switch(stid)
      {
      case BcState::CollectingInformation:
      case BcState::Active:
         return EventHandler::Suspend;
      }
      rel = false;
      break;

   case PotsSignal::Digits:
      //
      //  This occurs during race conditions, where the POTS shelf reports
      //  digits just before it receives instructions to stop doing so.  It
      //  could be prevented, in states other than Exception, by enhancing
      //  the POTS shelf software to know when a complete address has been
      //  dialed, after which it would no longer report digits.
      //
      switch(stid)
      {
      case BcState::Exception:
         return EventHandler::Suspend;
      }
      rel = false;
      break;

   case PotsSignal::Alerting:
      //
      //  Alerting arrives in the NU state in this scenario:
      //
      //         POTS call     POTS circuit
      //           :                    :
      //         PC|-ring!------------->|
      //  CIP REL->|         <-alerting-|
      //         NU|-release----------->|
      //           |<-alerting          |
      //
      //  In other states, log the alerting and discard it.  We want to
      //  identify the scenario that causes it, but releasing the call
      //  is too drastic.
      //
      switch(stid)
      {
      case BcState::Null:
         return RaiseReleaseCall(nextEvent, Cause::MessageInvalidForState);
      }
      rel = false;
      break;

   case PotsSignal::Flash:
      //
      //  Log this and discard it.  It occurs if a flash is reported when
      //  no service will respond to one.  Although this isn't an error,
      //  the goal is to enable a flash only when it will have an effect.
      //
      rel = false;
      break;

   case PotsSignal::Onhook:
      //
      //  An onhook arrives in the NU state when overload controls
      //  rejected a previous offhook:
      //
      //  POTS call     POTS circuit
      //      :                    :
      //      |<-----------offhook-| rejected by overload controls
      //      :                    :
      //    NU|<------------onhook-|
      //
      //  An onhook arrives in the PC state in this scenario, in
      //  which ignoring it allows the call to proceed as usual:
      //
      //  POTS call     POTS circuit
      //      :                    :
      //      |<-----------offhook-| rejected by overload controls
      //      :                    :
      //    PC|-ring!->   <-onhook-|
      //      |<-onhook     ring!->|
      //      |<----------alerting-|
      //
      switch(stid)
      {
      case BcState::Null:
         return RaiseReleaseCall(nextEvent, Cause::NormalCallClearing);
      case BcState::PresentingCall:
         return EventHandler::Suspend;
      }
      break;

   case PotsSignal::Facility:
   {
      //  In a basic call, this is only valid when it initiates a service.
      //
      auto pfi = pmsg->FindType< PotsFacilityInfo >(PotsParameter::Facility);

      if(pfi != nullptr)
      {
         if(pfi->ind == Facility::InitiationReq)
         {
            nextEvent = new InitiationReqEvent(*this, pfi->sid);
            return EventHandler::Initiate;
         }

         errval = pfi->ind;
      }
      break;
   }

   case PotsSignal::Progress:
   {
      //  In a basic call, this only occurs during a media update, which
      //  PSMs handle without any service level processing.  If we get
      //  here, some other progress indicator arrived.
      //
      auto ppi = pmsg->FindType< ProgressInfo >(PotsParameter::Progress);
      if(ppi != nullptr) errval = ppi->progress;
      break;
   }

   case PotsSignal::Release:
   {
      //  This occurs when a multiplexer releases a call.
      //
      auto pci = pmsg->FindType< CauseInfo >(PotsParameter::Cause);
      return RaiseReleaseCall(nextEvent, pci->cause);
   }
   }

   //p Including the PotsCircuit state in this log aids debugging, but it
   //  will have to be removed to decouple the POTS shelf and POTS call.
   //
   auto log = Log::Create(PotsLogGroup, PotsCallIcSignal);

   if(log != nullptr)
   {
      *log << Log::Tab << "sig=" << sid << " state=" << stid;
      *log << Log::Tab << " errval=" << errval << " rel=" << rel << CRLF;
      *log << Log::Tab << "trace " << GetContext()->strTrace() << CRLF;
      *log << Log::Tab << Profile()->GetCircuit()->strState();
      Log::Submit(log);
   }

   if(!rel) return EventHandler::Suspend;
   return RaiseReleaseCall(nextEvent, Cause::MessageInvalidForState);
}

//------------------------------------------------------------------------------

fn_name PotsBcSsm_AnalyzeNPsmTimeout = "PotsBcSsm.AnalyzeNPsmTimeout";

EventHandler::Rc PotsBcSsm::AnalyzeNPsmTimeout
   (const TlvMessage& msg, Event*& nextEvent)
{
   Debug::ft(PotsBcSsm_AnalyzeNPsmTimeout);

   auto toi = msg.FindType< TimeoutInfo >(Parameter::Timeout);

   if(toi->owner == this)
   {
      switch(toi->tid)
      {
      case PotsProtocol::AlertingTimeoutId:
         ClearTimer(PotsProtocol::AlertingTimeoutId);
         return RaiseFacilityFailure(nextEvent);
      case PotsProtocol::AnswerTimeoutId:
         ClearTimer(PotsProtocol::AnswerTimeoutId);
         return RaiseLocalNoAnswer(nextEvent);
      }
   }

   Debug::SwLog(PotsBcSsm_AnalyzeNPsmTimeout, "unexpected TimerId", toi->tid);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcSsm::ClearCall(Cause::Ind cause)
{
   Debug::ft("PotsBcSsm.ClearCall");

   auto upsm = PotsCallPsm::Cast(UPsm());

   if((upsm != nullptr) && (upsm->GetState() != ProtocolSM::Idle))
   {
      upsm->SendSignal(PotsSignal::Release);
      upsm->SendCause(cause);
      prof_->SetState(upsm, PotsProfile::Idle);
   }

   return ProxyBcSsm::ClearCall(cause);
}

//------------------------------------------------------------------------------

fn_name PotsBcSsm_ClearTimer = "PotsBcSsm.ClearTimer";

void PotsBcSsm::ClearTimer(TimerId tid)
{
   Debug::ft(PotsBcSsm_ClearTimer);

   if(tid_ != tid)
   {
      Debug::SwLog(PotsBcSsm_ClearTimer, "TimerId mismatch", pack2(tid_, tid));
      return;
   }

   tid_ = NIL_ID;
}

//------------------------------------------------------------------------------

void PotsBcSsm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   ProxyBcSsm::Display(stream, prefix, options);

   stream << prefix << "prof : " << prof_ << CRLF;
   stream << prefix << "tid  : " << tid_ << CRLF;
   stream << prefix << "trmt : " << trmt_ << CRLF;
}

//------------------------------------------------------------------------------

PotsProfile* PotsBcSsm::Profile() const
{
   return prof_;
}

//------------------------------------------------------------------------------

void PotsBcSsm::PsmDeleted(const ProtocolSM& exPsm)
{
   Debug::ft("PotsBcSsm.PsmDeleted");

   auto upsm = PotsCallPsm::Cast(UPsm());

   if(upsm == &exPsm)
   {
      prof_->ClearObjAddr(upsm);
   }

   ProxyBcSsm::PsmDeleted(exPsm);
}

//------------------------------------------------------------------------------

void PotsBcSsm::SetNextSap(TriggerId sap)
{
   Debug::ft("PotsBcSsm.SetNextSap");

   switch(sap)
   {
   case BcTrigger::AuthorizeOriginationSap:
      PotsStatistics::Incr(PotsStatistics::OrigAttempted);
      break;

   case BcTrigger::AuthorizeTerminationSap:
      PotsStatistics::Incr(PotsStatistics::TermAttempted);
      break;

   case BcTrigger::LocalReleaseSap:
      switch(CurrState())
      {
      case BcState::AuthorizingOrigination:
      case BcState::CollectingInformation:
      case BcState::AnalyzingInformation:
      case BcState::SelectingRoute:
      case BcState::AuthorizingCallSetup:
         PotsStatistics::Incr(PotsStatistics::OrigAbandoned);
         break;
      }
      break;

   case BcTrigger::RemoteReleaseSap:
      switch(CurrState())
      {
      case BcState::AuthorizingTermination:
      case BcState::SelectingFacility:
      case BcState::PresentingCall:
      case BcState::TermAlerting:
         PotsStatistics::Incr(PotsStatistics::TermAbandoned);
         break;
      }
      break;
   }

   ProxyBcSsm::SetNextSap(sap);
}

//------------------------------------------------------------------------------

void PotsBcSsm::SetNextSnp(TriggerId snp)
{
   Debug::ft("PotsBcSsm.SetNextSnp");

   switch(snp)
   {
   case BcTrigger::LocalAlertingSnp:
      PotsStatistics::Incr(PotsStatistics::Alerted);
      break;

   case BcTrigger::LocalAnswerSnp:
      PotsStatistics::Incr(PotsStatistics::Answered);
   }

   ProxyBcSsm::SetNextSnp(snp);
}

//------------------------------------------------------------------------------

fn_name PotsBcSsm_SetProfile = "PotsBcSsm.SetProfile";

void PotsBcSsm::SetProfile(PotsProfile* prof)
{
   Debug::ft(PotsBcSsm_SetProfile);

   if(prof == nullptr)
   {
      Debug::SwLog(PotsBcSsm_SetProfile, "null profile", 0);
      return;
   }

   prof_ = prof;
}

//------------------------------------------------------------------------------

fn_name PotsBcSsm_StartTimer = "PotsBcSsm.StartTimer";

void PotsBcSsm::StartTimer(TimerId tid, uint32_t secs)
{
   Debug::ft(PotsBcSsm_StartTimer);

   ProtocolSM* psm;

   if(tid_ != NIL_ID)
   {
      Debug::SwLog(PotsBcSsm_StartTimer, "timer in use", pack2(tid_, tid));

      psm = TimerPsm(tid_);

      if(psm == nullptr)
         Debug::SwLog(PotsBcSsm_StartTimer, "PSM not found", tid_);
      else
         psm->StopTimer(*this, tid_);

      tid_ = NIL_ID;
   }

   psm = TimerPsm(tid);

   if(psm == nullptr)
   {
      Debug::SwLog(PotsBcSsm_StartTimer, "PSM not found", tid);
      return;
   }

   if(psm->StartTimer(secs, *this, tid)) tid_ = tid;
}

//------------------------------------------------------------------------------

fn_name PotsBcSsm_StopTimer = "PotsBcSsm.StopTimer";

void PotsBcSsm::StopTimer(TimerId tid)
{
   Debug::ft(PotsBcSsm_StopTimer);

   if(tid_ != tid)
   {
      Debug::SwLog(PotsBcSsm_StopTimer, "TimerId mismatch", pack2(tid_, tid));
      return;
   }

   auto psm = TimerPsm(tid_);

   if(psm == nullptr)
   {
      Debug::SwLog(PotsBcSsm_StopTimer, "PSM not found", tid_);
      return;
   }

   psm->StopTimer(*this, tid_);
   tid_ = NIL_ID;
}

//------------------------------------------------------------------------------

ProtocolSM* PotsBcSsm::TimerPsm(TimerId tid) const
{
   Debug::ft("PotsBcSsm.TimerPsm");

   switch(tid)
   {
   case PotsProtocol::AlertingTimeoutId:
   case PotsProtocol::AnswerTimeoutId:
      return NPsm();
   }

   return UPsm();
}
}
