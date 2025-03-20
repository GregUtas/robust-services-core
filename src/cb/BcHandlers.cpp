//==============================================================================
//
//  BcHandlers.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "BcSessions.h"
#include "BcProtocol.h"
#include "Context.h"
#include "Debug.h"
#include "NbAppIds.h"
#include "SbEvents.h"

//------------------------------------------------------------------------------

namespace CallBase
{
fn_name BcNuAnalyzeRemoteMessage_ProcessEvent =
   "BcNuAnalyzeRemoteMessage.ProcessEvent";

EventHandler::Rc BcNuAnalyzeRemoteMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(BcNuAnalyzeRemoteMessage_ProcessEvent);

   auto& ame = static_cast<AnalyzeMsgEvent&>(currEvent);
   auto sid = ame.Msg()->GetSignal();
   auto& bcssm = static_cast<BcSsm&>(ssm);

   if(sid == CipSignal::IAM)
   {
      nextEvent = new BcTerminateEvent(ssm);
      return Continue;
   }

   Debug::SwLog(BcNuAnalyzeRemoteMessage_ProcessEvent, "invalid signal", sid);
   return bcssm.RaiseReleaseCall(nextEvent, Cause::MessageInvalidForState);
}

//------------------------------------------------------------------------------

fn_name BcScAnalyzeRemoteMessage_ProcessEvent =
   "BcScAnalyzeRemoteMessage.ProcessEvent";

EventHandler::Rc BcScAnalyzeRemoteMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(BcScAnalyzeRemoteMessage_ProcessEvent);

   auto&         ame = static_cast<AnalyzeMsgEvent&>(currEvent);
   auto          msg = static_cast<CipMessage*>(ame.Msg());
   auto          sid = msg->GetSignal();
   auto&         bcssm = static_cast<BcSsm&>(ssm);
   ProgressInfo* cpi;
   CauseInfo*    cci;

   switch(sid)
   {
   case CipSignal::CPG:
      cpi = msg->FindType<ProgressInfo>(CipParameter::Progress);
      switch(cpi->progress)
      {
      case Progress::EndOfSelection:
         return bcssm.RaiseRemoteProgress(nextEvent, cpi->progress);
      case Progress::Alerting:
         return bcssm.RaiseRemoteAlerting(nextEvent);
      default:
         Debug::SwLog(BcScAnalyzeRemoteMessage_ProcessEvent,
            "invalid Progress::Ind", cpi->progress);
      }
      break;

   case CipSignal::ANM:
      return bcssm.RaiseRemoteAnswer(nextEvent);

   case CipSignal::REL:
      cci = msg->FindType<CauseInfo>(CipParameter::Cause);
      if(cci->cause == Cause::UserBusy)
      {
         return bcssm.RaiseRemoteBusy(nextEvent);
      }
      return bcssm.RaiseRemoteRelease(nextEvent, cci->cause);

   case Signal::Timeout:
      return bcssm.AnalyzeNPsmTimeout(*msg, nextEvent);

   default:
      Debug::SwLog(BcScAnalyzeRemoteMessage_ProcessEvent,
         "invalid signal", sid);
   }

   return bcssm.RaiseReleaseCall(nextEvent, Cause::MessageInvalidForState);
}

//------------------------------------------------------------------------------

fn_name BcOaAnalyzeRemoteMessage_ProcessEvent =
   "BcOaAnalyzeRemoteMessage.ProcessEvent";

EventHandler::Rc BcOaAnalyzeRemoteMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(BcOaAnalyzeRemoteMessage_ProcessEvent);

   auto&      ame = static_cast<AnalyzeMsgEvent&>(currEvent);
   auto       msg = static_cast<CipMessage*>(ame.Msg());
   auto       sid = msg->GetSignal();
   auto&      bcssm = static_cast<BcSsm&>(ssm);
   CauseInfo* cci;

   switch(sid)
   {
   case CipSignal::ANM:
      if(Debug::SwFlagOn(CallTrapFlag))
      {
         Context::Kill("trap recovery test", 0);
      }
      return bcssm.RaiseRemoteAnswer(nextEvent);

   case CipSignal::REL:
      cci = msg->FindType<CauseInfo>(CipParameter::Cause);
      if(cci->cause == Cause::AnswerTimeout)
      {
         return bcssm.RaiseRemoteNoAnswer(nextEvent);
      }
      return bcssm.RaiseRemoteRelease(nextEvent, cci->cause);

   case Signal::Timeout:
      return bcssm.AnalyzeNPsmTimeout(*msg, nextEvent);
   }

   Debug::SwLog(BcOaAnalyzeRemoteMessage_ProcessEvent, "invalid signal", sid);
   return bcssm.RaiseReleaseCall(nextEvent, Cause::MessageInvalidForState);
}

//------------------------------------------------------------------------------

fn_name BcPcAnalyzeRemoteMessage_ProcessEvent =
   "BcPcAnalyzeRemoteMessage.ProcessEvent";

EventHandler::Rc BcPcAnalyzeRemoteMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(BcPcAnalyzeRemoteMessage_ProcessEvent);

   auto&      ame = static_cast<AnalyzeMsgEvent&>(currEvent);
   auto       msg = static_cast<CipMessage*>(ame.Msg());
   auto       sid = msg->GetSignal();
   auto&      bcssm = static_cast<BcSsm&>(ssm);
   CauseInfo* cci;

   switch(sid)
   {
   case CipSignal::REL:
      cci = msg->FindType<CauseInfo>(CipParameter::Cause);
      return bcssm.RaiseRemoteRelease(nextEvent, cci->cause);

   case Signal::Timeout:
      return bcssm.AnalyzeNPsmTimeout(*msg, nextEvent);
   }

   Debug::SwLog(BcPcAnalyzeRemoteMessage_ProcessEvent, "invalid signal", sid);
   return bcssm.RaiseReleaseCall(nextEvent, Cause::MessageInvalidForState);
}

//------------------------------------------------------------------------------

fn_name BcAcAnalyzeRemoteMessage_ProcessEvent =
   "BcAcAnalyzeRemoteMessage.ProcessEvent";

EventHandler::Rc BcAcAnalyzeRemoteMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(BcAcAnalyzeRemoteMessage_ProcessEvent);

   auto&         ame = static_cast<AnalyzeMsgEvent&>(currEvent);
   auto          msg = static_cast<CipMessage*>(ame.Msg());
   auto          sid = msg->GetSignal();
   auto&         bcssm = static_cast<BcSsm&>(ssm);
   ProgressInfo* cpi;
   CauseInfo*    cci;

   switch(sid)
   {
   case CipSignal::CPG:
      cpi = msg->FindType<ProgressInfo>(CipParameter::Progress);
      switch(cpi->progress)
      {
      case Progress::Suspend:
         return bcssm.RaiseRemoteSuspend(nextEvent);
      case Progress::Resume:
         return bcssm.RaiseRemoteResume(nextEvent);
      default:
         Debug::SwLog(BcAcAnalyzeRemoteMessage_ProcessEvent,
            "invalid Progress::Ind", cpi->progress);
      }
      break;

   case CipSignal::REL:
      cci = msg->FindType<CauseInfo>(CipParameter::Cause);
      return bcssm.RaiseRemoteRelease(nextEvent, cci->cause);

   case Signal::Timeout:
      return bcssm.AnalyzeNPsmTimeout(*msg, nextEvent);

   default:
      Debug::SwLog(BcAcAnalyzeRemoteMessage_ProcessEvent,
         "invalid signal", sid);
   }

   return bcssm.RaiseReleaseCall(nextEvent, Cause::MessageInvalidForState);
}
}
