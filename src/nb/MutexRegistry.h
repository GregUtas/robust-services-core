//==============================================================================
//
//  MutexRegistry.h
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
#ifndef MUTEXREGISTRY_H_INCLUDED
#define MUTEXREGISTRY_H_INCLUDED

#include "Permanent.h"
#include <string>
#include "NbTypes.h"
#include "Registry.h"

namespace NodeBase
{
   class SysMutex;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for mutexes.
//
class MutexRegistry : public Permanent
{
   friend class Singleton<MutexRegistry>;
   friend class SysMutex;
public:
   //  Deleted to prohibit copying.
   //
   MutexRegistry(const MutexRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   MutexRegistry& operator=(const MutexRegistry& that) = delete;

   //  Returns the mutex associated with NAME.
   //
   SysMutex* Find(const std::string& name) const;

   //  Releases all mutexes owned by the running thread.
   //
   void Abandon() const;

   //  Returns the registry.
   //
   const Registry<SysMutex>& Mutexes() const { return mutexes_; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;

   //  Overridden to display each mutex.
   //
   void Summarize(std::ostream& stream) const override;
private:
   //  Private because this is a singleton.
   //
   MutexRegistry();

   //  Private because this is a singleton.
   //
   ~MutexRegistry();

   //  Adds MUTEX to the registry.
   //
   bool BindMutex(SysMutex& mutex);

   //  Removes MUTEX from the registry.
   //
   void UnbindMutex(SysMutex& mutex);

   //  The registry of mutexes.
   //
   Registry<SysMutex> mutexes_;
};
}
#endif
