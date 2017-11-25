//==============================================================================
//
//  Initiator.cpp
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
#include "Initiator.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "Event.h"
#include "Service.h"
#include "ServiceRegistry.h"
#include "ServiceSM.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "Trigger.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name Initiator_ctor = "Initiator.ctor";

Initiator::Initiator
   (ServiceId sid, ServiceId aid, TriggerId tid, Priority prio) :
   sid_(sid),
   aid_(aid),
   tid_(tid),
   prio_(prio)
{
   Debug::ft(Initiator_ctor);

   auto trg = GetTrigger();

   if(trg == nullptr)
   {
      Debug::SwErr(Initiator_ctor, pack3(sid_, aid_, tid_), 0);
      return;
   }

   trg->BindInitiator(*this);
}

//------------------------------------------------------------------------------

fn_name Initiator_dtor = "Initiator.dtor";

Initiator::~Initiator()
{
   Debug::ft(Initiator_dtor);

   auto trg = GetTrigger();

   if(trg == nullptr)
   {
      Debug::SwErr(Initiator_dtor, pack3(sid_, aid_, tid_), 0);
      return;
   }

   trg->UnbindInitiator(*this);
}

//------------------------------------------------------------------------------

void Initiator::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "sid  : " << int(sid_) << CRLF;
   stream << prefix << "aid  : " << int(aid_) << CRLF;
   stream << prefix << "tid  : " << int(tid_) << CRLF;
   stream << prefix << "prio : " << int(prio_) << CRLF;
   stream << prefix << "link : " << link_.to_str() << CRLF;
}

//------------------------------------------------------------------------------

fn_name Initiator_EventError = "Initiator.EventError";

EventHandler::Rc Initiator::EventError(Event*& evt, EventHandler::Rc rc) const
{
   Debug::ft(Initiator_EventError);

   Debug::SwErr(Initiator_EventError, evt->Eid(), sid_);
   delete evt;
   evt = nullptr;
   return rc;
}

//------------------------------------------------------------------------------

fn_name Initiator_GetTrigger = "Initiator.GetTrigger";

Trigger* Initiator::GetTrigger() const
{
   Debug::ft(Initiator_GetTrigger);

   auto svc = Singleton< ServiceRegistry >::Instance()->GetService(sid_);

   if(svc == nullptr)
   {
      Debug::SwErr(Initiator_GetTrigger, pack3(sid_, aid_, tid_), 0);
      return nullptr;
   }

   auto anc = Singleton< ServiceRegistry >::Instance()->GetService(aid_);

   if(anc == nullptr)
   {
      Debug::SwErr(Initiator_GetTrigger, pack3(sid_, aid_, tid_), 1);
      return nullptr;
   }

   auto trg = anc->GetTrigger(tid_);

   if(trg == nullptr)
   {
      Debug::SwErr(Initiator_GetTrigger, pack3(sid_, aid_, tid_), 2);
      return nullptr;
   }

   return trg;
}

//------------------------------------------------------------------------------

fn_name Initiator_InvokeHandler = "Initiator.InvokeHandler";

EventHandler::Rc Initiator::InvokeHandler
   (const ServiceSM& parentSsm, Event& icEvent, Event*& ogEvent) const
{
   Debug::ft(Initiator_InvokeHandler);

   //  When an initiator receives an event, it may only pass the event on or
   //  request the creation of its modifier.
   //
   auto svc = Singleton< ServiceRegistry >::Instance()->GetService(sid_);

   if(svc->GetStatus() != Service::Enabled) return EventHandler::Pass;

   auto rc = ProcessEvent(parentSsm, icEvent, ogEvent);

   switch(rc)
   {
   case EventHandler::Pass:
      //
      //  Check that no event was created.
      //
      if(ogEvent != nullptr)
      {
         return EventError(ogEvent, EventHandler::Pass);
      }
      break;

   case EventHandler::Initiate:
      //
      //  Check that an initiation request was created.
      //
      if(ogEvent == nullptr)
      {
         Debug::SwErr(Initiator_InvokeHandler, parentSsm.Sid(), sid_);
         return EventHandler::Pass;
      }

      if(ogEvent->Eid() != Event::InitiationReq)
      {
         return EventError(ogEvent, EventHandler::Pass);
      }
      break;

   default:
      Debug::SwErr(Initiator_InvokeHandler, sid_, rc);

      if(ogEvent != nullptr)
      {
         return EventError(ogEvent, EventHandler::Pass);
      }
      return EventHandler::Pass;
   }

   return rc;
}

//------------------------------------------------------------------------------

ptrdiff_t Initiator::LinkDiff()
{
   int local;
   auto fake = reinterpret_cast< const Initiator* >(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

void Initiator::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Initiator_ProcessEvent = "Initiator.ProcessEvent";

EventHandler::Rc Initiator::ProcessEvent
   (const ServiceSM& parentSsm, Event& icEvent, Event*& ogEvent) const
{
   Debug::ft(Initiator_ProcessEvent);

   Debug::SwErr(Initiator_ProcessEvent, parentSsm.Sid(), sid_);
   return EventHandler::Pass;
}
}
