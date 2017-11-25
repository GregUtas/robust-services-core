//==============================================================================
//
//  PotsProxyHandlers.cpp
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
#include "PotsProxyHandlers.h"
#include "BcAddress.h"
#include "BcCause.h"
#include "BcProgress.h"
#include "BcProtocol.h"
#include "BcSessions.h"
#include "Context.h"
#include "Debug.h"
#include "PotsProfile.h"
#include "PotsProfileRegistry.h"
#include "PotsSessions.h"
#include "PotsStatistics.h"
#include "ProxyBcSessions.h"
#include "SbEvents.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "Tones.h"

using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
fn_name PotsProxyNuAnalyzeLocalMessage_ProcessEvent =
   "PotsProxyNuAnalyzeLocalMessage.ProcessEvent";

EventHandler::Rc PotsProxyNuAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyNuAnalyzeLocalMessage_ProcessEvent);

   auto& ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto msg = static_cast< CipMessage* >(ame.Msg());
   auto sid = msg->GetSignal();
   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto cause = Cause::MessageInvalidForState;

   if(sid == CipSignal::IAM)
   {
      auto clg = msg->FindType< DigitString >(CipParameter::Calling);
      auto reg = Singleton< PotsProfileRegistry >::Instance();
      auto prof = reg->Profile(clg->ToDN());

      if(prof != nullptr)
      {
         pssm.SetProfile(prof);
         msg->Save();
         nextEvent = new BcOriginateEvent(ssm);
         return Continue;
      }

      cause = Cause::UnallocatedNumber;
   }

   Debug::SwErr(PotsProxyNuAnalyzeLocalMessage_ProcessEvent, sid, cause);
   return pssm.RaiseReleaseCall(nextEvent, cause);
}

//------------------------------------------------------------------------------

fn_name PotsProxyNuOriginate_ProcessEvent = "PotsProxyNuOriginate.ProcessEvent";

EventHandler::Rc PotsProxyNuOriginate::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyNuOriginate_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   pssm.SetModel(BcSsm::ObcModel);
   PotsStatistics::Incr(PotsStatistics::ProxyAttempted);
   return pssm.RaiseAuthorizeOrigination(nextEvent);
}

//==============================================================================

fn_name PotsProxyCiCollectInformation_ProcessEvent =
   "PotsProxyCiCollectInformation.ProcessEvent";

EventHandler::Rc PotsProxyCiCollectInformation::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyCiCollectInformation_ProcessEvent);

   auto msg = static_cast< CipMessage* >(Context::ContextMsg());
   auto sid = msg->GetSignal();
   auto& pssm = static_cast< PotsBcSsm& >(ssm);

   if(sid == CipSignal::IAM)
   {
      auto cld = msg->FindType< DigitString >(CipParameter::Called);

      pssm.DialedDigits().AddDigits(*cld);
      return pssm.RaiseAnalyzeInformation(nextEvent);
   }

   Debug::SwErr(PotsProxyNuAnalyzeLocalMessage_ProcessEvent, sid, 0);
   return pssm.RaiseReleaseCall(nextEvent, Cause::TemporaryFailure);
}

//==============================================================================

fn_name PotsProxyScAnalyzeLocalMessage_ProcessEvent =
   "PotsProxyScAnalyzeLocalMessage.ProcessEvent";

EventHandler::Rc PotsProxyScAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyScAnalyzeLocalMessage_ProcessEvent);

   auto&         ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto          msg = static_cast< CipMessage* >(ame.Msg());
   auto          sid = msg->GetSignal();
   auto&         pssm = static_cast< PotsBcSsm& >(ssm);
   ProgressInfo* cpi;
   CauseInfo*    cci;

   switch(sid)
   {
   case CipSignal::CPG:
      //
      //  Progress currently occurs only during a media update, which PSMs
      //  handle without any service level processing.  If we get here,
      //  some other progress indicator arrived.
      //
      cpi = msg->FindType< ProgressInfo >(CipParameter::Progress);
      Debug::SwErr
         (PotsProxyScAnalyzeLocalMessage_ProcessEvent, cpi->progress, 1);
      break;

   case CipSignal::REL:
      cci = msg->FindType< CauseInfo >(CipParameter::Cause);
      return pssm.RaiseReleaseCall(nextEvent, cci->cause);

   default:
      Debug::SwErr(PotsProxyScAnalyzeLocalMessage_ProcessEvent, sid, 0);
   }

   return pssm.RaiseReleaseCall(nextEvent, Cause::MessageInvalidForState);
}

//------------------------------------------------------------------------------

fn_name PotsProxyScSendCall_ProcessEvent = "PotsProxyScSendCall.ProcessEvent";

EventHandler::Rc PotsProxyScSendCall::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyScSendCall_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto ogIam = pssm.BuildCipIam();

   if(ogIam == nullptr)
   {
      return pssm.RaiseReleaseCall(nextEvent, Cause::TemporaryFailure);
   }

   ogIam->AddAddress(pssm.Profile()->GetDN(), CipParameter::Calling);

   auto upsm = static_cast< ProxyBcPsm* >(pssm.UPsm());
   auto icIam = upsm->FindRcvdMsg(CipSignal::IAM);

   if(icIam == nullptr)
   {
      return pssm.RaiseReleaseCall(nextEvent, Cause::TemporaryFailure);
   }

   ogIam->CopyType< DigitString >(*icIam, CipParameter::OriginalCalling);
   ogIam->CopyType< DigitString >(*icIam, CipParameter::OriginalCalled);

   return Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsProxyScRemoteProgress_ProcessEvent =
   "PotsProxyScRemoteProgress.ProcessEvent";

EventHandler::Rc PotsProxyScRemoteProgress::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyScRemoteProgress_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto upsm = pssm.FirstProxy();

   pssm.Relay(*upsm);
   pssm.SetNextSnp(BcTrigger::RemoteProgressSnp);
   return Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsProxyScRemoteAlerting_ProcessEvent =
   "PotsProxyScRemoteAlerting.ProcessEvent";

EventHandler::Rc PotsProxyScRemoteAlerting::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyScRemoteAlerting_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto upsm = pssm.FirstProxy();

   pssm.Relay(*upsm);
   pssm.SetNextSnp(BcTrigger::RemoteAlertingSnp);
   pssm.SetNextState(BcState::OrigAlerting);
   return Suspend;
}

//==============================================================================

fn_name PotsProxyPcAnalyzeLocalMessage_ProcessEvent =
   "PotsProxyPcAnalyzeLocalMessage.ProcessEvent";

EventHandler::Rc PotsProxyPcAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyPcAnalyzeLocalMessage_ProcessEvent);

   auto&         ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto          msg = static_cast< CipMessage* >(ame.Msg());
   auto          sid = msg->GetSignal();
   auto&         pssm = static_cast< PotsBcSsm& >(ssm);
   ProgressInfo* cpi;
   CauseInfo*    cci;

   switch(sid)
   {
   case CipSignal::CPG:
      cpi = msg->FindType< ProgressInfo >(CipParameter::Progress);
      switch(cpi->progress)
      {
      case Progress::EndOfSelection:
         if(pssm.NPsm()->GetState() == CipPsm::IamRcvd)
            return pssm.RaiseLocalProgress(nextEvent, cpi->progress);
         else
            return Suspend;
      case Progress::Alerting:
         return pssm.RaiseLocalAlerting(nextEvent);
      default:
         Debug::SwErr
            (PotsProxyPcAnalyzeLocalMessage_ProcessEvent, cpi->progress, 1);
      }
      break;

   case CipSignal::ANM:
      return pssm.RaiseLocalAnswer(nextEvent);

   case CipSignal::REL:
      cci = msg->FindType< CauseInfo >(CipParameter::Cause);
      return pssm.RaiseLocalRelease(nextEvent, cci->cause);

   default:
      Debug::SwErr(PotsProxyPcAnalyzeLocalMessage_ProcessEvent, sid, 0);
   }

   return pssm.RaiseReleaseCall(nextEvent, Cause::MessageInvalidForState);
}

//------------------------------------------------------------------------------

fn_name PotsProxyPcLocalProgress_ProcessEvent =
   "PotsProxyPcLocalProgress.ProcessEvent";

EventHandler::Rc PotsProxyPcLocalProgress::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyPcLocalProgress_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto npsm = pssm.NPsm();

   pssm.SetNextSnp(BcTrigger::LocalProgressSnp);
   pssm.Relay(*npsm);
   return Suspend;
}

//==============================================================================

fn_name PotsProxyTaAnalyzeLocalMessage_ProcessEvent =
   "PotsProxyTaAnalyzeLocalMessage.ProcessEvent";

EventHandler::Rc PotsProxyTaAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyTaAnalyzeLocalMessage_ProcessEvent);

   auto&         ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto          msg = static_cast< CipMessage* >(ame.Msg());
   auto          sid = msg->GetSignal();
   auto&         pssm = static_cast< PotsBcSsm& >(ssm);
   ProgressInfo* cpi;
   CauseInfo*    cci;

   switch(sid)
   {
   case CipSignal::CPG:
      cpi = msg->FindType< ProgressInfo >(CipParameter::Progress);
      switch(cpi->progress)
      {
      case Progress::EndOfSelection:
         return Suspend;
      case Progress::Alerting:
         return pssm.RaiseLocalAlerting(nextEvent);
      default:
         Debug::SwErr
            (PotsProxyTaAnalyzeLocalMessage_ProcessEvent, cpi->progress, 1);
      }
      break;

   case CipSignal::ANM:
      return pssm.RaiseLocalAnswer(nextEvent);

   case CipSignal::REL:
      cci = msg->FindType< CauseInfo >(CipParameter::Cause);
      return pssm.RaiseLocalRelease(nextEvent, cci->cause);
   }

   Debug::SwErr(PotsProxyTaAnalyzeLocalMessage_ProcessEvent, sid, 0);
   return pssm.RaiseReleaseCall(nextEvent, Cause::MessageInvalidForState);
}

//==============================================================================

fn_name PotsProxyAcAnalyzeLocalMessage_ProcessEvent =
   "PotsProxyAcAnalyzeLocalMessage.ProcessEvent";

EventHandler::Rc PotsProxyAcAnalyzeLocalMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyAcAnalyzeLocalMessage_ProcessEvent);

   auto&         ame = static_cast< AnalyzeMsgEvent& >(currEvent);
   auto          msg = static_cast< CipMessage* >(ame.Msg());
   auto          sid = msg->GetSignal();
   auto&         pssm = static_cast< PotsBcSsm& >(ssm);
   ProgressInfo* cpi;
   CauseInfo*    cci;

   switch(sid)
   {
   case CipSignal::CPG:
      cpi = msg->FindType< ProgressInfo >(CipParameter::Progress);
      switch(cpi->progress)
      {
      case Progress::Suspend:
         if(pssm.ProxyCount() > 1) return Suspend;
         return pssm.RaiseLocalSuspend(nextEvent);
      case Progress::Resume:
         if(pssm.ProxyCount() > 1) return Suspend;
         if(pssm.CurrState() == BcState::Active) return Suspend;
         return pssm.RaiseLocalResume(nextEvent);
      case Progress::EndOfSelection:
      case Progress::Alerting:
         if(pssm.ProxyCount() > 1) return Suspend;
      default:
         Debug::SwErr
            (PotsProxyAcAnalyzeLocalMessage_ProcessEvent, cpi->progress, 1);
      }
      break;

   case CipSignal::REL:
      cci = msg->FindType< CauseInfo >(CipParameter::Cause);
      return pssm.RaiseLocalRelease(nextEvent, cci->cause);

   case CipSignal::ANM:
      if(pssm.ProxyCount() > 1) return pssm.RaiseLocalAnswer(nextEvent);

   default:
      Debug::SwErr(PotsProxyAcAnalyzeLocalMessage_ProcessEvent, sid, 0);
   }

   return pssm.RaiseReleaseCall(nextEvent, Cause::MessageInvalidForState);
}

//------------------------------------------------------------------------------

fn_name PotsProxyAcLocalSuspend_ProcessEvent =
   "PotsProxyAcLocalSuspend.ProcessEvent";

EventHandler::Rc PotsProxyAcLocalSuspend::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyAcLocalSuspend_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto npsm = pssm.NPsm();

   pssm.Relay(*npsm);
   pssm.SetNextState(BcState::LocalSuspending);
   return Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsProxyAcRemoteSuspend_ProcessEvent =
   "PotsProxyAcRemoteSuspend.ProcessEvent";

EventHandler::Rc PotsProxyAcRemoteSuspend::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyAcRemoteSuspend_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto upsm = pssm.FirstProxy();

   pssm.Relay(*upsm);
   pssm.SetNextState(BcState::RemoteSuspending);
   return Suspend;
}

//==============================================================================

fn_name PotsProxyLsLocalResume_ProcessEvent =
   "PotsProxyLsLocalResume.ProcessEvent";

EventHandler::Rc PotsProxyLsLocalResume::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyLsLocalResume_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto npsm = pssm.NPsm();

   pssm.Relay(*npsm);
   pssm.SetNextState(BcState::Active);
   return Suspend;
}

//==============================================================================

fn_name PotsProxyRsRemoteResume_ProcessEvent =
   "PotsProxyRsRemoteResume.ProcessEvent";

EventHandler::Rc PotsProxyRsRemoteResume::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyRsRemoteResume_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto upsm = pssm.FirstProxy();

   pssm.Relay(*upsm);
   pssm.SetNextState(BcState::Active);
   return Suspend;
}

//==============================================================================

fn_name PotsProxyLocalAlerting_ProcessEvent =
   "PotsProxyLocalAlerting.ProcessEvent";

EventHandler::Rc PotsProxyLocalAlerting::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyLocalAlerting_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto npsm = pssm.NPsm();

   //  If the NPSM has a peer media PSM, connect media from that PSM,
   //  because the far end should be applying ringback.  If the NPSM
   //  has no peer media PSM, apply ringback on the NPSM.
   //
   auto peer = npsm->GetOgPsm();

   if(peer != nullptr)
      peer->SetIcTone(Tone::Media);
   else
      npsm->SetOgTone(Tone::Ringback);

   if(pssm.CurrState() == BcState::PresentingCall)
   {
      pssm.Relay(*npsm);
      pssm.SetNextSnp(BcTrigger::LocalAlertingSnp);
      pssm.SetNextState(BcState::TermAlerting);
   }

   return Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsProxyLocalAnswer_ProcessEvent = "PotsProxyLocalAnswer.ProcessEvent";

EventHandler::Rc PotsProxyLocalAnswer::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyLocalAnswer_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto upsm = static_cast< MediaPsm* >(Context::ContextPsm());
   auto npsm = pssm.NPsm();

   //  Ensure a media flow between the UPSM and NPSM.
   //
   upsm->EnsureMedia(*npsm);

   //  If this is the first ANM, relay it to the NPSM.
   //
   if(pssm.CurrState() != BcState::Active)
   {
      pssm.Relay(*npsm);
      PotsStatistics::Incr(PotsStatistics::ProxyAnswered);
      pssm.SetNextSnp(BcTrigger::LocalAnswerSnp);
      pssm.SetNextState(BcState::Active);
   }

   return Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsProxyRemoteAnswer_ProcessEvent =
   "PotsProxyRemoteAnswer.ProcessEvent";

EventHandler::Rc PotsProxyRemoteAnswer::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyRemoteAnswer_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto upsm = pssm.FirstProxy();

   pssm.Relay(*upsm);
   pssm.SetNextSnp(BcTrigger::RemoteAnswerSnp);
   pssm.SetNextState(BcState::Active);
   return Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsProxyLocalRelease_ProcessEvent =
   "PotsProxyLocalRelease.ProcessEvent";

EventHandler::Rc PotsProxyLocalRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyLocalRelease_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto upsm = static_cast< MediaPsm* >(Context::ContextPsm());

   //  Disable media on the UPSM.
   //
   upsm->DisableMedia();

   //  If this is the last UPSM, relay the REL to the NPSM.
   //
   if(pssm.ProxyCount() == 1)
   {
      auto npsm = pssm.NPsm();

      pssm.Relay(*npsm);
      pssm.SetNextSnp(BcTrigger::CallClearedSnp);
      pssm.SetNextState(BcState::Null);
   }

   return Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsProxyRemoteRelease_ProcessEvent =
   "PotsProxyRemoteRelease.ProcessEvent";

EventHandler::Rc PotsProxyRemoteRelease::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyRemoteRelease_ProcessEvent);

   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto npsm = pssm.NPsm();
   auto upsm = pssm.FirstProxy();

   npsm->DisableMedia();
   pssm.Relay(*upsm);
   pssm.SetNextSnp(BcTrigger::CallClearedSnp);
   pssm.SetNextState(BcState::Null);
   return Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsProxyReleaseCall_ProcessEvent = "PotsProxyReleaseCall.ProcessEvent";

EventHandler::Rc PotsProxyReleaseCall::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(PotsProxyReleaseCall_ProcessEvent);

   auto& cte = static_cast< BcReleaseCallEvent& >(currEvent);
   auto& pssm = static_cast< PotsBcSsm& >(ssm);
   auto npsm = pssm.NPsm();

   CauseInfo cci;
   cci.cause = cte.GetCause();

   if((npsm != nullptr) && (npsm->GetState() != ProtocolSM::Idle))
   {
      auto msg = new CipMessage(npsm, 16);
      npsm->DisableMedia();
      msg->SetSignal(CipSignal::REL);
      msg->AddCause(cci);
   }

   auto upsm = pssm.FirstProxy();

   while((upsm != nullptr) && (upsm->GetState() == ProtocolSM::Idle))
   {
      pssm.NextProxy(upsm);
   }

   if(upsm != nullptr)
   {
      auto msg = new CipMessage(upsm, 16);
      upsm->DisableMedia();
      msg->SetSignal(CipSignal::REL);
      msg->AddCause(cci);
   }

   pssm.SetNextSnp(BcTrigger::CallClearedSnp);
   pssm.SetNextState(BcState::Null);
   return Suspend;
}
}