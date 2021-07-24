//==============================================================================
//
//  Singletons.h
//
//  Copyright (C) 2013-2021  Greg Utas
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
#ifndef SINGLETONS_H_INCLUDED
#define SINGLETONS_H_INCLUDED

#include "Permanent.h"
#include "Allocators.h"
#include "Array.h"
#include "SysTypes.h"

namespace NodeBase
{
   struct SingletonTuple;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for singletons.  This simplifies restart software because
//  Shutdown functions do not have to nullify a singleton's Instance_ pointer
//  when a restart frees the heap in which the singleton was created.  When
//  Singleton.Instance creates a singleton, it adds it to this registry, which
//  saves the location of the Instance_ pointer and the type of memory used by
//  the singleton.  This allows all affected singleton Instance_ pointers to
//  be cleared by this registry's Shutdown function.
//
class Singletons : public Permanent
{
public:
   //  Deleted to prohibit copying.
   //
   Singletons(const Singletons& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   Singletons& operator=(const Singletons& that) = delete;

   //  Returns the registry of singletons.
   //
   static Singletons* Instance();

   //  Adds INSTANCE, which uses TYPE, to the registry.
   //
   void BindInstance(const Base** addr, MemoryType type);

   //  Removes INSTANCE from the registry.
   //
   void UnbindInstance(const Base** addr);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden for restarts.
   //
   void Shutdown(RestartLevel level) override;
private:
   //  Private because this is a singleton.
   //
   Singletons();

   //  Private because this is a singleton.
   //
   ~Singletons();

   //  Information about each singleton.
   //
   Array< SingletonTuple, PermanentAllocator< SingletonTuple >> registry_;
};
}
#endif
