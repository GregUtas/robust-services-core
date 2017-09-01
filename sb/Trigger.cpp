//==============================================================================
//
//  Trigger.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Trigger.h"
#include <bitset>
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "Initiator.h"
#include "NbTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name Trigger_ctor = "Trigger.ctor";

Trigger::Trigger(Id tid) : tid_(tid)
{
   Debug::ft(Trigger_ctor);

   initq_.Init(Initiator::LinkDiff());
}

//------------------------------------------------------------------------------

fn_name Trigger_dtor = "Trigger.dtor";

Trigger::~Trigger()
{
   Debug::ft(Trigger_dtor);

   initq_.Purge();
}

//------------------------------------------------------------------------------

fn_name Trigger_BindInitiator = "Trigger.BindInitiator";

bool Trigger::BindInitiator(Initiator& init)
{
   Debug::ft(Trigger_BindInitiator);

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
   Protected::Display(stream, prefix, options);

   if(!options.test(DispVerbose)) return;

   stream << prefix << "tid   : " << int(tid_) << CRLF;
   stream << prefix << "initq : " << CRLF;
   initq_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

void Trigger::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Trigger_UnbindInitiator = "Trigger.UnbindInitiator";

void Trigger::UnbindInitiator(Initiator& init)
{
   Debug::ft(Trigger_UnbindInitiator);

   if(!initq_.Exq(init))
   {
      Debug::SwErr(Trigger_UnbindInitiator, tid_, init.Sid());
   }
}
}
