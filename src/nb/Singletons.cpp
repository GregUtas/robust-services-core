//==============================================================================
//
//  Singletons.cpp
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
#include "Singletons.h"
#include <bitset>
#include <cstddef>
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "NbTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
struct SingletonTuple
{
   const Base** addr;  // pointer to a singleton's Instance_ pointer
   MemoryType type;    // the type of memory that the singleton uses

   SingletonTuple(const Base** a, MemoryType t) : addr(a), type(t) { }
};

//------------------------------------------------------------------------------
//
//> The maximum size of the registry.
//
constexpr size_t MaxSingletons = 32 * kBs;

//  The registry of singletons.
//
static Singletons* Instance_ = nullptr;

//------------------------------------------------------------------------------

Singletons::Singletons()
{
   Debug::ft("Singletons.ctor");

   registry_.Init(MaxSingletons);
   registry_.Reserve(MaxSingletons >> 4);
}

//------------------------------------------------------------------------------

fn_name Singletons_dtor = "Singletons.dtor";

Singletons::~Singletons()
{
   Debug::ftnt(Singletons_dtor);

   Debug::SwLog(Singletons_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

void Singletons::BindInstance(const Base** addr, MemoryType type)
{
   Debug::ft("Singletons.BindInstance");

   //  Singletons on the permanent or immutable heap do not have to
   //  be recorded, because those heaps always survive restarts.
   //
   switch(type)
   {
   case MemPermanent:
   case MemImmutable:
      return;
   }

   //  Add this singleton to the registry.
   //
   SingletonTuple entry(addr, type);
   registry_.PushBack(entry);
}

//------------------------------------------------------------------------------

void Singletons::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Object::Display(stream, prefix, options);

   stream << prefix << "size : " << registry_.Size() << CRLF;

   if(!options.test(DispVerbose)) return;

   auto lead = prefix + spaces(2);
   stream << prefix << "registry : " << CRLF;

   for(size_t i = 0; i < registry_.Size(); ++i)
   {
      const auto& entry = registry_[i];

      stream << lead << strIndex(i);
      stream << entry.addr;
      stream << spaces(2) << entry.type;
      stream << spaces(2) << strClass(*entry.addr) << CRLF;
   }
}

//------------------------------------------------------------------------------

Singletons* Singletons::Instance()
{
   if(Instance_ != nullptr) return Instance_;
   Instance_ = new Singletons;
   return Instance_;
}

//------------------------------------------------------------------------------

void Singletons::Patch(sel_t selector, void* arguments)
{
   Object::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void Singletons::Shutdown(RestartLevel level)
{
   Debug::ft("Singletons.Shutdown");

   MemoryType type;

   //  Determine the highest MemoryType that is affected by the restart.
   //
   switch(level)
   {
   case RestartWarm:
      type = MemTemporary;
      break;
   case RestartCold:
      type = MemSlab;
      break;
   case RestartReload:
      type = MemProtected;
      break;
   default:
      return;
   }

   //  Run through the registry and nullify all affected singleton Instance_
   //  pointers.   When an entry is nullified, the last entry moves into its
   //  slot to keep the array contiguous, so don't increment the loop index.
   //
   for(size_t i = 0; i < registry_.Size(); NO_OP)
   {
      auto entry = &registry_[i];

      if(entry->type <= type)
      {
         *entry->addr = nullptr;
         registry_.Erase(i);
      }
      else
      {
         ++i;
      }
   }
}

//------------------------------------------------------------------------------

void Singletons::UnbindInstance(const Base** addr)
{
   Debug::ftnt("Singletons.UnbindInstance");

   //  Search for a singleton whose Instance_ pointer matches ADDR and remove
   //  it from the registry.  Move the last entry into its slot to keep the
   //  array contiguous.
   //
   for(size_t i = 0; i < registry_.Size(); ++i)
   {
      if(registry_[i].addr == addr)
      {
         registry_.Erase(i);
         return;
      }
   }
}
}
