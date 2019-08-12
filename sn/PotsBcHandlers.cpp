//==============================================================================
//
//  PotsBcHandlers.cpp
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
fn_name PotsBcNuAnalyzeLocalMessage_ProcessEvent =
   "PotsBcNuAnalyzeLocalMessage.ProcessEvent";

EventHandler::Rc PotsBcNuAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcNuAnalyzeLocalMessage_ProcessEvent);

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

fn_name PotsBcNuOriginate_ProcessEvent = "PotsBcNuOriginate.ProcessEvent";

EventHandler::Rc PotsBcNuOriginate::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcNuOriginate_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.SetModel(BcSsm::ObcModel);
   return pssm.RaiseAuthorizeOrigination(nextEvent);
}

//==============================================================================

fn_name PotsBcAoAnalyzeLocalMessage_ProcessEvent =
   "PotsBcAoAnalyzeLocalMessage.ProcessEvent";

EventHandler::Rc PotsBcAoAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcAoAnalyzeLocalMessage_ProcessEvent);

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

fn_name PotsBcAoAuthorizeOrigination_ProcessEvent =
   "PotsBcAoAuthorizeOrigination.ProcessEvent";

EventHandler::Rc PotsBcAoAuthorizeOrigination::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcAoAuthorizeOrigination_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.RaiseCollectInformation(nextEvent);
}

//------------------------------------------------------------------------------

fn_name PotsBcAoOriginationDenied_ProcessEvent =
   "PotsBcAoOriginationDenied.ProcessEvent";

EventHandler::Rc PotsBcAoOriginationDenied::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcAoOriginationDenied_ProcessEvent);

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

fn_name PotsBcCiCollectInformation_ProcessEvent =
   "PotsBcCiCollectInformation.ProcessEvent";

EventHandler::Rc PotsBcCiCollectInformation::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcCiCollectInformation_ProcessEvent);

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

fn_name PotsBcCiCollectionTimeout_ProcessEvent =
   "PotsBcCiCollectionTimeout.ProcessEvent";

EventHandler::Rc PotsBcCiCollectionTimeout::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcCiCollectionTimeout_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto upsm = PotsCallPsm::Cast(pssm.UPsm());

   upsm->ReportDigits(false);
   return pssm.RaiseReleaseCall(nextEvent, Cause::AddressTimeout);
}

//------------------------------------------------------------------------------

fn_name PotsBcCiLocalInformation_ProcessEvent =
   "PotsBcCiLocalInformation.ProcessEvent";

EventHandler::Rc PotsBcCiLocalInformation::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcCiLocalInformation_ProcessEvent);

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

fn_name PotsBcAiAnalyzeInformation_ProcessEvent =
   "PotsBcAiAnalyzeInformation.ProcessEvent";

EventHandler::Rc PotsBcAiAnalyzeInformation::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcAiAnalyzeInformation_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.AnalyzeInformation(nextEvent);
}

//------------------------------------------------------------------------------

fn_name PotsBcAiInvalidInformation_ProcessEvent =
   "PotsBcAiInvalidInformation.ProcessEvent";

EventHandler::Rc PotsBcAiInvalidInformation::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcAiInvalidInformation_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.RaiseReleaseCall(nextEvent, Cause::InvalidAddress);
}

//==============================================================================

fn_name PotsBcSrSelectRoute_ProcessEvent = "PotsBcSrSelectRoute.ProcessEvent";

EventHandler::Rc PotsBcSrSelectRoute::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcSrSelectRoute_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.SelectRoute(nextEvent);
}

//==============================================================================

fn_name PotsBcAsAuthorizeCallSetup_ProcessEvent =
   "PotsBcAsAuthorizeCallSetup.ProcessEvent";

EventHandler::Rc PotsBcAsAuthorizeCallSetup::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcAsAuthorizeCallSetup_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.RaiseSendCall(nextEvent);
}

//==============================================================================

fn_name PotsBcScAnalyzeLocalMessage_ProcessEvent =
   "PotsBcScAnalyzeLocalMessage.ProcessEvent";

EventHandler::Rc PotsBcScAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcScAnalyzeLocalMessage_ProcessEvent);

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

fn_name PotsBcScSendCall_ProcessEvent = "PotsBcScSendCall.ProcessEvent";

EventHandler::Rc PotsBcScSendCall::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcScSendCall_ProcessEvent);

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

fn_name PotsBcScRemoteBusy_ProcessEvent = "PotsBcScRemoteBusy.ProcessEvent";

EventHandler::Rc PotsBcScRemoteBusy::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcScRemoteBusy_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.RaiseReleaseCall(nextEvent, Cause::UserBusy);
}

//------------------------------------------------------------------------------

fn_name PotsBcScRemoteProgress_ProcessEvent =
   "PotsBcScRemoteProgress.ProcessEvent";

EventHandler::Rc PotsBcScRemoteProgress::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcScRemoteProgress_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.SetNextSnp(BcTrigger::RemoteProgressSnp);
   return Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsBcScRemoteAlerting_ProcessEvent =
   "PotsBcScRemoteAlerting.ProcessEvent";

EventHandler::Rc PotsBcScRemoteAlerting::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcScRemoteAlerting_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.SetNextSnp(BcTrigger::RemoteAlertingSnp);
   pssm.SetNextState(BcState::OrigAlerting);
   return Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsBcScRemoteRelease_ProcessEvent =
   "PotsBcScRemoteRelease.ProcessEvent";

EventHandler::Rc PotsBcScRemoteRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcScRemoteRelease_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto& rre = static_cast< BcRemoteReleaseEvent& >(currEvent);
   auto cause = rre.GetCause();

   pssm.SetNextSnp(BcTrigger::RemoteReleaseSnp);
   return pssm.RaiseReleaseCall(nextEvent, cause);
}

//==============================================================================

fn_name PotsBcOaRemoteNoAnswer_ProcessEvent =
   "PotsBcOaRemoteNoAnswer.ProcessEvent";

EventHandler::Rc PotsBcOaRemoteNoAnswer::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcOaRemoteNoAnswer_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.RaiseReleaseCall(nextEvent, Cause::AnswerTimeout);
}

//==============================================================================

fn_name PotsBcNuTerminate_ProcessEvent = "PotsBcNuTerminate.ProcessEvent";

EventHandler::Rc PotsBcNuTerminate::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcNuTerminate_ProcessEvent);

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

fn_name PotsBcAtAuthorizeTermination_ProcessEvent =
   "PotsBcAtAuthorizeTermination.ProcessEvent";

EventHandler::Rc PotsBcAtAuthorizeTermination::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcAtAuthorizeTermination_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.RaiseSelectFacility(nextEvent);
}

//------------------------------------------------------------------------------

fn_name PotsBcAtTerminationDenied_ProcessEvent =
   "PotsBcAtTerminationDenied.ProcessEvent";

EventHandler::Rc PotsBcAtTerminationDenied::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcAtTerminationDenied_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto& tde = static_cast< BcTerminationDeniedEvent& >(currEvent);

   return pssm.ClearCall(tde.GetCause());
}

//==============================================================================

fn_name PotsBcSfAnalyzeLocalMessage_ProcessEvent =
   "PotsBcSfAnalyzeLocalMessage.ProcessEvent";

EventHandler::Rc PotsBcSfAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcSfAnalyzeLocalMessage_ProcessEvent);

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

fn_name PotsBcSfSelectFacility_ProcessEvent =
   "PotsBcSfSelectFacility.ProcessEvent";

EventHandler::Rc PotsBcSfSelectFacility::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcSfSelectFacility_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   if(pssm.Profile()->GetState() == PotsProfile::Idle)
   {
      return pssm.RaisePresentCall(nextEvent);
   }

   return pssm.RaiseLocalBusy(nextEvent);
}

//------------------------------------------------------------------------------

fn_name PotsBcSfLocalBusy_ProcessEvent = "PotsBcSfLocalBusy.ProcessEvent";

EventHandler::Rc PotsBcSfLocalBusy::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcSfLocalBusy_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.ClearCall(Cause::UserBusy);
}

//==============================================================================

fn_name PotsBcPcAnalyzeLocalMessage_ProcessEvent =
   "PotsBcPcAnalyzeLocalMessage.ProcessEvent";

EventHandler::Rc PotsBcPcAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcPcAnalyzeLocalMessage_ProcessEvent);

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

fn_name PotsBcPcPresentCall_ProcessEvent = "PotsBcPcPresentCall.ProcessEvent";

EventHandler::Rc PotsBcPcPresentCall::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcPcPresentCall_ProcessEvent);

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

fn_name PotsBcPcFacilityFailure_ProcessEvent =
   "PotsBcPcFacilityFailure.ProcessEvent";

EventHandler::Rc PotsBcPcFacilityFailure::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcPcFacilityFailure_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.ClearCall(Cause::AlertingTimeout);
}

//------------------------------------------------------------------------------

fn_name PotsBcPcLocalAlerting_ProcessEvent =
   "PotsBcPcLocalAlerting.ProcessEvent";

EventHandler::Rc PotsBcPcLocalAlerting::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcPcLocalAlerting_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.StartTimer(PotsProtocol::AnswerTimeoutId, PotsProtocol::AnswerTimeout);
   return pssm.HandleLocalAlerting();
}

//------------------------------------------------------------------------------

fn_name PotsBcPcRemoteRelease_ProcessEvent =
   "PotsBcPcRemoteRelease.ProcessEvent";

EventHandler::Rc PotsBcPcRemoteRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcPcRemoteRelease_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.StopTimer(PotsProtocol::AlertingTimeoutId);
   return pssm.HandleRemoteRelease(currEvent);
}

//==============================================================================

fn_name PotsBcTaAnalyzeLocalMessage_ProcessEvent =
   "PotsBcTaAnalyzeLocalMessage.ProcessEvent";

EventHandler::Rc PotsBcTaAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcTaAnalyzeLocalMessage_ProcessEvent);

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

fn_name PotsBcTaLocalNoAnswer_ProcessEvent =
   "PotsBcTaLocalNoAnswer.ProcessEvent";

EventHandler::Rc PotsBcTaLocalNoAnswer::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcTaLocalNoAnswer_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   return pssm.ClearCall(Cause::AnswerTimeout);
}

//------------------------------------------------------------------------------

fn_name PotsBcTaRemoteRelease_ProcessEvent =
   "PotsBcTaRemoteRelease.ProcessEvent";

EventHandler::Rc PotsBcTaRemoteRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcTaRemoteRelease_ProcessEvent);

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

fn_name PotsBcAcLocalSuspend_ProcessEvent = "PotsBcAcLocalSuspend.ProcessEvent";

EventHandler::Rc PotsBcAcLocalSuspend::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcAcLocalSuspend_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.StartTimer
      (PotsProtocol::SuspendTimeoutId, PotsProtocol::SuspendTimeout);
   pssm.BuildCipCpg(Progress::Suspend);
   pssm.SetNextState(BcState::LocalSuspending);
   return Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsBcAcRemoteSuspend_ProcessEvent =
   "PotsBcAcRemoteSuspend.ProcessEvent";

EventHandler::Rc PotsBcAcRemoteSuspend::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcAcRemoteSuspend_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.SetNextState(BcState::RemoteSuspending);
   return Suspend;
}

//==============================================================================

fn_name PotsBcLsLocalResume_ProcessEvent = "PotsBcLsLocalResume.ProcessEvent";

EventHandler::Rc PotsBcLsLocalResume::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcLsLocalResume_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   PotsStatistics::Incr(PotsStatistics::Resumed);
   pssm.StopTimer(PotsProtocol::SuspendTimeoutId);
   pssm.BuildCipCpg(Progress::Resume);
   pssm.SetNextState(BcState::Active);
   return Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsBcLsRemoteRelease_ProcessEvent =
   "PotsBcLsRemoteRelease.ProcessEvent";

EventHandler::Rc PotsBcLsRemoteRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcLsRemoteRelease_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.StopTimer(PotsProtocol::SuspendTimeoutId);
   return pssm.HandleRemoteRelease(currEvent);
}

//==============================================================================

fn_name PotsBcRsRemoteResume_ProcessEvent = "PotsBcRsRemoteResume.ProcessEvent";

EventHandler::Rc PotsBcRsRemoteResume::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcRsRemoteResume_ProcessEvent);

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

fn_name PotsBcExApplyTreatment_ProcessEvent =
   "PotsBcExApplyTreatment.ProcessEvent";

EventHandler::Rc PotsBcExApplyTreatment::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcExApplyTreatment_ProcessEvent);

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

fn_name PotsBcLocalAnswer_ProcessEvent = "PotsBcLocalAnswer.ProcessEvent";

EventHandler::Rc PotsBcLocalAnswer::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcLocalAnswer_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto upsm = PotsCallPsm::Cast(pssm.UPsm());

   upsm->ApplyRinging(false);
   return pssm.HandleLocalAnswer();
}

//------------------------------------------------------------------------------

fn_name PotsBcRemoteAnswer_ProcessEvent = "PotsBcRemoteAnswer.ProcessEvent";

EventHandler::Rc PotsBcRemoteAnswer::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcRemoteAnswer_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.SetNextSnp(BcTrigger::RemoteAnswerSnp);
   pssm.SetNextState(BcState::Active);
   return Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsBcLocalRelease_ProcessEvent = "PotsBcLocalRelease.ProcessEvent";

EventHandler::Rc PotsBcLocalRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcLocalRelease_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto& lre = static_cast< BcLocalReleaseEvent& >(currEvent);

   pssm.SetNextSnp(BcTrigger::LocalReleaseSnp);
   return pssm.ClearCall(lre.GetCause());
}

//------------------------------------------------------------------------------

fn_name PotsBcReleaseCall_ProcessEvent = "PotsBcReleaseCall.ProcessEvent";

EventHandler::Rc PotsBcReleaseCall::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcReleaseCall_ProcessEvent);

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

fn_name PotsBcReleaseUser_ProcessEvent = "PotsBcReleaseUser.ProcessEvent";

EventHandler::Rc PotsBcReleaseUser::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsBcReleaseUser_ProcessEvent);

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
