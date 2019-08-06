//==============================================================================
//
//  State.cpp
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
#include "State.h"
#include <bitset>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "EventHandler.h"
#include "Formatters.h"
#include "NbTypes.h"
#include "Service.h"
#include "ServiceRegistry.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
const State::Id State::MaxId = 63;

//------------------------------------------------------------------------------

fn_name State_ctor = "State.ctor";

State::State(ServiceId sid, Id stid) : sid_(sid)
{
   Debug::ft(State_ctor);

   stid_.SetId(stid);

   for(auto i = 0; i <= Event::MaxId; ++i)
   {
      handlers_[i] = NIL_ID;
   }

   for(auto i = 0; i <= MaxServicePortId; ++i)
   {
      msgAnalyzers_[i] = NIL_ID;
   }

   //  Check that the state's service is registered.
   //
   auto svc = Singleton< ServiceRegistry >::Instance()->GetService(sid);

   if(svc == nullptr)
   {
      Debug::SwLog(State_ctor, "service not found", pack2(sid, stid));
      return;
   }

   //  Register system-defined event handlers.
   //
   //  The Analyze Message event applies to every state.
   //
   handlers_[Event::AnalyzeMsg] = EventHandler::AnalyzeMsg;

   //  The Analyze SAP and SNP events only apply to a modifier's states.
   //
   if(svc->IsModifier())
   {
      handlers_[Event::AnalyzeSap] = EventHandler::AnalyzeSap;
      handlers_[Event::AnalyzeSnp] = EventHandler::AnalyzeSnp;
   }

   //  The Force Transition event applies to a modifiable service's states.
   //
   if(svc->IsModifiable())
   {
      handlers_[Event::ForceTransition] = EventHandler::ForceTransition;
   }

   //  The Initiation Request event applies to a modifiable service's state
   //  and to any modifier service's state.
   //
   if(svc->IsModifiable() || svc->IsModifier())
   {
      handlers_[Event::InitiationReq] = EventHandler::InitiationReq;
   }

   //  The Media Failure event applies to any service's state.
   //
   handlers_[Event::MediaFailure] = EventHandler::MediaFailure;

   //  Register the state with its service.
   //
   svc->BindState(*this);
}

//------------------------------------------------------------------------------

fn_name State_dtor = "State.dtor";

State::~State()
{
   Debug::ft(State_dtor);

   auto svc = Singleton< ServiceRegistry >::Instance()->GetService(sid_);
   if(svc != nullptr) svc->UnbindState(*this);
}

//------------------------------------------------------------------------------

fn_name State_BindEventHandler = "State.BindEventHandler";

bool State::BindEventHandler(EventHandlerId ehid, EventId eid)
{
   //  Check that
   //  o the event handler is not private to the framework
   //  o the event is not private to the framework
   //  o an event handler is not already registered
   //
   if(!EventHandler::AppCanUse(ehid))
   {
      Debug::SwLog(State_BindEventHandler, 
         "invalid event", pack3(sid_, Stid(), ehid));
      return false;
   }

   if(!Event::AppCanHandle(eid))
   {
      Debug::SwLog(State_BindEventHandler,
         "unexpected event", pack3(sid_, Stid(), ehid));
      return false;
   }

   if(handlers_[eid] != NIL_ID)
   {
      Debug::SwLog(State_BindEventHandler,
         "replacing event handler", pack3(sid_, Stid(), ehid));
   }

   handlers_[eid] = ehid;
   return true;
}

//------------------------------------------------------------------------------

fn_name State_BindMsgAnalyzer = "State.BindMsgAnalyzer";

bool State::BindMsgAnalyzer(EventHandlerId ehid, ServicePortId pid)
{
   //  Check that
   //  o the analyzer is not private to the framework
   //  o the port is valid
   //  o a message analyzer is not already registered
   //
   if(!EventHandler::AppCanUse(ehid))
   {
      Debug::SwLog(State_BindMsgAnalyzer,
         "invalid event", pack4(sid_, Stid(), ehid, pid));
      return false;
   }

   if(!Service::IsValidPortId(pid))
   {
      Debug::SwLog(State_BindMsgAnalyzer,
         "invalid ServicePortId", pack4(sid_, Stid(), ehid, pid));
      return false;
   }

   if(msgAnalyzers_[pid] != NIL_ID)
   {
      Debug::SwLog(State_BindMsgAnalyzer,
         "replacing message analyzer", pack4(sid_, Stid(), ehid, pid));
   }

   msgAnalyzers_[pid] = ehid;
   return true;
}

//------------------------------------------------------------------------------

ptrdiff_t State::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const State* >(&local);
   return ptrdiff(&fake->stid_, fake);
}

//------------------------------------------------------------------------------

void State::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   if(!options.test(DispVerbose)) return;

   auto svc = Singleton< ServiceRegistry >::Instance()->GetService(sid_);

   stream << prefix << "stid : " << stid_.to_str() << CRLF;
   stream << prefix << "sid  : " << int(sid_);
   stream << " (" << strObj(svc) << ')' << CRLF;

   auto lead1 = prefix + spaces(2);
   auto lead2 = prefix + spaces(4);

   stream << prefix << "handlers [EventId]" << CRLF;

   for(auto i = 0; i <= Event::MaxId; ++i)
   {
      if(handlers_[i] != NIL_ID)
      {
         stream << lead1 << '[' << strName(svc->EventName(i), i) << ']' << CRLF;
         stream << lead2 << strClass(svc->GetHandler(handlers_[i])) << CRLF;
      }
   }

   stream << prefix << "msgAnalyzers [ServicePortId]" << CRLF;

   for(auto i = 0; i <= MaxServicePortId; ++i)
   {
      if(msgAnalyzers_[i] != NIL_ID)
      {
         stream << lead1 << '[' << svc->PortName(i) << ']' << CRLF;
         stream << lead2 << strClass(svc->GetHandler(msgAnalyzers_[i])) << CRLF;
      }
   }
}

//------------------------------------------------------------------------------

EventHandlerId State::GetHandler(EventId eid) const
{
   if(!Event::IsValidId(eid)) return NIL_ID;
   return handlers_[eid];
}

//------------------------------------------------------------------------------

EventHandlerId State::MsgAnalyzer(ServicePortId pid) const
{
   if(!Service::IsValidPortId(pid)) return NIL_ID;
   return msgAnalyzers_[pid];
}

//------------------------------------------------------------------------------

void State::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}
}
