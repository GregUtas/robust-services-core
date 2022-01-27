//==============================================================================
//
//  ThreadRegistry.h
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
#ifndef THREADREGISTRY_H_INCLUDED
#define THREADREGISTRY_H_INCLUDED

#include "Permanent.h"
#include <cstddef>
#include <list>
#include <map>
#include <utility>
#include <vector>
#include "NbTypes.h"
#include "SysDecls.h"
#include "SysTypes.h"

namespace NodeBase
{
   class SysThread;
   class Thread;
}

//------------------------------------------------------------------------------

namespace NodeBase
{
//  The state of a thread.  This handles the following scenarios:
//  1. A thread runs before its Thread object is fully constructed.
//  2. A thread runs without a Thread object because
//     a) its constructor trapped after its native thread was created
//     b) its destructor trapped
//     c) it was deleted by another thread
//     d) it deleted itself
//  In case #1, the thread must wait to run.  In the other cases, it
//  must exit as soon as possible.
//
enum ThreadState
{
   NotRegistered,  // no entry for SysThreadId in ThreadRegistry
   Constructing,   // Thread under construction
   Constructed,    // waiting to enter Thread.Start or running
   Deleting,       // Thread destructor entered
   Deleted         // Thread unexpectedly deleted but still running
};

//------------------------------------------------------------------------------
//
//  Information about a thread.
//
struct ThreadInfo
{
   ThreadInfo(ThreadState state, SysThread* systhrd, Thread* thread);

   //  The thread's state;
   //
   ThreadState state_;

   //  The wrapper for the native thread.
   //
   SysThread* const systhrd_;

   //  The full RSC thread object.
   //
   Thread* thread_;
};

//------------------------------------------------------------------------------
//
//  Global registry for threads.
//
class ThreadRegistry : public Permanent
{
   friend class Singleton< ThreadRegistry >;
   friend class Thread;
   friend class ModuleRegistry;
public:
   //  Deleted to prohibit copying.
   //
   ThreadRegistry(const ThreadRegistry& that) = delete;

   //  Deleted to prohibit copy assignment.
   //
   ThreadRegistry& operator=(const ThreadRegistry& that) = delete;

   //  Returns the thread whose native identifier is NID.
   //
   Thread* FindThread(SysThreadId nid) const;

   //  Returns the number of threads in the registry.
   //
   static size_t Size();

   //  Returns all Threads sorted by ThreadId.
   //
   std::vector< Thread* > GetThreads() const;

   //  Returns the thread registered against TID.
   //
   Thread* GetThread(ThreadId tid) const;

   //  Overridden to be forwarded to all threads in the registry.
   //
   void ClaimBlocks() override;

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

   //  Overridden for restarts.
   //
   void Startup(RestartLevel level) override;
private:
   //  Private because this is a singleton.
   //
   ThreadRegistry();

   //  Private because this is a singleton.
   //
   ~ThreadRegistry();

   //  The type for the thread registry.
   //
   typedef std::map< SysThreadId, ThreadInfo > ThreadMap;

   //  Returns the registry of threads.  Used for iteration.
   //
   const ThreadMap& Threads() const { return threads_; }

   //  Puts SYSTHRD and THREAD in the Constructing state.
   //
   void Created(SysThread* systhrd, Thread* thread);

   //  Puts NID the Constructed state.
   //
   void Initialized(SysThreadId nid);

   //  Puts the running thread in STATE, with its wrapper being SYSTHRD.
   //
   void Destroying(ThreadState state, const SysThread* systhrd);

   //  Returns the running thread's state.  If it is Deleted, the thread
   //  is removed from the registry and must exit.  Returns Constructing
   //  if the thread is not found.
   //
   ThreadState GetState();

   //  Returns true if the running thread has been deleted.
   //
   bool IsDeleted() const;

   //  Removes NID from the registry.
   //
   void Erase(SysThreadId nid);

   //  Selects the next thread to run.
   //
   Thread* Select() const;

   //  Informs all threads that a restart is occurring.  Returns the
   //  threads that will exit instead of sleeping.
   //
   std::list< Thread* > Restarting(RestartLevel level) const;

   //  Updates THREADS by removing threads that are no longer registered.
   //
   void TrimThreads(std::list< Thread* >& threads) const;

   //  Sets THREAD's ThreadId when adding it to the registry.
   //
   void SetThreadId(Thread* thread) const;

   //  An entry for a thread.
   //
   typedef std::pair< SysThreadId, ThreadInfo > Entry;

   //  The threads.
   //
   ThreadMap threads_;

   //  The statistics group for per-thread statistics.
   //
   StatisticsGroupPtr statsGroup_;
};
}
#endif
