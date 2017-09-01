//==============================================================================
//
//  Singletons.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "Singletons.h"
#include <bitset>
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
   const Base** addr;    // pointer to a singleton's Instance_ pointer
   MemoryType type;      // the type of memory that the singleton uses
};

//------------------------------------------------------------------------------

const size_t Singletons::MaxSingletons = 16 * 1024;
Singletons* Singletons::Instance_ = nullptr;

//------------------------------------------------------------------------------

fn_name Singletons_ctor = "Singletons.ctor";

Singletons::Singletons()
{
   Debug::ft(Singletons_ctor);

   registry_.Init(MaxSingletons, MemPerm);
   registry_.Reserve(MaxSingletons >> 4);
}

//------------------------------------------------------------------------------

fn_name Singletons_dtor = "Singletons.dtor";

Singletons::~Singletons()
{
   Debug::ft(Singletons_dtor);
}

//------------------------------------------------------------------------------

fn_name Singletons_BindInstance = "Singletons.BindInstance";

void Singletons::BindInstance(const Base** addr, MemoryType type)
{
   Debug::ft(Singletons_BindInstance);

   //  Singletons on the permanent or immutable heap do not have to
   //  be recorded, because those heaps always survive restarts.
   //
   switch(type)
   {
   case MemProt:
   case MemDyn:
   case MemTemp:
      break;
   default:
      return;
   }

   //  Add this singleton to the registry.
   //
   SingletonTuple entry;
   entry.addr = addr;
   entry.type = type;
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
      auto entry = registry_[i];

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

fn_name Singletons_Shutdown = "Singletons.Shutdown";

void Singletons::Shutdown(RestartLevel level)
{
   Debug::ft(Singletons_Shutdown);

   MemoryType type;

   //  Determine the highest MemoryType that is affected by the restart.
   //
   switch(level)
   {
   case RestartWarm:
      type = MemTemp;
      break;
   case RestartCold:
      type = MemDyn;
      break;
   case RestartReload:
      type = MemProt;
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

fn_name Singletons_UnbindInstance = "Singletons.UnbindInstance";

void Singletons::UnbindInstance(const Base** addr)
{
   Debug::ft(Singletons_UnbindInstance);

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
