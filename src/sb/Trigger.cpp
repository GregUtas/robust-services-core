//==============================================================================
//
//  Trigger.cpp
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
#include "Trigger.h"
#include <bitset>
#include <iomanip>
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "Formatters.h"
#include "Initiator.h"
#include "NbTypes.h"

using namespace NodeBase;
using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
Trigger::Trigger(Id tid) : tid_(tid)
{
   Debug::ft("Trigger.ctor");

   initq_.Init(Initiator::LinkDiff());
}

//------------------------------------------------------------------------------

fn_name Trigger_dtor = "Trigger.dtor";

Trigger::~Trigger()
{
   Debug::ftnt(Trigger_dtor);

   Debug::SwLog(Trigger_dtor, UnexpectedInvocation, 0);
   initq_.Purge();
}

//------------------------------------------------------------------------------

bool Trigger::BindInitiator(Initiator& init)
{
   Debug::ft("Trigger.BindInitiator");

   auto prio = init.GetPriority();
   Initiator* prev = nullptr;

   for(auto curr = initq_.First(); curr != nullptr; initq_.Next(curr))
   {
      if(curr->GetPriority() < prio) break;
      prev = curr;
   }

   return initq_.Insert(prev, init);
}

//------------------------------------------------------------------------------

void Trigger::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   if(!options.test(DispVerbose)) return;

   stream << prefix << "tid   : " << int(tid_) << CRLF;
   stream << prefix << "initq : " << CRLF;
   initq_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

void Trigger::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fixed_string InitiatorHeader = "Priority  Service  Ancestor  Name";
//                             |       8        9        10..<name>

size_t Trigger::Summarize(ostream& stream, uint32_t selector) const
{
   stream << "Triggers for " << strClass(this) << ':' << CRLF;
   stream << InitiatorHeader << CRLF;

   for(auto i = initq_.First(); i != nullptr; initq_.Next(i))
   {
      stream << setw(8) << int(i->GetPriority());
      stream << setw(9) << i->Sid();
      stream << setw(10) << i->Aid();
      stream << spaces(2) << strClass(i) << CRLF;
   }

   return initq_.Size();
}

//------------------------------------------------------------------------------

fn_name Trigger_UnbindInitiator = "Trigger.UnbindInitiator";

void Trigger::UnbindInitiator(Initiator& init)
{
   Debug::ftnt(Trigger_UnbindInitiator);

   if(!initq_.Exq(init))
   {
      Debug::SwLog(Trigger_UnbindInitiator,
         "Exq failed", pack2(init.Sid(), tid_));
   }
}
}
