//==============================================================================
//
//  SbHandlers.cpp
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
#include "SbHandlers.h"
#include "Algorithms.h"
#include "Context.h"
#include "Debug.h"
#include "SbEvents.h"
#include "Service.h"
#include "ServiceRegistry.h"
#include "ServiceSM.h"
#include "Singleton.h"
#include "State.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name SbAnalyzeMessage_ProcessEvent = "SbAnalyzeMessage.ProcessEvent";

EventHandler::Rc SbAnalyzeMessage::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(SbAnalyzeMessage_ProcessEvent);

   //  Find the port associated with the incoming message.  The port
   //  and the SSM's state determine which message analyzer to invoke.
   //
   auto pid = ssm.CalcPort(static_cast< const AnalyzeMsgEvent& >(currEvent));

   if(pid == NIL_ID) return Pass;

   auto service = ssm.GetService();
   auto stid = ssm.CurrState();
   auto state = service->GetState(stid);
   auto ehid = state->MsgAnalyzer(pid);
   auto handler = service->GetHandler(ehid);

   if(handler == nullptr)
   {
      //  There is no message analyzer.  This is acceptable for a
      //  modifier, but not for a root service.
      //
      if(ssm.Parent() == nullptr)
      {
         Context::Kill(SbAnalyzeMessage_ProcessEvent,
            pack3(ssm.Sid(), state->Stid(), pid), 0);
      }

      return Pass;
   }

   auto rc = handler->ProcessEvent(ssm, currEvent, nextEvent);

   //  A message analyzer is not allowed to change its service's state.
   //
   if(ssm.CurrState() != stid)
   {
      Debug::SwLog(SbAnalyzeMessage_ProcessEvent,
         pack3(ssm.Sid(), state->Stid(), ssm.CurrState()), 1);
   }

   return rc;
}

//==============================================================================

fn_name SbAnalyzeSap_ProcessEvent = "SbAnalyzeSap.ProcessEvent";

EventHandler::Rc SbAnalyzeSap::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(SbAnalyzeSap_ProcessEvent);

   //  Invoke the modifier's SAP handler.
   //
   return ssm.ProcessSap(currEvent, nextEvent);
}

//==============================================================================

fn_name SbAnalyzeSnp_ProcessEvent = "SbAnalyzeSnp.ProcessEvent";

EventHandler::Rc SbAnalyzeSnp::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(SbAnalyzeSnp_ProcessEvent);

   //  Invoke the modifier's SNP handler.
   //
   return ssm.ProcessSnp(currEvent, nextEvent);
}

//==============================================================================

fn_name SbForceTransition_ProcessEvent = "SbForceTransition.ProcessEvent";

EventHandler::Rc SbForceTransition::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(SbForceTransition_ProcessEvent);

   //  Invoke the event handler specified by the event.
   //
   auto& fte = static_cast< ForceTransitionEvent& >(currEvent);

   return fte.Handler()->ProcessEvent(ssm, currEvent, nextEvent);
}

//==============================================================================

fn_name SbInitiationReq_ProcessEvent = "SbInitiationReq.ProcessEvent";

EventHandler::Rc SbInitiationReq::ProcessEvent
   (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft(SbInitiationReq_ProcessEvent);

   auto& ire = static_cast< InitiationReqEvent& >(currEvent);

   //  Determine how to treat this initiation event:
   //  (a) As an event that this modifier may screen.
   //  (b) As an event that this modifier requested and that has now reached
   //      that modifier after other modifiers in the SSMQ have screened it.
   //  (c) As an event that has been screened and that should now be passed
   //      to the target modifier by its parent.
   //
   if(ire.Owner() != &ssm)
   {
      if(ire.IsBeingScreened())
      {
         //  Case (a).  Invoke the modifier's SIP handler.
         //
         return ssm.ProcessSip(currEvent, nextEvent);
      }
      else
      {
         //  Case (b).  Invoke the target modifier's initiation
         //  ack or nack handler.
         //
         if(!ire.WasDenied())
            return ssm.ProcessInitAck(currEvent, nextEvent);
         else
            return ssm.ProcessInitNack(currEvent, nextEvent);
      }
   }

   //  Case (c).  Before invoking ProcessEvent on the modifier, create
   //  its SSM if necessary.  When the modifier receives the event, it
   //  is processed under case (b).
   //
   ire.SetScreening(false);

   auto modifier = ire.GetReceiver();

   if(modifier == nullptr)
   {
      auto reg = Singleton< ServiceRegistry >::Instance();
      auto svc = reg->GetService(ire.GetModifier());

      if(svc == nullptr)
      {
         Debug::SwLog(SbInitiationReq_ProcessEvent, ire.GetModifier(), 0);
         return Suspend;
      }

      if(svc->GetStatus() != Service::Enabled) return Suspend;

      modifier = svc->AllocModifier();

      if(modifier == nullptr)
      {
         Context::Kill(SbInitiationReq_ProcessEvent, ire.GetModifier(), 0);
      }

      ssm.HenqModifier(*modifier);
      ire.SetReceiver(modifier);
   }

   auto rc = modifier->ProcessEvent(&currEvent, nextEvent);

   switch(rc)
   {
   case Suspend:
      break;

   case Revert:
      //
      //  We are about to return to the parent's ProcessEvent function, so
      //  map this to Continue to handle the event that the parent owns.
      //
      rc = Continue;
      break;

   default:
      //  Other results are unlikely.  We are here because an SSM raised
      //  an initiation event after handling some other event.
      //
      Debug::SwLog(SbInitiationReq_ProcessEvent, ire.GetModifier(), rc);
      delete nextEvent;
      nextEvent = nullptr;
      rc = Suspend;
   }

   modifier->DeleteIdleModifier();
   return rc;
}
}
