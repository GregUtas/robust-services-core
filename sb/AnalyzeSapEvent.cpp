//==============================================================================
//
//  AnalyzeSapEvent.cpp
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
#include "SbEvents.h"
#include <ostream>
#include <string>
#include "Context.h"
#include "Debug.h"
#include "Message.h"
#include "SbTrace.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "TraceBuffer.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
AnalyzeSapEvent::AnalyzeSapEvent(ServiceSM& owner, StateId currState,
   Event& currEvent, TriggerId tid) :
   Event(AnalyzeSap, &owner),
   currState_(currState),
   currEvent_(&currEvent),
   trigger_(tid),
   currSsm_(nullptr),
   currInit_(nullptr),
   savedMsg_(nullptr)
{
   Debug::ft("AnalyzeSapEvent.ctor");
}

//------------------------------------------------------------------------------

AnalyzeSapEvent::~AnalyzeSapEvent()
{
   Debug::ftnt("AnalyzeSapEvent.dtor");
}

//------------------------------------------------------------------------------

Event* AnalyzeSapEvent::BuildSap(ServiceSM& owner, TriggerId tid)
{
   Debug::ft("AnalyzeSapEvent.BuildSap");

   //  Second-order modifiers receive the Analyze SAP event in its
   //  original form.
   //
   return this;
}

//------------------------------------------------------------------------------

Event* AnalyzeSapEvent::BuildSnp(ServiceSM& owner, TriggerId tid)
{
   Debug::ft("AnalyzeSapEvent.BuildSnp");

   //  Notification is not provided after handling the Analyze SAP event.
   //
   return nullptr;
}

//------------------------------------------------------------------------------

void AnalyzeSapEvent::Capture
   (ServiceId sid, const State& state, EventHandler::Rc rc) const
{
   auto rec = new SxpTrace(sid, state, *this, rc);
   Singleton< TraceBuffer >::Instance()->Insert(rec);
}

//------------------------------------------------------------------------------

void AnalyzeSapEvent::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Event::Display(stream, prefix, options);

   stream << prefix << "currState : " << int(currState_) << CRLF;
   stream << prefix << "currEvent : " << currEvent_ << CRLF;
   stream << prefix << "trigger   : " << int(trigger_) << CRLF;
   stream << prefix << "currSsm   : " << currSsm_ << CRLF;
   stream << prefix << "currInit  : " << currInit_ << CRLF;
   stream << prefix << "savedMsg  : " << savedMsg_ << CRLF;
}

//------------------------------------------------------------------------------

void AnalyzeSapEvent::Free()
{
   Debug::ft("AnalyzeSapEvent.Free");

   //  Free the underlying event and the SAP.
   //
   currEvent_->Free();
   Event::Free();
}

//------------------------------------------------------------------------------

fn_name AnalyzeSapEvent_FreeContext = "AnalyzeSapEvent.FreeContext";

void AnalyzeSapEvent::FreeContext(bool freeMsg)
{
   Debug::ft(AnalyzeSapEvent_FreeContext);

   //  Before freeing the SAP, restore the saved message unless freeing it.
   //
   if(location_ == Saved)
   {
      if(!freeMsg) savedMsg_->Restore();
      savedMsg_->Unsave();
      savedMsg_ = nullptr;
      Free();
      return;
   }

   Debug::SwLog(AnalyzeSapEvent_FreeContext, "invalid location", location_);
}

//------------------------------------------------------------------------------

void AnalyzeSapEvent::Patch(sel_t selector, void* arguments)
{
   Event::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

Event* AnalyzeSapEvent::Restore(EventHandler::Rc& rc)
{
   Debug::ft("AnalyzeSapEvent.Restore");

   //  Restore the SAP, its underlying event, and the saved context message.
   //
   if(Event::Restore(rc) != nullptr)
   {
      if(currEvent_->Restore(rc) != nullptr)
      {
         if(savedMsg_->Restore())
         {
            savedMsg_->Unsave();
            savedMsg_ = nullptr;
            return this;
         }
      }

      Context::Kill("failed to restore event", rc);
   }

   return nullptr;
}

//------------------------------------------------------------------------------

Event* AnalyzeSapEvent::RestoreContext(EventHandler::Rc& rc)
{
   Debug::ft("AnalyzeSapEvent.RestoreContext");

   //  The Restore function handles everything.
   //
   return Restore(rc);
}

//------------------------------------------------------------------------------

bool AnalyzeSapEvent::Save()
{
   Debug::ft("AnalyzeSapEvent.Save");

   //  Save the SAP and its underlying event.
   //
   if(Event::Save())
   {
      if(currEvent_->Save()) return true;

      EventHandler::Rc rc;
      Event::Restore(rc);
   }

   return false;
}

//------------------------------------------------------------------------------

bool AnalyzeSapEvent::SaveContext()
{
   Debug::ft("AnalyzeSapEvent.SaveContext");

   //  Save the context message if successful in saving the SAP event.
   //
   if(Save())
   {
      auto msg = Context::ContextMsg();

      if(msg != nullptr)
      {
         msg->Save();
         savedMsg_ = msg;
         return true;
      }

      EventHandler::Rc rc;
      Restore(rc);
   }

   return false;
}
}
