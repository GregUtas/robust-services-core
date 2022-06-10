//==============================================================================
//
//  DeferredRegistry.cpp
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
#include "DeferredRegistry.h"
#include <bitset>
#include <chrono>
#include <iomanip>
#include <ostream>
#include <string>
#include "Debug.h"
#include "DeferredThread.h"
#include "Formatters.h"
#include "Q2Link.h"
#include "Restart.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
DeferredRegistry::DeferredRegistry() :
   corrupt_(false)
{
   Debug::ft("DeferredRegistry.ctor");

   itemq_.Init(Deferred::LinkDiff());
}

//------------------------------------------------------------------------------

fn_name DeferredRegistry_dtor = "DeferredRegistry.dtor";

DeferredRegistry::~DeferredRegistry()
{
   Debug::ftnt(DeferredRegistry_dtor);

   Debug::SwLog(DeferredRegistry_dtor, UnexpectedInvocation, 0);

   itemq_.Purge();
}

//------------------------------------------------------------------------------

void DeferredRegistry::ClaimBlocks()
{
   Debug::ft("DeferredRegistry.ClaimBlocks");

   //  Traverse the work item queue to verify that it has not been corrupted,
   //  claiming items on the queue while doing so.
   //
   if(corrupt_)
   {
      Restart::Initiate(RestartCold, DeferredQueueCorruption, 0);
   }

   corrupt_ = true;

   for(auto i = itemq_.First(); i != nullptr; itemq_.Next(i))
   {
      i->ClaimBlocks();
   }

   corrupt_ = false;
}

//------------------------------------------------------------------------------

void DeferredRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Dynamic::Display(stream, prefix, options);

   stream << prefix << "corrupt : " << corrupt_ << CRLF;
   stream << prefix << "itemq   : ";

   auto first = itemq_.First();

   if(first == nullptr)
   {
      stream << "(empty)" << CRLF;
      return;
   }

   auto lead = prefix + spaces(2);

   if(options.test(DispVerbose))
   {
      stream << CRLF;

      for(auto i = first; i != nullptr; itemq_.Next(i))
      {
         stream << lead << strObj(i) << CRLF;
      }
   }
   else
   {
      stream << prefix << "(first item only)" << CRLF;
      stream << lead << strObj(itemq_.First()) << CRLF;
   }
}

//------------------------------------------------------------------------------

void DeferredRegistry::Erase(Deferred* item)
{
   Debug::ft("DeferredRegistry.Erase");

   Exqueue(item);
   delete item;
}

//------------------------------------------------------------------------------

void DeferredRegistry::EraseAll(const Base* owner)
{
   Debug::ft("DeferredRegistry.EraseAll");

   if(owner == nullptr) return;

   for(Deferred* curr = itemq_.First(), *next; curr != nullptr; curr = next)
   {
      next = itemq_.Next(*curr);

      if(curr->owner_ == owner)
      {
         Erase(curr);
      }
   }
}

//------------------------------------------------------------------------------

void DeferredRegistry::Exqueue(Deferred* item)
{
   Debug::ft("DeferredRegistry.Exqueue");

   itemq_.Exq(*item);
}

//------------------------------------------------------------------------------

void DeferredRegistry::Insert(Deferred* item)
{
   Debug::ft("DeferredRegistry.Insert");

   //  DeferredThread wakes up 1000 msecs after it last began to run.
   //  If it last began to run over 500 msecs ago, the next timeouts
   //  will occur in less than half a second, so adjust ITEM's timeout.
   //  The minimum timeout is one second.
   //
   if(item == nullptr) return;

   if(item->secs_ == 0)
   {
      item->secs_ = 1;
   }
   else
   {
      auto thr = Singleton<DeferredThread>::Instance();
      auto incr = (thr->CurrTimeRunning().count() >= 500 ? 1 : 0);
      item->secs_ += incr;
   }

   itemq_.Enq(*item);
}

//------------------------------------------------------------------------------

void DeferredRegistry::NotifyAll(const Base* owner, Deferred::Event event)
{
   Debug::ft("DeferredRegistry.NotifyAll");

   if(owner == nullptr) return;

   for(Deferred* curr = itemq_.First(), *next; curr != nullptr; curr = next)
   {
      next = itemq_.Next(*curr);

      if(curr->owner_ == owner)
      {
         RaiseEvent(curr, event);
      }
   }
}

//------------------------------------------------------------------------------

void DeferredRegistry::Patch(sel_t selector, void* arguments)
{
   Dynamic::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void DeferredRegistry::RaiseEvent(Deferred* item, Deferred::Event event)
{
   Debug::ft("DeferredRegistry.RaiseEvent");

   itemq_.Exq(*item);
   item->EventHasOccurred(event);

   if(!item->link_.IsQueued())
   {
      delete item;
   }
}

//------------------------------------------------------------------------------

void DeferredRegistry::RaiseTimeouts()
{
   Debug::ft("DeferredRegistry.RaiseTimeouts");

   for(Deferred* curr = itemq_.First(), *next; curr != nullptr; curr = next)
   {
      next = itemq_.Next(*curr);

      if(--curr->secs_ <= 0)
      {
         RaiseEvent(curr, Deferred::Timeout);
      }
   }
}

//------------------------------------------------------------------------------

void DeferredRegistry::Shutdown(RestartLevel level)
{
   Debug::ft("DeferredRegistry.Shutdown");

   //  During a warm restart, delete all work items that don't want to survive.
   //
   if(level != RestartWarm) return;

   for(Deferred* curr = itemq_.First(), *next; curr != nullptr; curr = next)
   {
      next = itemq_.Next(*curr);

      if(!curr->warm_)
      {
         Erase(curr);
      }
   }
}

//------------------------------------------------------------------------------

fixed_string ItemHeader = "Secs  Warm?  Item / Owner";
//                        |   4      7..<item> / <owner>

void DeferredRegistry::Summarize(ostream& stream) const
{
   if(itemq_.Empty())
   {
      stream << spaces(2) << "[No items found.]" << CRLF;
      return;
   }

   stream << ItemHeader << CRLF;

   for(auto i = itemq_.First(); i != nullptr; itemq_.Next(i))
   {
      stream << setw(4) << i->secs_;
      stream << setw(7) << i->warm_;
      stream << spaces(2) << strObj(i);
      stream << " / " << strObj(i->owner_) << CRLF;
   }
}
}
