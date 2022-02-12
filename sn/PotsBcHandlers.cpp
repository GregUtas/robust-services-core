//==============================================================================
//
//  PotsBcHandlers.cpp
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
#include "PotsBcHandlers.h"
#include "BcAddress.h"
#include "BcCause.h"
#include "BcProgress.h"
#include "BcProtocol.h"
#include "BcRouting.h"
#include "BcSessions.h"
#include "Debug.h"
#include "PotsCircuit.h"
#include "PotsProfile.h"
#include "PotsProfileRegistry.h"
#include "PotsProtocol.h"
#include "PotsSessions.h"
#include "PotsStatistics.h"
#include "PotsTreatmentRegistry.h"
#include "PotsTreatments.h"
#include "ProxyBcSessions.h"
#include "SbAppIds.h"
#include "SbEvents.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "TimerProtocol.h"
#include "Tones.h"

using namespace CallBase;
using namespace MediaBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
EventHandler::Rc PotsBcNuAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcNuAnalyzeLocalMessage.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto prof = pssm.Profile();
   auto state = prof->GetState();
   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto sid = ame.Msg()->GetSignal();
   auto upsm = PotsCallPsm::Cast(pssm.UPsm());

   switch(state)
   {
   case PotsProfile::Active:
      if(sid == PotsSignal::Offhook)
      {
         nextEvent = new BcOriginateEvent(ssm);
         return Continue;
      }
      break;

   case PotsProfile::Lockout:
      if(sid == PotsSignal::Onhook)
      {
         upsm->SendSignal(PotsSignal::Release);
         upsm->SendCause(Cause::NormalCallClearing);
         prof->SetState(upsm, PotsProfile::Idle);
         return Suspend;
      }
      break;
   }

   return pssm.AnalyzeMsg(ame, nextEvent);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcNuOriginate::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcNuOriginate.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.SetModel(BcSsm::ObcModel);
   return pssm.RaiseAuthorizeOrigination(nextEvent);
}

//==============================================================================

EventHandler::Rc PotsBcAoAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcAoAnalyzeLocalMessage.ProcessEvent");

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto sid = ame.Msg()->GetSignal();
   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   if(sid == PotsSignal::Onhook)
   {
      return pssm.RaiseLocalRelease(nextEvent, Cause::NormalCallClearing);
   }

   return pssm.AnalyzeMsg(ame, nextEvent);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcAoAuthorizeOrigination::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcAoAuthorizeOrigination.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.RaiseCollectInformation(nextEvent);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcAoOriginationDenied::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcAoOriginationDenied.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto& ode = static_cast< BcOriginationDeniedEvent& >(currEvent);

   return pssm.RaiseReleaseCall(nextEvent, ode.GetCause());
}

//==============================================================================

fn_name PotsBcCiAnalyzeLocalMessage_ProcessEvent =
   "PotsBcCiAnalyzeLocalMessage.ProcessEvent";

EventHandler::Rc PotsBcCiAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcCiAnalyzeLocalMessage_ProcessEvent);

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto sid = ame.Msg()->GetSignal();
   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   switch(sid)
   {
   case PotsSignal::Digits:
   {
      auto pmsg = static_cast< Pots_UN_Message* >(ame.Msg());

      pssm.StopTimer(PotsProtocol::CollectionTimeoutId);

      auto digs = pmsg->FindType< DigitString >(PotsParameter::Digits);
      auto dsrc = pssm.DialedDigits().AddDigits(*digs);

      if((dsrc == DigitString::IllegalDigit) ||
         (dsrc == DigitString::Overflow))
      {
         return pssm.RaiseCollectionTimeout(nextEvent);
      }

      return pssm.RaiseLocalInformation(nextEvent);
   }

   case PotsSignal::Onhook:
      pssm.StopTimer(PotsProtocol::CollectionTimeoutId);
      return pssm.RaiseLocalRelease(nextEvent, Cause::NormalCallClearing);

   case Signal::Timeout:
      auto tmsg = static_cast< TlvMessage* >(ame.Msg());
      auto toi = tmsg->FindType< TimeoutInfo >(Parameter::Timeout);

      if((toi->owner == &pssm) &&
         (toi->tid == PotsProtocol::CollectionTimeoutId))
      {
         pssm.ClearTimer(PotsProtocol::CollectionTimeoutId);
         return pssm.RaiseCollectionTimeout(nextEvent);
      }

      Debug::SwLog(PotsBcCiAnalyzeLocalMessage_ProcessEvent,
         "unexpected TimerId", toi->tid);
      return Suspend;
   }

   return pssm.AnalyzeMsg(ame, nextEvent);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcCiCollectInformation::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcCiCollectInformation.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto upsm = PotsCallPsm::Cast(pssm.UPsm());

   if(pssm.DialedDigits().Empty())
   {
      upsm->ReportDigits(true);
      upsm->SetOgTone(Tone::Dial);
      pssm.StartTimer
         (PotsProtocol::CollectionTimeoutId, PotsProtocol::FirstDigitTimeout);
   }
   else
   {
      upsm->SetOgTone(Tone::Silence);
      pssm.StartTimer
         (PotsProtocol::CollectionTimeoutId, PotsProtocol::InterDigitTimeout);
   }

   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcCiCollectionTimeout::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcCiCollectionTimeout.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto upsm = PotsCallPsm::Cast(pssm.UPsm());

   upsm->ReportDigits(false);
   return pssm.RaiseReleaseCall(nextEvent, Cause::AddressTimeout);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcCiLocalInformation::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcCiLocalInformation.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   if(pssm.DialedDigits().IsCompleteAddress())
   {
      auto upsm = PotsCallPsm::Cast(pssm.UPsm());

      upsm->ReportDigits(false);
      upsm->SetOgTone(Tone::Silence);
      return pssm.RaiseAnalyzeInformation(nextEvent);
   }

   return pssm.RaiseCollectInformation(nextEvent);
}

//==============================================================================

EventHandler::Rc PotsBcAiAnalyzeInformation::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcAiAnalyzeInformation.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.AnalyzeInformation(nextEvent);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcAiInvalidInformation::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcAiInvalidInformation.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.RaiseReleaseCall(nextEvent, Cause::InvalidAddress);
}

//==============================================================================

EventHandler::Rc PotsBcSrSelectRoute::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcSrSelectRoute.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.SelectRoute(nextEvent);
}

//==============================================================================

EventHandler::Rc PotsBcAsAuthorizeCallSetup::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcAsAuthorizeCallSetup.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.RaiseSendCall(nextEvent);
}

//==============================================================================

EventHandler::Rc PotsBcScAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcScAnalyzeLocalMessage.ProcessEvent");

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto sid = ame.Msg()->GetSignal();
   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   if(sid == PotsSignal::Onhook)
   {
      return pssm.RaiseLocalRelease(nextEvent, Cause::NormalCallClearing);
   }

   return pssm.AnalyzeMsg(ame, nextEvent);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcScSendCall::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcScSendCall.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto iam = pssm.BuildCipIam();

   if(iam == nullptr)
   {
      return pssm.RaiseReleaseCall(nextEvent, Cause::TemporaryFailure);
   }

   iam->AddAddress(pssm.Profile()->GetDN(), CipParameter::Calling);
   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcScRemoteBusy::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcScRemoteBusy.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.RaiseReleaseCall(nextEvent, Cause::UserBusy);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcScRemoteProgress::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcScRemoteProgress.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.SetNextSnp(BcTrigger::RemoteProgressSnp);
   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcScRemoteAlerting::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcScRemoteAlerting.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.SetNextSnp(BcTrigger::RemoteAlertingSnp);
   pssm.SetNextState(BcState::OrigAlerting);
   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcScRemoteRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcScRemoteRelease.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto& rre = static_cast< BcRemoteReleaseEvent& >(currEvent);
   auto cause = rre.GetCause();

   pssm.SetNextSnp(BcTrigger::RemoteReleaseSnp);
   return pssm.RaiseReleaseCall(nextEvent, cause);
}

//==============================================================================

EventHandler::Rc PotsBcOaRemoteNoAnswer::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcOaRemoteNoAnswer.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.RaiseReleaseCall(nextEvent, Cause::AnswerTimeout);
}

//==============================================================================

EventHandler::Rc PotsBcNuTerminate::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcNuTerminate.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto npsm = pssm.NPsm();
   auto cmsg = static_cast< CipMessage* >(npsm->FirstRcvdMsg());
   auto rte = cmsg->FindType< RouteResult >(CipParameter::Route);
   auto reg = Singleton< PotsProfileRegistry >::Instance();
   auto prof = reg->Profile(rte->identifier);

   if(prof == nullptr)
   {
      return pssm.RaiseReleaseCall(nextEvent, Cause::ExchangeRoutingError);
   }

   //  Save the incoming IAM.  It contains data that may be required during
   //  subsequent transactions.  Currently, this is only necessary for CFN,
   //  but a complete POTS call server would also need it in many other cases.
   //
   pssm.SetProfile(prof);
   pssm.SetModel(BcSsm::TbcModel);
   cmsg->Save();
   return pssm.RaiseAuthorizeTermination(nextEvent);
}

//==============================================================================

EventHandler::Rc PotsBcAtAuthorizeTermination::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcAtAuthorizeTermination.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.RaiseSelectFacility(nextEvent);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcAtTerminationDenied::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcAtTerminationDenied.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto& tde = static_cast< BcTerminationDeniedEvent& >(currEvent);

   return pssm.ClearCall(tde.GetCause());
}

//==============================================================================

EventHandler::Rc PotsBcSfAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcSfAnalyzeLocalMessage.ProcessEvent");

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   //  This can occur during a service such as call waiting, which sends its
   //  first message (to a multiplexer) during the Selecting Facility state
   //  to see if a call can be presented.  If the multiplexer traps, it sends
   //  a Release message that arrives in this state, and the modifier might
   //  pass the message back to basic call for handling.
   //
   return pssm.AnalyzeMsg(ame, nextEvent);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcSfSelectFacility::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcSfSelectFacility.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   if(pssm.Profile()->GetState() == PotsProfile::Idle)
   {
      return pssm.RaisePresentCall(nextEvent);
   }

   return pssm.RaiseLocalBusy(nextEvent);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcSfLocalBusy::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcSfLocalBusy.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.ClearCall(Cause::UserBusy);
}

//==============================================================================

EventHandler::Rc PotsBcPcAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcPcAnalyzeLocalMessage.ProcessEvent");

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto sid = ame.Msg()->GetSignal();
   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   switch(sid)
   {
   case PotsSignal::Alerting:
      pssm.StopTimer(PotsProtocol::AlertingTimeoutId);
      return pssm.RaiseLocalAlerting(nextEvent);

   case PotsSignal::Offhook:
      pssm.StopTimer(PotsProtocol::AlertingTimeoutId);
      return pssm.RaiseLocalAnswer(nextEvent);
   }

   return pssm.AnalyzeMsg(ame, nextEvent);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcPcPresentCall::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcPcPresentCall.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto prof = pssm.Profile();
   auto npsm = pssm.NPsm();
   auto port = prof->GetCircuit()->TsPort();

   auto upsm = new PotsCallPsm(port);

   upsm->MakeEdge(port);
   pssm.SetUPsm(*upsm);

   npsm->EnableMedia(*upsm);
   upsm->ApplyRinging(true);
   pssm.StartTimer
      (PotsProtocol::AlertingTimeoutId, PotsProtocol::AlertingTimeout);
   pssm.BuildCipCpg(Progress::EndOfSelection);
   pssm.SetNextSnp(BcTrigger::PresentCallSnp);
   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcPcFacilityFailure::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcPcFacilityFailure.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.ClearCall(Cause::AlertingTimeout);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcPcLocalAlerting::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcPcLocalAlerting.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.StartTimer(PotsProtocol::AnswerTimeoutId, PotsProtocol::AnswerTimeout);
   return pssm.HandleLocalAlerting();
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcPcRemoteRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcPcRemoteRelease.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.StopTimer(PotsProtocol::AlertingTimeoutId);
   return pssm.HandleRemoteRelease(currEvent);
}

//==============================================================================

EventHandler::Rc PotsBcTaAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcTaAnalyzeLocalMessage.ProcessEvent");

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto sid = ame.Msg()->GetSignal();
   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   if(sid == PotsSignal::Offhook)
   {
      pssm.StopTimer(PotsProtocol::AnswerTimeoutId);
      return pssm.RaiseLocalAnswer(nextEvent);
   }

   return pssm.AnalyzeMsg(ame, nextEvent);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcTaLocalNoAnswer::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcTaLocalNoAnswer.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.ClearCall(Cause::AnswerTimeout);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcTaRemoteRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcTaRemoteRelease.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.StopTimer(PotsProtocol::AnswerTimeoutId);
   return pssm.HandleRemoteRelease(currEvent);
}

//==============================================================================

fn_name PotsBcAcAnalyzeLocalMessage_ProcessEvent =
   "PotsBcAcAnalyzeLocalMessage.ProcessEvent";

EventHandler::Rc PotsBcAcAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcAcAnalyzeLocalMessage_ProcessEvent);

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto sid = ame.Msg()->GetSignal();
   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   switch(sid)
   {
   case PotsSignal::Onhook:
      if(pssm.GetModel() == BcSsm::ObcModel)
         return pssm.RaiseLocalRelease(nextEvent, Cause::NormalCallClearing);
      if(pssm.CurrState() != BcState::LocalSuspending)
         return pssm.RaiseLocalSuspend(nextEvent);
      break;

   case PotsSignal::Offhook:
      if(pssm.CurrState() == BcState::LocalSuspending)
         return pssm.RaiseLocalResume(nextEvent);
      break;

   case Signal::Timeout:
      auto tmsg = static_cast< TlvMessage* >(ame.Msg());
      auto toi = tmsg->FindType< TimeoutInfo >(Parameter::Timeout);

      if((toi->owner == &pssm) &&
         (toi->tid == PotsProtocol::SuspendTimeoutId))
      {
         pssm.ClearTimer(PotsProtocol::SuspendTimeoutId);
         return pssm.RaiseLocalRelease(nextEvent, Cause::NormalCallClearing);
      }

      Debug::SwLog(PotsBcAcAnalyzeLocalMessage_ProcessEvent,
         "unexpected TimerId", toi->tid);
      return Suspend;
   }

   return pssm.AnalyzeMsg(ame, nextEvent);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcAcLocalSuspend::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcAcLocalSuspend.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.StartTimer
      (PotsProtocol::SuspendTimeoutId, PotsProtocol::SuspendTimeout);
   pssm.BuildCipCpg(Progress::Suspend);
   pssm.SetNextState(BcState::LocalSuspending);
   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcAcRemoteSuspend::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcAcRemoteSuspend.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.SetNextState(BcState::RemoteSuspending);
   return Suspend;
}

//==============================================================================

EventHandler::Rc PotsBcLsLocalResume::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcLsLocalResume.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   PotsStatistics::Incr(PotsStatistics::Resumed);
   pssm.StopTimer(PotsProtocol::SuspendTimeoutId);
   pssm.BuildCipCpg(Progress::Resume);
   pssm.SetNextState(BcState::Active);
   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcLsRemoteRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcLsRemoteRelease.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.StopTimer(PotsProtocol::SuspendTimeoutId);
   return pssm.HandleRemoteRelease(currEvent);
}

//==============================================================================

EventHandler::Rc PotsBcRsRemoteResume::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcRsRemoteResume.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.SetNextState(BcState::Active);
   return Suspend;
}

//==============================================================================

fn_name PotsBcExAnalyzeLocalMessage_ProcessEvent =
   "PotsBcExAnalyzeLocalMessage.ProcessEvent";

EventHandler::Rc PotsBcExAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcExAnalyzeLocalMessage_ProcessEvent);

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto sid = ame.Msg()->GetSignal();
   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   switch(sid)
   {
   case PotsSignal::Onhook:
      pssm.StopTimer(PotsProtocol::TreatmentTimeoutId);
      return pssm.RaiseLocalRelease(nextEvent, Cause::NormalCallClearing);

   case Signal::Timeout:
      auto tmsg = static_cast< TlvMessage* >(ame.Msg());
      auto toi = tmsg->FindType< TimeoutInfo >(Parameter::Timeout);

      if((toi->owner == &pssm) &&
         (toi->tid == PotsProtocol::TreatmentTimeoutId))
      {
         pssm.ClearTimer(PotsProtocol::TreatmentTimeoutId);
         return pssm.RaiseApplyTreatment(nextEvent, Cause::NilInd);
      }

      Debug::SwLog(PotsBcExAnalyzeLocalMessage_ProcessEvent,
         "unexpected TimerId", toi->tid);
      return Suspend;
   }

   return pssm.AnalyzeMsg(ame, nextEvent);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcExApplyTreatment::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcExApplyTreatment.ProcessEvent");

   auto& ate = static_cast< BcApplyTreatmentEvent& >(currEvent);
   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto upsm = PotsCallPsm::Cast(pssm.UPsm());
   auto trmt = pssm.GetTreatment();

   if(trmt == nullptr)
   {
      auto cause = ate.GetCause();
      upsm->SendCause(cause);
      PotsStatistics::IncrCause(cause);

      auto reg = Singleton< PotsTreatmentRegistry >::Instance();
      auto tq = reg->CauseToTreatmentQ(cause);
      if(tq == nullptr) tq = reg->CauseToTreatmentQ(Cause::TemporaryFailure);
      trmt = tq->FirstTreatment();
   }
   else
   {
      trmt = trmt->NextTreatment();
   }

   if(trmt != nullptr)
   {
      pssm.SetTreatment(trmt);
      return trmt->ApplyTreatment(ate);
   }

   return pssm.RaiseReleaseCall(nextEvent, Cause::ExchangeRoutingError);
}

//==============================================================================

EventHandler::Rc PotsBcLocalAnswer::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcLocalAnswer.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto upsm = PotsCallPsm::Cast(pssm.UPsm());

   upsm->ApplyRinging(false);
   return pssm.HandleLocalAnswer();
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcRemoteAnswer::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcRemoteAnswer.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.SetNextSnp(BcTrigger::RemoteAnswerSnp);
   pssm.SetNextState(BcState::Active);
   return Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcLocalRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcLocalRelease.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto& lre = static_cast< BcLocalReleaseEvent& >(currEvent);

   pssm.SetNextSnp(BcTrigger::LocalReleaseSnp);
   return pssm.ClearCall(lre.GetCause());
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcReleaseCall::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcReleaseCall.ProcessEvent");

   auto& cte = static_cast< BcReleaseCallEvent& >(currEvent);
   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto upsm = pssm.UPsm();
   auto stid = pssm.CurrState();

   if((upsm != nullptr) && (upsm->GetState() != ProtocolSM::Idle))
   {
      switch(stid)
      {
      case BcState::AuthorizingOrigination:
      case BcState::CollectingInformation:
      case BcState::AnalyzingInformation:
      case BcState::SelectingRoute:
      case BcState::AuthorizingCallSetup:
      case BcState::SendingCall:
      case BcState::OrigAlerting:
      case BcState::Active:
      case BcState::RemoteSuspending:
      case BcState::Disconnecting:
         return pssm.RaiseApplyTreatment(nextEvent, cte.GetCause());
      }
   }

   return pssm.ClearCall(cte.GetCause());
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsBcReleaseUser::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsBcReleaseUser.ProcessEvent");

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto upsm = PotsCallPsm::Cast(pssm.UPsm());

   if(upsm == nullptr) return Suspend;

   if(pssm.CurrState() == BcState::TermAlerting)
   {
      //  Continue to apply ringback:
      //  o If the NPSM has no peer media PSM, or if its peer is the UPSM
      //    that is about to be released, apply ringback on the NPSM.
      //  o If the peer is another PSM, apply ringback in the usual way.
      //
      auto npsm = pssm.NPsm();
      auto peer = npsm->GetOgPsm();

      if((peer == nullptr) || (peer == upsm))
         npsm->SetOgTone(Tone::Ringback);
      else
         peer->SetIcTone(Tone::Ringback);
   }

   auto& rue = static_cast< ProxyBcReleaseUserEvent& >(currEvent);

   upsm->SendSignal(PotsSignal::Release);
   upsm->SendCause(rue.GetCause());
   pssm.SetNextSnp(ProxyBcTrigger::UserReleasedSnp);
   pssm.MorphToService(PotsProxyServiceId);

   return Suspend;
}
}
