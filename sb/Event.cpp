//==============================================================================
//
//  Event.cpp
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
#include "Event.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Context.h"
#include "Debug.h"
#include "RootServiceSM.h"
#include "SbEvents.h"
#include "SbPools.h"
#include "SbTrace.h"
#include "Singleton.h"
#include "TimePoint.h"
#include "ToolTypes.h"
#include "TraceBuffer.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name Event_ctor = "Event.ctor";

Event::Event(Id eid, ServiceSM* owner, Location loc) :
   eid_(eid),
   owner_(owner),
   location_(loc)
{
   Debug::ft(Event_ctor);

   if(loc == Saved) Debug::SwLog(Event_ctor, "invalid location", loc);

   if(owner_ != nullptr)
   {
      owner_->EnqEvent(*this, loc);
   }
   else
   {
      auto root = Context::ContextRoot();

      if(root != nullptr)
      {
         Debug::SwLog(Event_ctor, "owner should be root SSM", root->Sid());
      }
   }

   //  Record the event's creation if this context is traced.
   //
   TransTrace* trans = nullptr;

   if(Context::RunningContextTraced(trans))
   {
      auto warp = TimePoint::Now();
      auto buff = Singleton< TraceBuffer >::Instance();

      if(buff->ToolIsOn(ContextTracer))
      {
         auto rec = new EventTrace(EventTrace::Creation, *this);
         buff->Insert(rec);
      }

      if(trans != nullptr) trans->ResumeTime(warp);
   }
}

//------------------------------------------------------------------------------

fn_name Event_dtor = "Event.dtor";

Event::~Event()
{
   Debug::ftnt(Event_dtor);

   //  Record the event's deletion if this context is traced.
   //
   TransTrace* trans = nullptr;

   if(Context::RunningContextTraced(trans))
   {
      auto warp = TimePoint::Now();
      auto buff = Singleton< TraceBuffer >::Extant();

      if(buff->ToolIsOn(ContextTracer))
      {
         auto rec = new EventTrace(EventTrace::Deletion, *this);
         buff->Insert(rec);
      }

      if(trans != nullptr) trans->ResumeTime(warp);
   }

   if(owner_ != nullptr) owner_->ExqEvent(*this, location_);
}

//------------------------------------------------------------------------------

fn_name Event_BuildSap = "Event.BuildSap";

Event* Event::BuildSap(ServiceSM& owner, TriggerId tid)
{
   Debug::ft(Event_BuildSap);

   return new AnalyzeSapEvent(owner, owner.CurrState(), *this, tid);
}

//------------------------------------------------------------------------------

fn_name Event_BuildSnp = "Event.BuildSnp";

Event* Event::BuildSnp(ServiceSM& owner, TriggerId tid)
{
   Debug::ft(Event_BuildSnp);

   return new AnalyzeSnpEvent
      (owner, owner.CurrState(), owner.NextState(), *this, tid);
}

//------------------------------------------------------------------------------

void Event::Capture
   (ServiceId sid, const State& state, EventHandler::Rc rc) const
{
   auto rec = new HandlerTrace(sid, state, *this, rc);
   Singleton< TraceBuffer >::Instance()->Insert(rec);
}

//------------------------------------------------------------------------------

void Event::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Pooled::Display(stream, prefix, options);

   stream << prefix << "eid      : " << int(eid_) << CRLF;
   stream << prefix << "owner    : " << owner_ << CRLF;
   stream << prefix << "location : " << location_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name Event_Free = "Event.Free";

void Event::Free()
{
   Debug::ft(Event_Free);

   //  To be freed using this function, an event must currently be saved.
   //
   if(location_ == Saved)
   {
      delete this;
      return;
   }

   Debug::SwLog(Event_Free, "invalid location", pack2(eid_, location_));
}

//------------------------------------------------------------------------------

fn_name Event_FreeContext = "Event.FreeContext";

void Event::FreeContext(bool freeMsg)
{
   Debug::ft(Event_FreeContext);

   //  Only certain events support this function.
   //
   Debug::SwLog(Event_FreeContext, "invalid event", eid_);
}

//------------------------------------------------------------------------------

fn_name Event_new = "Event.operator new";

void* Event::operator new(size_t size)
{
   Debug::ft(Event_new);

   return Singleton< EventPool >::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

void Event::Patch(sel_t selector, void* arguments)
{
   Pooled::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Event_Restore = "Event.Restore";

Event* Event::Restore(EventHandler::Rc& rc)
{
   Debug::ft(Event_Restore);

   //  To be restored, an event must currently be saved.
   //
   if(location_ == Saved)
   {
      SetLocation(Active);
      rc = EventHandler::Revert;
      return this;
   }

   Debug::SwLog(Event_Restore, "invalid location", pack2(eid_, location_));
   rc = EventHandler::Suspend;
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Event_RestoreContext = "Event.RestoreContext";

Event* Event::RestoreContext(EventHandler::Rc& rc)
{
   Debug::ft(Event_RestoreContext);

   //  Only certain events support this function.
   //
   Debug::SwLog(Event_RestoreContext, "invalid event", eid_);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Event_Save = "Event.Save";

bool Event::Save()
{
   Debug::ft(Event_Save);

   //  To be saved, an event must currently be in context.
   //
   if(location_ == Active)
   {
      SetLocation(Saved);
      return true;
   }

   Debug::SwLog(Event_Save, "invalid location", pack2(eid_, location_));
   return false;
}

//------------------------------------------------------------------------------

fn_name Event_SaveContext = "Event.SaveContext";

bool Event::SaveContext()
{
   Debug::ft(Event_SaveContext);

   //  Only certain events support this function.
   //
   Debug::SwLog(Event_SaveContext, "invalid event", eid_);
   return false;
}

//------------------------------------------------------------------------------

fn_name Event_SetCurrInitiator = "Event.SetCurrInitiator";

void Event::SetCurrInitiator(const Initiator* init)
{
   Debug::ft(Event_SetCurrInitiator);
}

//------------------------------------------------------------------------------

fn_name Event_SetCurrSsm = "Event.SetCurrSsm";

void Event::SetCurrSsm(ServiceSM* ssm)
{
   Debug::ft(Event_SetCurrSsm);
}

//------------------------------------------------------------------------------

fn_name Event_SetLocation = "Event.SetLocation";

void Event::SetLocation(Location loc)
{
   Debug::ft(Event_SetLocation);

   if(location_ != loc)
   {
      owner_->ExqEvent(*this, location_);
      location_ = loc;
      owner_->EnqEvent(*this, location_);
   }
}

//------------------------------------------------------------------------------

fn_name Event_SetOwner = "Event.SetOwner";

void Event::SetOwner(RootServiceSM& owner)
{
   Debug::ft(Event_SetOwner);

   if(owner_ != nullptr)
   {
      Debug::SwLog(Event_SetOwner,
         "owner already exists", pack2(owner.Sid(), eid_));
      return;
   }

   owner_ = &owner;
   location_ = Active;
   owner_->EnqEvent(*this, Active);
}
}
