//==============================================================================
//
//  ServiceSM.cpp
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
#include "ServiceSM.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Clock.h"
#include "Context.h"
#include "Debug.h"
#include "Formatters.h"
#include "Initiator.h"
#include "SbEvents.h"
#include "SbPools.h"
#include "SbTrace.h"
#include "SbTracer.h"
#include "Service.h"
#include "ServiceRegistry.h"
#include "Singleton.h"
#include "State.h"
#include "SysTypes.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name ServiceSM_ctor = "ServiceSM.ctor";

ServiceSM::ServiceSM(ServiceId sid) :
   sid_(sid),
   currState_(Null),
   nextState_(Null),
   idled_(false),
   nextSap_(NIL_ID),
   nextSnp_(NIL_ID),
   parentSsm_(nullptr)
{
   Debug::ft(ServiceSM_ctor);

   for(auto i = 0; i <= Trigger::MaxId; ++i) triggered_[i] = false;

   ssmq_.Init(Pooled::LinkDiff());

   for(auto i = 0; i < Event::Location_N; ++i)
   {
      eventq_[i].Init(Pooled::LinkDiff());
   }

   //  See if this service should trigger tracing of this context.
   //
   auto ctx = Context::RunningContext();
   if(ctx == nullptr) return;

   if(!ctx->TraceOn())
   {
      ctx->SetTrace(Singleton< SbTracer >::Instance()->ServiceIsTraced(sid_));
   }

   //  Record the SSM's creation if this context is traced.
   //
   TransTrace* trans = nullptr;

   if(ctx->TraceOn(trans))
   {
      auto warp = Clock::TicksNow();
      auto buff = Singleton< TraceBuffer >::Instance();

      if(buff->ToolIsOn(ContextTracer))
      {
         auto rec = new SsmTrace(SsmTrace::Creation, *this);
         buff->Insert(rec);
      }

      if(trans != nullptr) trans->ResumeTime(warp);
   }
}

//------------------------------------------------------------------------------

fn_name ServiceSM_dtor = "ServiceSM.dtor";

ServiceSM::~ServiceSM()
{
   Debug::ft(ServiceSM_dtor);

   //  Record the SSM's deletion if this context is traced.
   //
   TransTrace* trans = nullptr;

   if(Context::RunningContextTraced(trans))
   {
      auto warp = Clock::TicksNow();
      auto buff = Singleton< TraceBuffer >::Instance();

      if(Singleton< TraceBuffer >::Instance()->ToolIsOn(ContextTracer))
      {
         auto rec = new SsmTrace(SsmTrace::Deletion, *this);
         buff->Insert(rec);
      }

      if(trans != nullptr) trans->ResumeTime(warp);
   }

   //  Delete all events and any modifiers in the SSMQ.
   //
   for(auto i = 0; i < Event::Location_N; ++i)
   {
      eventq_[i].Purge();
   }

   ssmq_.Purge();

   //  If this SSM is a modifier, exqueue it.
   //
   if(parentSsm_ != nullptr)
   {
      if(!parentSsm_->ssmq_.Exq(*this))
      {
         Debug::SwLog(ServiceSM_dtor, parentSsm_->Sid(), sid_);
      }
   }
}

//------------------------------------------------------------------------------

fn_name ServiceSM_CalcPort = "ServiceSM.CalcPort";

ServicePortId ServiceSM::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft(ServiceSM_CalcPort);

   Context::Kill(strOver(this), sid_);
   return NIL_ID;
}

//------------------------------------------------------------------------------

fn_name ServiceSM_DeleteIdleModifier = "ServiceSM.DeleteIdleModifier";

void ServiceSM::DeleteIdleModifier()
{
   Debug::ft(ServiceSM_DeleteIdleModifier);

   //  If this is a modifier in the Null state, delete it.
   //
   if((parentSsm_ != nullptr) && (currState_ == Null))
   {
      delete this;
   }
}

//------------------------------------------------------------------------------

void ServiceSM::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Pooled::Display(stream, prefix, options);

   stream << prefix << "sid       : " << sid_ << CRLF;
   stream << prefix << "currState : " << currState_ << CRLF;
   stream << prefix << "nextState : " << nextState_ << CRLF;
   stream << prefix << "idled     : " << idled_ << CRLF;
   stream << prefix << "nextSap   : " << nextSap_ << CRLF;
   stream << prefix << "nextSnp   : " << nextSnp_ << CRLF;

   stream << prefix << "triggered : " << CRLF;

   auto lead = prefix + spaces(2);
   stream << lead;

   auto found = false;

   for(auto i = 0; i <= Trigger::MaxId; ++i)
   {
      if(triggered_[i])
      {
         stream << i << SPACE;
         found = true;
      }
   }

   if(!found) stream << "none";
   stream << CRLF;

   stream << prefix << "ssmq : " << CRLF;
   ssmq_.Display(stream, lead, options);
   stream << prefix << "parentSsm : " << parentSsm_ << CRLF;
   stream << prefix << "eventq[Active] : " << CRLF;
   eventq_[Event::Active].Display(stream, lead, options);
   stream << prefix << "eventq[Pending] : " << CRLF;
   eventq_[Event::Pending].Display(stream, lead, options);
   stream << prefix << "eventq[Saved] : " << CRLF;
   eventq_[Event::Saved].Display(stream, lead, options);
}

//------------------------------------------------------------------------------

fn_name ServiceSM_EndOfTransaction = "ServiceSM.EndOfTransaction";

void ServiceSM::EndOfTransaction()
{
   Debug::ft(ServiceSM_EndOfTransaction);

   //  The following allows SetNextState to be used after ProcessEvent,
   //  usually to enter the Null state after all PSMs have been deleted.
   //
   currState_ = nextState_;

   //  Invoke EndOfTransaction on each modifier.  Delete a modifier that
   //  ends up in the Null state.
   //
   for(auto mod = ssmq_.First(); mod != nullptr; ssmq_.Next(mod))
   {
      mod->EndOfTransaction();
      if(mod->CurrState() == Null) delete mod;
   }
}

//------------------------------------------------------------------------------

fn_name ServiceSM_EnqEvent = "ServiceSM.EnqEvent";

void ServiceSM::EnqEvent(Event& evt, Event::Location loc)
{
   Debug::ft(ServiceSM_EnqEvent);

   if(loc >= Event::Location_N)
   {
      Debug::SwLog(ServiceSM_EnqEvent,
         "invalid location", pack3(sid_, evt.Eid(), loc));
      return;
   }

   if(!eventq_[loc].Enq(evt))
   {
      Debug::SwLog(ServiceSM_EnqEvent,
         "Enq failed", pack3(sid_, evt.Eid(), loc));
   }
}

//------------------------------------------------------------------------------

fn_name ServiceSM_EventError1 = "ServiceSM.EventError1";

void ServiceSM::EventError1(Event*& evt) const
{
   Debug::ft(ServiceSM_EventError1);

   Debug::SwLog(ServiceSM_EventError1, sid_, evt->Eid());
   delete evt;
   evt = nullptr;
}

//------------------------------------------------------------------------------

fn_name ServiceSM_EventError2 = "ServiceSM.EventError2";

EventHandler::Rc ServiceSM::EventError2(Event*& evt, EventHandler::Rc rc) const
{
   Debug::ft(ServiceSM_EventError2);

   Debug::SwLog(ServiceSM_EventError2, sid_, evt->Eid());
   delete evt;
   evt = nullptr;
   return rc;
}

//------------------------------------------------------------------------------

fn_name ServiceSM_ExqEvent = "ServiceSM.ExqEvent";

bool ServiceSM::ExqEvent(Event& evt, Event::Location loc)
{
   Debug::ft(ServiceSM_ExqEvent);

   if(loc >= Event::Location_N)
   {
      Debug::SwLog(ServiceSM_ExqEvent,
         "invalid location", pack3(sid_, evt.Eid(), loc));
      return false;
   }

   if(!eventq_[loc].Exq(evt))
   {
      Debug::SwLog(ServiceSM_EnqEvent,
         "Exq failed", pack3(sid_, evt.Eid(), loc));
      return false;
   }

   return true;
}

//------------------------------------------------------------------------------

Service* ServiceSM::GetService() const
{
   return Singleton< ServiceRegistry >::Instance()->GetService(sid_);
}

//------------------------------------------------------------------------------

fn_name ServiceSM_GetSubtended = "ServiceSM.GetSubtended";

void ServiceSM::GetSubtended(Base* objects[], size_t& count) const
{
   Debug::ft(ServiceSM_GetSubtended);

   Pooled::GetSubtended(objects, count);

   for(auto i = 0; i < Event::Location_N; ++i)
   {
      for(auto evt = eventq_[i].First(); evt != nullptr; eventq_[i].Next(evt))
      {
         evt->GetSubtended(objects, count);
      }
   }

   for(auto mod = ssmq_.First(); mod != nullptr; ssmq_.Next(mod))
   {
      mod->GetSubtended(objects, count);
   }
}

//------------------------------------------------------------------------------

fn_name ServiceSM_HasTriggered = "ServiceSM.HasTriggered";

bool ServiceSM::HasTriggered(TriggerId tid) const
{
   Debug::ft(ServiceSM_HasTriggered);

   if(!Trigger::IsValidId(tid)) return false;

   return triggered_[tid];
}

//------------------------------------------------------------------------------

fn_name ServiceSM_HenqModifier = "ServiceSM.HenqModifier";

void ServiceSM::HenqModifier(ServiceSM& modifier)
{
   Debug::ft(ServiceSM_HenqModifier);

   ssmq_.Henq(modifier);
   modifier.SetParent(*this);
}

//------------------------------------------------------------------------------

fn_name ServiceSM_MorphToService = "ServiceSM.MorphToService";

void ServiceSM::MorphToService(ServiceId sid)
{
   Debug::ft(ServiceSM_MorphToService);

   //e Support true morphing (Object::MorphTo).

   sid_ = sid;
}

//------------------------------------------------------------------------------

fn_name ServiceSM_new = "ServiceSM.operator new";

void* ServiceSM::operator new(size_t size)
{
   Debug::ft(ServiceSM_new);

   return Singleton< ServiceSMPool >::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

void ServiceSM::Patch(sel_t selector, void* arguments)
{
   Pooled::Patch(selector,arguments);
}

//------------------------------------------------------------------------------

fn_name ServiceSM_ProcessEvent = "ServiceSM.ProcessEvent";

EventHandler::Rc ServiceSM::ProcessEvent(Event* currEvent, Event*& nextEvent)
{
   Debug::ft(ServiceSM_ProcessEvent);

   auto phase = ModifierSapPhase;
   auto rc = EventHandler::Suspend;

   Event* sapEvent = nullptr;
   Event* snpEvent = nullptr;
   TriggerId tid;
   Trigger* trigger;
   const Initiator* modifierInit;
   ServiceSM* modifierSsm;

   //  Return immediately if the SSM has entered the Null state.
   //
   if(idled_) return EventHandler::Pass;

   //  Event routing begins by determining which event to process next.
   //
   while(true)
   {
      switch(phase)
      {
      case ModifierSapPhase:
         //
         //  By default, the state will not change.
         //
         nextState_ = currState_;

         //  If there are modifiers on the SSMQ, create an SAP event and
         //  pass it down the SSMQ.  Next, pass it down the InitQ unless
         //  some modifier decided otherwise.
         //
         phase = InitiatorSapPhase;
         if(ssmq_.Empty()) break;
         sapEvent = currEvent->BuildSap(*this, nextSap_);
         if(sapEvent == nullptr) break;
         tid = nextSap_;
         nextSap_ = NIL_ID;
         modifierSsm = ssmq_.First();
         rc = ProcessSsmqSap(modifierSsm, *sapEvent, nextEvent, phase);
         if(phase == InitiatorSapPhase) nextSap_ = tid;
         break;

      case ModifierReentryPhase:
         //
         //  A modifier ended a transaction during SSMQ SAP processing after
         //  saving the context.  It has now restored that context in order
         //  to resume traversal of the SSMQ, starting at the next modifier.
         //  currEvent is the SAP whose processing is to resume; it contains
         //  the information needed to restore the context.
         //
         phase = InitiatorSapPhase;
         sapEvent = currEvent;
         if(sapEvent->Owner() == this)
            tid = static_cast< AnalyzeSapEvent* >(sapEvent)->GetTrigger();
         else
            tid = NIL_ID;
         nextSap_ = NIL_ID;
         modifierSsm = static_cast< AnalyzeSapEvent* >(sapEvent)->CurrSsm();
         currEvent = static_cast< AnalyzeSapEvent* >(sapEvent)->CurrEvent();
         nextState_ = currState_;
         ssmq_.Next(modifierSsm);
         if(modifierSsm != nullptr)
            rc = ProcessSsmqSap(modifierSsm, *sapEvent, nextEvent, phase);
         if(phase == InitiatorSapPhase) nextSap_ = tid;
         break;

      case InitiatorSapPhase:
         //
         //  If the SSM has defined this to be an SAP at which modifiers can
         //  be triggered, pass an SAP event down the InitQ if it contains
         //  any modifiers.  The SSM's event handler will be invoked next
         //  unless some initiator decides otherwise.
         //
         phase = LocalEventPhase;
         tid = nextSap_;
         if(tid == NIL_ID) break;
         nextSap_ = NIL_ID;
         trigger = GetService()->GetTrigger(tid);

         if(trigger == nullptr)
         {
            triggered_[tid] = true; break;
         }

         modifierInit = trigger->initq_.First();

         if(modifierInit == nullptr)
         {
            triggered_[tid] = true; break;
         }

         if(sapEvent == nullptr) sapEvent = currEvent->BuildSap(*this, tid);
         if(sapEvent == nullptr) break;
         rc = ProcessInitqSap(trigger,
            modifierInit, *sapEvent, nextEvent, phase);
         break;

      case InitiatorReentryPhase:
         //
         //  A modifier was just initiated, and it ended the transaction
         //  during SSMQ SAP processing after saving the context.  It has
         //  now restored that context in order to resume traversal of the
         //  InitQ, starting at the next modifier.  currEvent is the SAP
         //  whose processing is to resume; it contains the information
         //  needed to restore the context.
         //
         phase = LocalEventPhase;
         sapEvent = currEvent;
         if(sapEvent->Owner() == this)
            tid = static_cast< AnalyzeSapEvent* >(sapEvent)->GetTrigger();
         else
            tid = NIL_ID;
         if(tid == NIL_ID) break;
         nextSap_ = NIL_ID;
         trigger = GetService()->GetTrigger(tid);
         currEvent = static_cast< AnalyzeSapEvent* >(sapEvent)->CurrEvent();
         modifierInit = static_cast< AnalyzeSapEvent* >
            (sapEvent)->CurrInitiator();
         modifierInit = trigger->initq_.Next(*modifierInit);

         if(modifierInit == nullptr)
         {
            triggered_[tid] = true; break;
         }

         rc = ProcessInitqSap(trigger,
            modifierInit, *sapEvent, nextEvent, phase);
         break;

      case LocalEventPhase:
         //
         //  Invoke our event handler.  After that, pass an SNP down the
         //  SSMQ and the InitQ.
         //
         phase = FreeEventPhase;
         {
            auto svc = GetService();
            auto state = svc->GetState(currState_);
            auto ehid = state->GetHandler(currEvent->Eid());
            auto handler = svc->GetHandler(ehid);

            if(handler == nullptr)
            {
               Context::Kill("event handler not found",
                  pack3(sid_, state->Stid(), currEvent->Eid()));
               return EventHandler::Suspend;
            }

            rc = handler->ProcessEvent(*this, *currEvent, nextEvent);

            //  Record the event handler's invocation if this context
            //  is traced.
            //
            TransTrace* trans = nullptr;

            if(Context::RunningContextTraced(trans))
            {
               auto warp = Clock::TicksNow();
               auto buff = Singleton< TraceBuffer >::Instance();

               if(buff->ToolIsOn(ContextTracer))
               {
                  currEvent->Capture(sid_, *state, rc);
               }

               if(trans != nullptr) trans->ResumeTime(warp);
            }
         }

         switch(rc)
         {
         case EventHandler::Suspend:
         case EventHandler::Pass:
         case EventHandler::Resume:
            //
            //  There should be no next event and no next SAP.
            //
            if(nextEvent != nullptr) rc = EventError2(nextEvent, rc);

            if(nextSap_ != NIL_ID)
            {
               Debug::SwLog(ServiceSM_ProcessEvent, pack2(nextSap_, sid_), rc);
               nextSap_ = NIL_ID;
            }
            break;

         case EventHandler::Continue:
            //
            //  There should be a next event that this SSM owns.
            //
            if(nextEvent != nullptr)
            {
               if(nextEvent->Owner() != this)
                  rc = EventError2(nextEvent, EventHandler::Suspend);
            }
            else
            {
               Debug::SwLog(ServiceSM_ProcessEvent, sid_, rc);
               rc = EventHandler::Suspend;
            }
            break;

         case EventHandler::Revert:
            //
            //  There should be a next event that one of this SSM's
            //  ancestors owns.
            //
            if(nextEvent != nullptr)
            {
               ServiceSM* ancestor;

               for(ancestor = parentSsm_; ancestor != nullptr;
                  ancestor = ancestor->Parent())
               {
                  if(nextEvent->Owner() == ancestor) break;
               }

               if(ancestor == nullptr)
                  rc = EventError2(nextEvent, EventHandler::Suspend);
            }
            else
            {
               Debug::SwLog(ServiceSM_ProcessEvent, sid_, rc);
               rc = EventHandler::Suspend;
            }
            break;

         case EventHandler::Initiate:
            //
            //  There should be an initiation request that this SSM owns
            //  (in which case it is requesting the creation of one of its
            //  own modifiers) or that this SSM's parent owns (in which
            //  case this SSM is requesting the creation of one of its
            //  siblings).
            //
            if(nextEvent != nullptr)
            {
               if(nextEvent->Eid() == Event::InitiationReq)
               {
                  if(nextEvent->Owner() == this)
                     rc = EventHandler::Continue;
                  else if(nextEvent->Owner() == parentSsm_)
                     rc = EventHandler::Revert;
                  else
                     rc = EventError2(nextEvent, EventHandler::Suspend);
               }
               else
               {
                  rc = EventError2(nextEvent, EventHandler::Suspend);
               }
            }
            else
            {
               Debug::SwLog(ServiceSM_ProcessEvent, sid_, rc);
               rc = EventHandler::Suspend;
            }
            break;

         default:
            //
            //  Illegal event handler return code.
            //
            Context::Kill("invalid result", pack2(rc, sid_));
         }

         //  If there are modifiers on the SSMQ, create an SNP event
         //  and pass it down the SSMQ.
         //
         if(!ssmq_.Empty())
         {
            snpEvent = currEvent->BuildSnp(*this, nextSnp_);
            if(snpEvent != nullptr) ProcessSsmqSnp(ssmq_.First(), *snpEvent);
         }

         //  If the SSM has defined this to be an SNP where modifiers
         //  can be triggered, pass an SNP event down the InitQ if it
         //  contains any modifiers.
         //
         tid = nextSnp_;
         if(tid == NIL_ID) break;
         trigger = GetService()->GetTrigger(tid);

         if(trigger != nullptr)
         {
            modifierInit = trigger->initq_.First();

            if(modifierInit != nullptr)
            {
               if(snpEvent == nullptr)
                  snpEvent = currEvent->BuildSnp(*this, tid);
               if(snpEvent != nullptr)
                  ProcessInitqSnp(trigger, modifierInit, *snpEvent);
            }
         }

         triggered_[tid] = true;
         nextSnp_ = NIL_ID;
         break;

      case FreeEventPhase:
         //
         //  Free the events that have just been processed and update this
         //  SSM's state.  Continue with the next event or exit.
         //
         if(currEvent != nextEvent)
         {
            if(currEvent != nullptr)
            {
               //  If the incoming and SAP events are the same, make sure
               //  not to free the event twice, which would happen if this
               //  SSM owned the event and the event's SAP was itself, as
               //  is the case for Analyze Message and Initiation Request.
               //
               if(currEvent == sapEvent) sapEvent = nullptr;

               if((currEvent->Owner() == this) &&
                  (currEvent->GetLocation() != Event::Saved))
               {
                  delete currEvent;
               }
               currEvent = nullptr;
            }
         }

         if(sapEvent != nullptr)
         {
            if((sapEvent->Owner() == this) &&
               (sapEvent->GetLocation() != Event::Saved))
            {
               delete sapEvent;
            }
            sapEvent = nullptr;
         }

         if(snpEvent != nullptr)
         {
            if(snpEvent->Owner() == this) delete snpEvent;
            snpEvent = nullptr;
         }

         currState_ = nextState_;

         switch(rc)
         {
         case EventHandler::Suspend:
            //
            //  Return unless there is a pending event.
            //
            currEvent = eventq_[Event::Pending].First();
            if(currEvent == nullptr) return EventHandler::Suspend;
            currEvent->SetLocation(Event::Active);
            phase = ModifierSapPhase;
            break;

         case EventHandler::Continue:
            //
            //  If nextEvent is ours, continue with the next event.  If it is
            //  an SAP owned by us, RestoreContext was invoked, and we must
            //  resume processing of the SSMQ or InitQ from the point where
            //  the previous transaction ended.
            //
            //  In rare cases, nextEvent can be owned by an ancestor.  This
            //  occurs if a modifier returns EventHandler::Revert with an
            //  event that is destined for its grandparent.
            //
            if(nextEvent->Owner() != this) return EventHandler::Continue;

            if(nextEvent->Eid() != Event::AnalyzeSap)
               phase = ModifierSapPhase;
            else if(((AnalyzeSapEvent*) nextEvent)->CurrInitiator() != nullptr)
               phase = InitiatorReentryPhase;
            else if(((AnalyzeSapEvent*) nextEvent)->CurrSsm() != nullptr)
               phase = ModifierReentryPhase;
            else
               Context::Kill("failed to route next event",
                  pack3(sid_, currState_, nextEvent->Eid()));

            currEvent = nextEvent;
            nextEvent = nullptr;
            break;

         default:
            //
            //  If any pending events exist, we face a dilemma.  Processing
            //  of the last event determined that we should exit this SSM
            //  and continue processing in another.  If we dequeue an event
            //  instead, the outcome will probably be different.  How to
            //  resolve this is unclear, so block it until a use case arises.
            //
            currEvent = eventq_[Event::Pending].First();

            while(currEvent != nullptr)
            {
               EventError1(currEvent);
               currEvent = eventq_[Event::Pending].First();
            }

            return rc;
         }
         break;

      default:
         //
         //  Illegal event routing phase.
         //
         Context::Kill("invalid phase", pack2(sid_, phase));
      }
   }
}

//------------------------------------------------------------------------------

fn_name ServiceSM_ProcessInitAck = "ServiceSM.ProcessInitAck";

EventHandler::Rc ServiceSM::ProcessInitAck(Event& currEvent, Event*& nextEvent)
{
   Debug::ft(ServiceSM_ProcessInitAck);

   Context::Kill(strOver(this), sid_);
   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

fn_name ServiceSM_ProcessInitNack = "ServiceSM.ProcessInitNack";

EventHandler::Rc ServiceSM::ProcessInitNack(Event& currEvent, Event*& nextEvent)
{
   Debug::ft(ServiceSM_ProcessInitNack);

   //  This function must be overridden if it can be invoked.
   //
   Context::Kill(strOver(this), sid_);
   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

fn_name ServiceSM_ProcessInitqSap = "ServiceSM.ProcessInitqSap";

EventHandler::Rc ServiceSM::ProcessInitqSap
   (const Trigger* trigger, const Initiator* modifier, Event& sapEvent,
   Event*& nextEvent, Phase& phase)
{
   Debug::ft(ServiceSM_ProcessInitqSap);

   InitiationReqEvent* initEvent;

   while(true)
   {
      auto rc = modifier->InvokeHandler(*this, sapEvent, nextEvent);

      switch(rc)
      {
      case EventHandler::Pass:
         //
         //  If there is another initiator, pass the SAP to it.
         //  Otherwise, return and proceed to our event handler.
         //
         modifier = trigger->initq_.Next(*modifier);

         if(modifier == nullptr)
         {
            triggered_[trigger->Tid()] = true;
            return EventHandler::Continue;
         }
         break;

      case EventHandler::Initiate:
         //
         //  Process the initiation request and then delete it.
         //  Continue traversing the InitQ if told to resume.
         //
         sapEvent.SetCurrInitiator(modifier);
         initEvent = static_cast< InitiationReqEvent* >(nextEvent);
         nextEvent = nullptr;

         if(sapEvent.Eid() == Event::AnalyzeSap)
            initEvent->SetSapEvent(static_cast< AnalyzeSapEvent& >(sapEvent));

         rc = ProcessInitReq(*initEvent, nextEvent, phase);
         delete initEvent;
         initEvent = nullptr;

         if(rc != EventHandler::Resume) return rc;

         modifier = trigger->initq_.Next(*modifier);

         if(modifier == nullptr)
         {
            triggered_[trigger->Tid()] = true;
            return EventHandler::Continue;
         }
         break;

      default:
         //
         //  Initiator::InvokeHandler should have prevented this.
         //
         Context::Kill("invalid result", pack2(modifier->Sid(), rc));
      }
   }
}

//------------------------------------------------------------------------------

fn_name ServiceSM_ProcessInitqSnp = "ServiceSM.ProcessInitqSnp";

void ServiceSM::ProcessInitqSnp
   (const Trigger* trigger, const Initiator* modifier, Event& snpEvent)
{
   Debug::ft(ServiceSM_ProcessInitqSnp);

   Event* initEvent;
   Event* nextEvent = nullptr;
   auto phase = LocalEventPhase;

   while(true)
   {
      auto rc = modifier->InvokeHandler(*this, snpEvent, nextEvent);

      switch(rc)
      {
      case EventHandler::Pass:
         break;

      case EventHandler::Initiate:
         //
         //  Process the initiation request and delete it.
         //
         initEvent = nextEvent;
         nextEvent = nullptr;

         rc = ProcessInitReq(*initEvent, nextEvent, phase);

         if(rc != EventHandler::Resume)
         {
            //  Generate a log with the initiated modifier's service
            //  identifier.  Free any next event.
            //
            auto sibling = static_cast< InitiationReqEvent* >
               (initEvent)->GetModifier();
            Debug::SwLog(ServiceSM_ProcessInitqSnp, sibling, rc);

            if(nextEvent != nullptr)
            {
               Debug::SwLog(ServiceSM_ProcessInitqSnp,
                  sibling, nextEvent->Eid());
               delete nextEvent;
               nextEvent = nullptr;
            }
         }

         delete initEvent;
         initEvent = nullptr;
         break;

      default:
         //
         //  Initiator::InvokeHandler should have prevented this.
         //
         Context::Kill("invalid result", pack2(modifier->Sid(), rc));
      }

      //  If there is another modifier, pass the SAP to it.
      //  Otherwise, return and proceed to the next event.
      //
      modifier = trigger->initq_.Next(*modifier);
      if(modifier == nullptr) return;
   }
}

//------------------------------------------------------------------------------

fn_name ServiceSM_ProcessInitReq = "ServiceSM.ProcessInitReq";

EventHandler::Rc ServiceSM::ProcessInitReq
   (Event& currEvent, Event*& nextEvent, Phase& phase)
{
   Debug::ft(ServiceSM_ProcessInitReq);

   auto& initEvent = static_cast< InitiationReqEvent& >(currEvent);
   auto rc = EventHandler::Pass;

   //  This function only handles initiation requests made by *Initiators*.
   //  Pass the initiation request to each SSM in the SSMQ.  Each SSM should
   //  pass the request onward without setting another event; in rare cases,
   //  processing may be suspended.  When the end of the SSMQ is reached, or
   //  as soon as the requested modifier is denied, create and invoke that
   //  modifier if we are in the context of its parent.
   //
   for(ServiceSM* curr = ssmq_.First(), *next; curr != nullptr; curr = next)
   {
      next = ssmq_.Next(*curr);

      rc = curr->ProcessEvent(&currEvent, nextEvent);

      switch(rc)
      {
      case EventHandler::Pass:
      case EventHandler::Suspend:
         break;

      default:
         Debug::SwLog(ServiceSM_ProcessInitReq, curr->Sid(), rc);
         rc = EventHandler::Pass;
      }

      if(nextEvent != nullptr) curr->EventError1(nextEvent);

      if(rc == EventHandler::Suspend) return EventHandler::Suspend;

      curr->DeleteIdleModifier();
      if(initEvent.WasDenied()) break;
   }

   if(currEvent.Owner() != this) return rc;

   auto reg = Singleton< ServiceRegistry >::Instance();
   auto svc = reg->GetService(initEvent.GetModifier());
   auto modifier = svc->AllocModifier();
   if(modifier == nullptr) return EventHandler::Pass;

   HenqModifier(*modifier);
   initEvent.SetScreening(false);

   rc = modifier->ProcessEvent(&currEvent, nextEvent);

   switch(rc)
   {
   case EventHandler::Suspend:
      phase = FreeEventPhase;
      break;

   case EventHandler::Revert:
      phase = FreeEventPhase;
      rc = EventHandler::Continue;
      break;

   case EventHandler::Resume:
      break;

   default:
      //
      //  Other return codes are unlikely.  An event has been flagged as an
      //  SAP or SNP, which prompted an initiator to create an initiation
      //  request.  After the request is handled
      //  o EventHandler::Continue should have stayed within the new modifier.
      //  o EventHandler::Pass could be interpreted as wanting to continue
      //    with the processing of the original event (if at an SAP).  But
      //    if the new modifier does not need to divert its parent from its
      //    usual path, it should probably trigger at an SNP, not an SAP.
      //  o EventHandler::Initiate would be a request to initiate a sibling.
      //
      Debug::SwLog(ServiceSM_ProcessInitReq, modifier->Sid(), rc);
      delete nextEvent;
      nextEvent = nullptr;
      rc = EventHandler::Resume;
   }

   modifier->DeleteIdleModifier();
   return rc;
}

//------------------------------------------------------------------------------

fn_name ServiceSM_ProcessSap = "ServiceSM.ProcessSap";

EventHandler::Rc ServiceSM::ProcessSap(Event& currEvent, Event*& nextEvent)
{
   Debug::ft(ServiceSM_ProcessSap);

   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

fn_name ServiceSM_ProcessSip = "ServiceSM.ProcessSip";

EventHandler::Rc ServiceSM::ProcessSip(Event& currEvent, Event*& nextEvent)
{
   Debug::ft(ServiceSM_ProcessSip);

   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

fn_name ServiceSM_ProcessSnp = "ServiceSM.ProcessSnp";

EventHandler::Rc ServiceSM::ProcessSnp(Event& currEvent, Event*& nextEvent)
{
   Debug::ft(ServiceSM_ProcessSnp);

   return EventHandler::Pass;
}

//------------------------------------------------------------------------------

fn_name ServiceSM_ProcessSsmqSap = "ServiceSM.ProcessSsmqSap";

EventHandler::Rc ServiceSM::ProcessSsmqSap
   (ServiceSM* modifier, Event& sapEvent, Event*& nextEvent, Phase& phase)
{
   Debug::ft(ServiceSM_ProcessSsmqSap);

   for(ServiceSM* curr = modifier, *next; curr != nullptr; curr = next)
   {
      next = ssmq_.Next(*curr);
      sapEvent.SetCurrSsm(curr);
      auto rc = curr->ProcessEvent(&sapEvent, nextEvent);

      switch(rc)
      {
      case EventHandler::Pass:
         //
         //  If there is another modifier, pass the SAP to it.
         //  Otherwise, proceed to the InitQ.
         //
         break;

      case EventHandler::Revert:
         //
         //  Stop routing this event.  Route the next one.
         //
         phase = FreeEventPhase;
         rc = EventHandler::Continue;
         break;

      case EventHandler::Suspend:
         //
         //  This event has been handled.
         //
         phase = FreeEventPhase;
         rc = EventHandler::Suspend;
         break;

      default:
         //
         //  Other return codes are illegal.  Treat them as
         //  EventHandler::Pass after deleting any next event.
         //
         Debug::SwLog(ServiceSM_ProcessSsmqSap, curr->Sid(), rc);
         if(nextEvent != nullptr) curr->EventError1(nextEvent);
         rc = EventHandler::Pass;
      }

      curr->DeleteIdleModifier();

      if(rc != EventHandler::Pass) return rc;
   }

   return EventHandler::Continue;
}

//------------------------------------------------------------------------------

fn_name ServiceSM_ProcessSsmqSnp = "ServiceSM.ProcessSsmqSnp";

void ServiceSM::ProcessSsmqSnp(ServiceSM* modifier, Event& snpEvent)
{
   Debug::ft(ServiceSM_ProcessSsmqSnp);

   Event* nextEvent = nullptr;

   //  Pass the SNP to each SSM in the SSMQ.  Each SSM should pass the SNP
   //  onward without setting another event.  When the end of the SSMQ is
   //  reached, return to traverse the InitQ.
   //
   for(ServiceSM* curr = modifier, *next; curr != nullptr; curr = next)
   {
      next = ssmq_.Next(*curr);
      auto rc = curr->ProcessEvent(&snpEvent, nextEvent);
      if(rc != EventHandler::Pass)
      {
         Debug::SwLog(ServiceSM_ProcessSsmqSnp, curr->Sid(), rc);
      }
      if(nextEvent != nullptr) curr->EventError1(nextEvent);
      curr->DeleteIdleModifier();
   }
}

//------------------------------------------------------------------------------

fn_name ServiceSM_PsmDeleted = "ServiceSM.PsmDeleted";

void ServiceSM::PsmDeleted(ProtocolSM& exPsm)
{
   Debug::ft(ServiceSM_PsmDeleted);

   for(auto mod = ssmq_.First(); mod != nullptr; ssmq_.Next(mod))
   {
      mod->PsmDeleted(exPsm);
   }
}

//------------------------------------------------------------------------------

fn_name ServiceSM_SetNextSap = "ServiceSM.SetNextSap";

void ServiceSM::SetNextSap(TriggerId sap)
{
   Debug::ft(ServiceSM_SetNextSap);

   if(!Trigger::IsValidId(sap)) return;

   nextSap_ = sap;
}

//------------------------------------------------------------------------------

fn_name ServiceSM_SetNextSnp = "ServiceSM.SetNextSnp";

void ServiceSM::SetNextSnp(TriggerId snp)
{
   Debug::ft(ServiceSM_SetNextSnp);

   if(!Trigger::IsValidId(snp)) return;

   nextSnp_ = snp;
}

//------------------------------------------------------------------------------

fn_name ServiceSM_SetNextState = "ServiceSM.SetNextState";

void ServiceSM::SetNextState(StateId stid)
{
   Debug::ft(ServiceSM_SetNextState);

   nextState_ = stid;
   idled_ = (stid == Null);
}

//------------------------------------------------------------------------------

fn_name ServiceSM_SetParent = "ServiceSM.SetParent";

void ServiceSM::SetParent(ServiceSM& parent)
{
   Debug::ft(ServiceSM_SetParent);

   parentSsm_ = &parent;
}
}
