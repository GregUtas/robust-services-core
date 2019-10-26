//==============================================================================
//
//  ThreadRegistry.cpp
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
#include "ThreadRegistry.h"
#include "StatisticsGroup.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "NbCliParms.h"
#include "Singleton.h"
#include "Thread.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
class ThreadStatsGroup : public StatisticsGroup
{
public:
   ThreadStatsGroup();
   ~ThreadStatsGroup();
   void DisplayStats
      (ostream& stream, id_t id, const Flags& options) const override;
};

//------------------------------------------------------------------------------

fn_name ThreadStatsGroup_ctor = "ThreadStatsGroup.ctor";

ThreadStatsGroup::ThreadStatsGroup() : StatisticsGroup("Threads [Thread::Id]")
{
   Debug::ft(ThreadStatsGroup_ctor);
}

//------------------------------------------------------------------------------

fn_name ThreadStatsGroup_dtor = "ThreadStatsGroup.dtor";

ThreadStatsGroup::~ThreadStatsGroup()
{
   Debug::ft(ThreadStatsGroup_dtor);
}

//------------------------------------------------------------------------------

fn_name ThreadStatsGroup_DisplayStats = "ThreadStatsGroup.DisplayStats";

void ThreadStatsGroup::DisplayStats
   (ostream& stream, id_t id, const Flags& options) const
{
   Debug::ft(ThreadStatsGroup_DisplayStats);

   StatisticsGroup::DisplayStats(stream, id, options);

   auto reg = Singleton< ThreadRegistry >::Instance();

   if(id == 0)
   {
      auto& threads = reg->Threads();

      for(auto t = threads.First(); t != nullptr; threads.Next(t))
      {
         t->DisplayStats(stream, options);
      }
   }
   else
   {
      auto t = reg->GetThread(id);

      if(t == nullptr)
      {
         stream << spaces(2) << NoThreadExpl << CRLF;
         return;
      }

      t->DisplayStats(stream, options);
   }
}

//==============================================================================

fn_name ThreadRegistry_ctor = "ThreadRegistry.ctor";

ThreadRegistry::ThreadRegistry()
{
   Debug::ft(ThreadRegistry_ctor);

   threads_.Init(Thread::MaxId, Thread::CellDiff(), MemPerm);
   statsGroup_.reset(new ThreadStatsGroup);
   ids_.reset(new IdMap);
}

//------------------------------------------------------------------------------

fn_name ThreadRegistry_dtor = "ThreadRegistry.dtor";

ThreadRegistry::~ThreadRegistry()
{
   Debug::ft(ThreadRegistry_dtor);
}

//------------------------------------------------------------------------------

fn_name ThreadRegistry_AssociateIds = "ThreadRegistry.AssociateIds";

void ThreadRegistry::AssociateIds(const Thread& thread)
{
   Debug::ft(ThreadRegistry_AssociateIds);

   //  Add an entry that maps the thread's SysThreadId to its ThreadId.
   //
   auto tid = thread.Tid();
   auto nid = thread.NativeThreadId();
   auto result = ids_->insert(IdPair(nid, tid));

   //  If the entry couldn't be added, NID was already in the table.
   //  It has been recycled, so update it with its new ThreadId.
   //
   if(!result.second)
   {
      auto& entry = *result.first;
      entry.second = tid;
   }
}

//------------------------------------------------------------------------------

fn_name ThreadRegistry_BindThread = "ThreadRegistry.BindThread";

bool ThreadRegistry::BindThread(Thread& thread)
{
   Debug::ft(ThreadRegistry_BindThread);

   if(!threads_.Insert(thread)) return false;
   AssociateIds(thread);
   return true;
}

//------------------------------------------------------------------------------

fn_name ThreadRegistry_ClaimBlocks = "ThreadRegistry.ClaimBlocks";

void ThreadRegistry::ClaimBlocks()
{
   Debug::ft(ThreadRegistry_ClaimBlocks);

   //  Have all threads mark themselves and their objects as being in use.
   //
   for(auto t = threads_.First(); t != nullptr; threads_.Next(t))
   {
      t->ClaimBlocks();
   }
}

//------------------------------------------------------------------------------

void ThreadRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Permanent::Display(stream, prefix, options);

   stream << prefix << "statsGroup        : ";
   stream << strObj(statsGroup_.get()) << CRLF;

   stream << prefix << "threads [ThreadId]" << CRLF;
   threads_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

fn_name ThreadRegistry_FindThread = "ThreadRegistry.FindThread";

Thread* ThreadRegistry::FindThread(SysThreadId nid) const
{
   Debug::ft(ThreadRegistry_FindThread);

   auto tid = FindThreadId(nid);
   if(tid == NIL_ID) return nullptr;

   auto thr = threads_.At(tid);
   if(thr == nullptr) return nullptr;
   if(thr->NativeThreadId() != nid) return nullptr;
   return thr;
}

//------------------------------------------------------------------------------

ThreadId ThreadRegistry::FindThreadId(SysThreadId nid) const
{
   auto entry = ids_->find(nid);
   if(entry == ids_->cend()) return NIL_ID;
   return entry->second;
}

//------------------------------------------------------------------------------

Thread* ThreadRegistry::GetThread(ThreadId tid) const
{
   return threads_.At(tid);
}

//------------------------------------------------------------------------------

void ThreadRegistry::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name ThreadRegistry_Restarting = "ThreadRegistry.Restarting";

size_t ThreadRegistry::Restarting(RestartLevel level) const
{
   Debug::ft(ThreadRegistry_Restarting);

   size_t count = 0;

   for(auto t = threads_.Last(); t != nullptr; threads_.Prev(t))
   {
      if(t->Restarting(level)) ++count;
   }

   return count;
}

//------------------------------------------------------------------------------

fn_name ThreadRegistry_Shutdown = "ThreadRegistry.Shutdown";

void ThreadRegistry::Shutdown(RestartLevel level)
{
   Debug::ft(ThreadRegistry_Shutdown);

   for(auto t = threads_.Last(); t != nullptr; threads_.Prev(t))
   {
      t->Shutdown(level);
   }

   if(level < RestartCold) return;

   statsGroup_.release();
}

//------------------------------------------------------------------------------

fn_name ThreadRegistry_Startup = "ThreadRegistry.Startup";

void ThreadRegistry::Startup(RestartLevel level)
{
   Debug::ft(ThreadRegistry_Startup);

   //  This starts up all threads that survived the restart.
   //
   if(statsGroup_ == nullptr) statsGroup_.reset(new ThreadStatsGroup);

   for(auto t = threads_.First(); t != nullptr; threads_.Next(t))
   {
      t->Startup(level);
   }
}

//------------------------------------------------------------------------------

fn_name ThreadRegistry_UnbindThread = "ThreadRegistry.UnbindThread";

void ThreadRegistry::UnbindThread(Thread& thread)
{
   Debug::ft(ThreadRegistry_UnbindThread);

   threads_.Erase(thread);
}
}
