//==============================================================================
//
//  Service.cpp
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
#include "Service.h"
#include <iomanip>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "SbCliParms.h"
#include "SbHandlers.h"
#include "ServiceRegistry.h"
#include "Singleton.h"
#include "Trigger.h"

using namespace NodeBase;
using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fixed_string AnalyzeMsgEventStr      = "AnalyzeMsgEvent";
fixed_string AnalyzeSapEventStr      = "AnalyzeSapEvent";
fixed_string AnalyzeSnpEventStr      = "AnalyzeSnpEvent";
fixed_string InitiationEventStr      = "InitiationEvent";
fixed_string ForceTransitionEventStr = "ForceTransitionEvent";
fixed_string MediaFailureEventStr    = "MediaFailureEvent";

//------------------------------------------------------------------------------

Service::Service(Id sid, bool modifiable, bool modifier) :
   status_(NotRegistered),
   eventNames_{nullptr},
   modifiable_(modifiable),
   modifier_(modifier)
{
   Debug::ft("Service.ctor");

   sid_.SetId(sid);

   states_.Init(State::MaxId, State::CellDiff(), MemImmutable);
   handlers_.Init(EventHandler::MaxId, 0, MemImmutable, false);
   triggers_.Init(Trigger::MaxId, 0, MemImmutable, false);

   //  Add the service to the global service registry.
   //
   if(!Singleton<ServiceRegistry>::Instance()->BindService(*this)) return;

   status_ = Enabled;

   //  Register system event handlers and their corresponding event names.
   //  All services require the Analyze Message event handler.  There is
   //  no system-defined Media Failure event handler, but all services can
   //  receive this event.
   //
   BindSystemHandler
      (*Singleton<SbAnalyzeMessage>::Instance(), EventHandler::AnalyzeMsg);
   BindEventName(AnalyzeMsgEventStr, Event::AnalyzeMsg);
   BindEventName(AnalyzeSapEventStr, Event::AnalyzeSap);
   BindEventName(AnalyzeSnpEventStr, Event::AnalyzeSnp);
   BindEventName(MediaFailureEventStr, Event::MediaFailure);

   //  Only modifiers require the Analyze SAP and Analyze SNP event handlers.
   //
   if(modifier_)
   {
      BindSystemHandler
         (*Singleton<SbAnalyzeSap>::Instance(), EventHandler::AnalyzeSap);
      BindSystemHandler
         (*Singleton<SbAnalyzeSnp>::Instance(), EventHandler::AnalyzeSnp);
   }

   //  Only modifiable services require the Force Transition event handler.
   //
   if(modifiable_)
   {
      BindSystemHandler(*Singleton<SbForceTransition>::Instance(),
         EventHandler::ForceTransition);
      BindEventName(ForceTransitionEventStr, Event::ForceTransition);
   }

   //  Modifiable and modifier services require the initiation event handler.
   //
   if(modifiable_ || modifier_)
   {
      BindSystemHandler(*Singleton<SbInitiationReq>::Instance(),
         EventHandler::InitiationReq);
      BindEventName(InitiationEventStr, Event::InitiationReq);
   }
}

//------------------------------------------------------------------------------

fn_name Service_dtor = "Service.dtor";

Service::~Service()
{
   Debug::ftnt(Service_dtor);

   Debug::SwLog(Service_dtor, UnexpectedInvocation, 0);
   Singleton<ServiceRegistry>::Extant()->UnbindService(*this);
}

//------------------------------------------------------------------------------

fn_name Service_AllocModifier = "Service.AllocModifier";

ServiceSM* Service::AllocModifier() const
{
   Debug::ft(Service_AllocModifier);

   //  It is either illegal to allocate this service as a modifier, or it
   //  should have overridden this function.
   //
   Debug::SwLog(Service_AllocModifier,
      "invalid modifier", pack2(Sid(), modifier_));
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Service_BindEventName = "Service.BindEventName";

bool Service::BindEventName(NodeBase::c_string name, EventId eid)
{
   Debug::ft(Service_BindEventName);

   //  Before registering the event name, check that
   //  o the service is already registered
   //  o the event identifier is valid
   //
   if(status_ == NotRegistered)
   {
      Debug::SwLog(Service_BindEventName, "service not registered", Sid());
      return false;
   }

   if(!Event::IsValidId(eid))
   {
      Debug::SwLog(Service_BindEventName, "invalid event", eid);
      return false;
   }

   //  If an event name is already registered against EID,
   //  overwrite it after generating a warning.
   //
   if(eventNames_[eid] != nullptr)
   {
      Debug::SwLog(Service_BindEventName, "replacing event name", eid);
   }

   eventNames_[eid] = name;
   return true;
}

//------------------------------------------------------------------------------

fn_name Service_BindHandler = "Service.BindHandler";

bool Service::BindHandler(EventHandler& handler, EventHandlerId ehid)
{
   Debug::ft(Service_BindHandler);

   //  Before registering the event handler, check that
   //  o the service is already registered
   //  o the event handler identifier is valid
   //
   if(status_ == NotRegistered)
   {
      Debug::SwLog(Service_BindHandler,
         "service not registered", pack2(Sid(), ehid));
      return false;
   }

   if(!EventHandler::AppCanRegister(ehid))
   {
      Debug::SwLog(Service_BindHandler, "invalid event", pack2(Sid(), ehid));
      return false;
   }

   return handlers_.Insert(handler, ehid);
}

//------------------------------------------------------------------------------

fn_name Service_BindState = "Service.BindState";

bool Service::BindState(State& state)
{
   Debug::ft(Service_BindState);

   if(status_ == NotRegistered)
   {
      Debug::SwLog(Service_BindState, "service not registered", Sid());
      return false;
   }

   return states_.Insert(state);
}

//------------------------------------------------------------------------------

fn_name Service_BindSystemHandler = "Service.BindSystemHandler";

bool Service::BindSystemHandler(EventHandler& handler, EventHandlerId ehid)
{
   Debug::ft(Service_BindSystemHandler);

   //  Before registering the event handler, check that
   //  o the service is already registered
   //  o the event handler identifier is valid
   //
   if(status_ == NotRegistered)
   {
      Debug::SwLog(Service_BindSystemHandler, "service not registered", Sid());
      return false;
   }

   if(ehid >= EventHandler::NextId)
   {
      Debug::SwLog(Service_BindSystemHandler, "invalid event", ehid);
      return false;
   }

   return handlers_.Insert(handler, ehid);
}

//------------------------------------------------------------------------------

fn_name Service_BindTrigger = "Service.BindTrigger";

bool Service::BindTrigger(Trigger& trigger)
{
   Debug::ft(Service_BindTrigger);

   auto tid = trigger.Tid();

   //  Before registering the trigger, check that
   //  o the service is already registered
   //  o the service allows modifiers
   //
   if(status_ == NotRegistered)
   {
      Debug::SwLog(Service_BindTrigger, "service not registered", Sid());
      return false;
   }

   if(!modifiable_)
   {
      Debug::SwLog(Service_BindTrigger, "service not modifiable", Sid());
      return false;
   }

   return triggers_.Insert(trigger, tid);
}

//------------------------------------------------------------------------------

ptrdiff_t Service::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast<const Service*>(&local);
   return ptrdiff(&fake->sid_, fake);
}

//------------------------------------------------------------------------------

fn_name Service_Disable = "Service.Disable";

bool Service::Disable()
{
   Debug::ft(Service_Disable);

   //  If the service is registered, disable it.
   //
   if(status_ == NotRegistered)
   {
      Debug::SwLog(Service_Disable, "service not registered", Sid());
      return false;
   }

   FunctionGuard guard(Guard_ImmUnprotect);
   status_ = Disabled;
   return true;
}

//------------------------------------------------------------------------------

void Service::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "sid        : " << sid_.to_str() << CRLF;
   stream << prefix << "status     : " << status_ << CRLF;
   stream << prefix << "modifiable : ";
   stream << modifiable_ << CRLF;
   stream << prefix << "modifier   : ";
   stream << modifier_ << CRLF;
   stream << prefix << "states [State::Id]" << CRLF;
   states_.Display(stream, prefix + spaces(2), options);
   stream << prefix << "handlers [EventHandlerId]" << CRLF;
   handlers_.Display(stream, prefix + spaces(2), options);

   auto lead = prefix + spaces(2);
   stream << prefix << "eventNames [EventId]" << CRLF;

   for(auto i = 0; i <= Event::MaxId; ++i)
   {
      if(eventNames_[i] != nullptr)
      {
         stream << lead << strIndex(i) << eventNames_[i] << CRLF;
      }
   }

   stream << prefix << "triggers [TriggerId]" << CRLF;
   triggers_.Display(stream, lead, options);
}

//------------------------------------------------------------------------------

fn_name Service_Enable = "Service.Enable";

bool Service::Enable()
{
   Debug::ft(Service_Enable);

   //  If the service is registered, enable it.
   //
   if(status_ == NotRegistered)
   {
      Debug::SwLog(Service_Enable, "service not registered", Sid());
      return false;
   }

   FunctionGuard guard(Guard_ImmUnprotect);
   status_ = Enabled;
   return true;
}

//------------------------------------------------------------------------------

size_t Service::EventCount() const
{
   size_t count = 0;

   for(auto i = 0; i <= Event::MaxId; ++i)
   {
      if(EventName(i) != nullptr) ++count;
   }

   return count;
}

//------------------------------------------------------------------------------

c_string Service::EventName(EventId eid) const
{
   if(!Event::IsValidId(eid)) return nullptr;

   return eventNames_[eid];
}

//------------------------------------------------------------------------------

Trigger* Service::GetTrigger(TriggerId tid) const
{
   return triggers_.At(tid);
}

//------------------------------------------------------------------------------

void Service::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fixed_string UnknownPortStr = "Unknown port";
fixed_string UserPortStr = "User port";
fixed_string NetworkPortStr = "Network port";

c_string Service::PortName(PortId pid) const
{
   switch(pid)
   {
   case UserPort: return UserPortStr;
   case NetworkPort: return NetworkPortStr;
   }

   return UnknownPortStr;
}

//------------------------------------------------------------------------------

fixed_string ItemHeader = " Id  Name";
//                        |  3..<name>

size_t Service::Summarize(ostream& stream, uint32_t selector) const
{
   size_t count = 0;
   id_t id = NIL_ID;

   switch(selector)
   {
   case SummarizeStates:
      stream << "States for " << strClass(this) << ':' << CRLF;

      if(states_.Empty())
      {
         stream << spaces(3) << NoStatesExpl << CRLF;
         return 0;
      }

      stream << ItemHeader << CRLF;

      for(auto s = states_.First(); s != nullptr; states_.Next(s))
      {
         stream << setw(3) << s->Stid();
         stream << spaces(2) << strClass(s) << CRLF;
      }
      return states_.Size();

   case SummarizeEvents:
      stream << "Events for " << strClass(this) << ':' << CRLF;

      if(EventCount() == 0)
      {
         stream << spaces(2) << NoEventsExpl << CRLF;
         return 0;
      }

      stream << ItemHeader << CRLF;

      for(auto i = 0; i <= Event::MaxId; ++i)
      {
         auto name = EventName(i);

         if(name != nullptr)
         {
            ++count;
            stream << setw(3) << i;
            stream << spaces(2) << name << CRLF;
         }
      }
      return count;

   case SummarizeHandlers:
      stream << "Handlers for " << strClass(this) << ':' << CRLF;

      if(handlers_.Empty())
      {
         stream << spaces(2) << NoHandlersExpl << CRLF;
         return 0;
      }

      stream << ItemHeader << CRLF;

      for(auto h = handlers_.First(id); h != nullptr; h = handlers_.Next(id))
      {
         stream << setw(3) << id;
         stream << spaces(2) << strClass(h) << CRLF;
      }
      return handlers_.Size();

   case SummarizeTriggers:
      stream << "Triggers for " << strClass(this) << ':' << CRLF;

      if(triggers_.Empty())
      {
         stream << spaces(2) << NoTriggersExpl << CRLF;
         return 0;
      }

      stream << ItemHeader << CRLF;

      for(auto t = triggers_.First(id); t != nullptr; t = triggers_.Next(id))
      {
         stream << setw(3) << id;
         stream << spaces(2) << strClass(t) << CRLF;
      }
      return triggers_.Size();
   }

   return 0;
}

//------------------------------------------------------------------------------

void Service::UnbindState (State& state)
{
   Debug::ftnt("Service.UnbindState");

   states_.Erase(state);
}
}
