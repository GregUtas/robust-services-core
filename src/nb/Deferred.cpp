//==============================================================================
//
//  Deferred.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "Deferred.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "DeferredRegistry.h"
#include "Formatters.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "Thread.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
Deferred::Deferred(Base& owner, uint32_t secs, bool warm) :
   MsgBuffer(),
   owner_(&owner),
   secs_(secs),
   warm_(warm)
{
   Debug::ft("Deferred.ctor");

   auto reg = Singleton<DeferredRegistry>::Instance();
   reg->Insert(this);
}

//------------------------------------------------------------------------------

Deferred::~Deferred()
{
   Debug::ftnt("Deferred.dtor");
}

//------------------------------------------------------------------------------

void Deferred::Cleanup()
{
   Debug::ftnt("Deferred.Cleanup");

   Singleton<DeferredRegistry>::Instance()->Exqueue(this);
   MsgBuffer::Cleanup();
}

//------------------------------------------------------------------------------

void Deferred::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   MsgBuffer::Display(stream, prefix, options);

   stream << prefix << "link  : " << CRLF;
   link_.Display(stream, prefix + spaces(2));
   stream << prefix << "owner : " << owner_ << CRLF;
   stream << prefix << "secs  : " << secs_ << CRLF;
   stream << prefix << "warm  : " << warm_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name Deferred_EventHasOccurred = "Deferred.EventHasOccurred";

void Deferred::EventHasOccurred(Event event)
{
   Debug::ft(Deferred_EventHasOccurred);

   Debug::SwLog(Deferred_EventHasOccurred, strOver(this), 0);
}

//------------------------------------------------------------------------------

ptrdiff_t Deferred::LinkDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast<const Deferred*>(&local);
   return ptrdiff(&fake->link_, fake);
}

//------------------------------------------------------------------------------

void Deferred::Patch(sel_t selector, void* arguments)
{
   MsgBuffer::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void Deferred::Restart(uint32_t secs)
{
   Debug::ftnt("Deferred.Restart");

   auto reg = Singleton<DeferredRegistry>::Instance();
   reg->Exqueue(this);
   secs_ = secs;
   reg->Insert(this);
}

//------------------------------------------------------------------------------

void Deferred::SendToThread(Thread* thread)
{
   Debug::ftnt("Deferred.SendToThread");

   if(thread == nullptr) return;

   auto reg = Singleton<DeferredRegistry>::Instance();
   reg->Exqueue(this);
   thread->EnqMsg(*this);
}
}
