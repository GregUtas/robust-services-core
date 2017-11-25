//==============================================================================
//
//  ThreadRegistry.h
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
#ifndef THREADREGISTRY_H_INCLUDED
#define THREADREGISTRY_H_INCLUDED

#include "Permanent.h"
#include <cstddef>
#include <map>
#include <utility>
#include "NbTypes.h"
#include "Registry.h"
#include "SysDecls.h"
#include "SysTypes.h"

namespace NodeBase
{
   class Thread;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Global registry for threads.
//
class ThreadRegistry : public Permanent
{
   friend class Singleton< ThreadRegistry >;
   friend class Thread;
public:
   //  Returns the thread registered against TID.
   //
   Thread* GetThread(ThreadId tid) const;

   //  Returns the thread whose native identifier is NID.
   //
   Thread* FindThread(SysThreadId nid) const;

   //  Returns the registry of threads.  Used for iteration.
   //
   const Registry< Thread >& Threads() const { return threads_; }

   //  Returns the ThreadId associated with a native thread identifier.
   //  Returns NIL_ID if the native thread is unknown.
   //
   ThreadId FindThreadId(SysThreadId nid) const;

   //  Informs all threads that a restart is occurring.  Returns the
   //  number of threads that will exit instead of sleeping.
   //
   size_t Restarting(RestartLevel level) const;

   //  Overridden to claim all threads in the registry.
   //
   virtual void ClaimBlocks() override;

   //  Overridden for restarts.
   //
   virtual void Startup(RestartLevel level) override;

   //  Overridden for restarts.
   //
   virtual void Shutdown(RestartLevel level) override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   ThreadRegistry();

   //  Private because this singleton is not subclassed.
   //
   ~ThreadRegistry();

   //  Adds THREAD to the registry and returns the identifier assigned to
   //  it.  Returns NIL_ID if the registry is full.
   //
   bool BindThread(Thread& thread);

   //  Removes THREAD from the registry.
   //
   void UnbindThread(Thread& thread);

   //  Associates THREAD's identifier with its native thread identifier.
   //
   void AssociateIds(const Thread& thread);

   //  A table for mapping SysThreadIds to ThreadIds.
   //
   typedef std::map< SysThreadId, ThreadId > IdMap;

   //  A tuple that maps a SysThreadId to a ThreadId.
   //
   typedef std::pair< SysThreadId, ThreadId > IdPair;

   //  The global registry of threads.
   //
   Registry< Thread > threads_;

   //  The statistics group for per-thread statistics.
   //
   StatisticsGroupPtr statsGroup_;

   //  The table that maps a SysThreadId to a ThreadId.
   //
   std::unique_ptr< IdMap > ids_;
};
}
#endif
