//==============================================================================
//
//  Daemon.cpp
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
#include "Daemon.h"
#include <ostream>
#include "Algorithms.h"
#include "DaemonRegistry.h"
#include "Debug.h"
#include "Formatters.h"
#include "InitThread.h"
#include "Restart.h"
#include "Singleton.h"
#include "ThreadAdmin.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name Daemon_ctor = "Daemon.ctor";

Daemon::Daemon(fixed_string name, size_t size) :
   name_(name),
   size_(size)
{
   Debug::ft(Daemon_ctor);

   if(name == nullptr)
   {
      Debug::SwLog(Daemon_ctor, "null name", 0);
      return;
   }

   Singleton< DaemonRegistry >::Instance()->BindDaemon(*this);
}

//------------------------------------------------------------------------------

fn_name Daemon_dtor = "Daemon.dtor";

Daemon::~Daemon()
{
   Debug::ft(Daemon_dtor);

   Singleton< DaemonRegistry >::Instance()->UnbindDaemon(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t Daemon::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const Daemon* >(&local);
   return ptrdiff(&fake->did_, fake);
}

//------------------------------------------------------------------------------

fn_name Daemon_CreateThread = "Daemon.CreateThread";

Thread* Daemon::CreateThread()
{
   Debug::ft(Daemon_CreateThread);

   Debug::SwLog(Daemon_CreateThread, strOver(this), 0);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name Daemon_CreateThreads = "Daemon.CreateThreads";

void Daemon::CreateThreads()
{
   Debug::ft(Daemon_CreateThreads);

   while(threads_.size() < size_)
   {
      auto thread = CreateThread();
      threads_.insert(thread);
      ThreadAdmin::Incr(ThreadAdmin::Recreations);
   }
}

//------------------------------------------------------------------------------

void Daemon::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Permanent::Display(stream, prefix, options);

   stream << prefix << "name : " << name_ << CRLF;
   stream << prefix << "did  : " << did_.to_str() << CRLF;
   stream << prefix << "size : " << size_ << CRLF;
   stream << prefix << "threads [ThreadId]" << CRLF;

   auto lead = prefix + spaces(2);

   for(auto t = threads_.cbegin(); t != threads_.cend(); ++t)
   {
      stream << lead << strIndex((*t)->Tid()) << strObj(*t) << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name Daemon_Find = "Daemon.Find";

Daemon::Iterator Daemon::Find(Thread* thread)
{
   Debug::ft(Daemon_Find);

   if(thread == nullptr)
   {
      Debug::SwLog(Daemon_Find, "null thread", 0);
      return threads_.end();
   }

   return threads_.find(thread);
}

//------------------------------------------------------------------------------

void Daemon::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Daemon_ThreadCreated = "Daemon.ThreadCreated";

void Daemon::ThreadCreated(Thread* thread)
{
   Debug::ft(Daemon_ThreadCreated);

   threads_.insert(thread);
}

//------------------------------------------------------------------------------

fn_name Daemon_ThreadDeleted = "Daemon.ThreadDeleted";

void Daemon::ThreadDeleted(Thread* thread)
{
   Debug::ft(Daemon_ThreadDeleted);

   //  This does not immediately recreate the deleted thread.  We only create
   //  threads when invoked by InitThread, which is not the case here.  So we
   //  must ask InitThread to invoke us.  During a restart, however, threads
   //  often exit, so there is no point doing this, and InitThread will soon
   //  invoke our Startup function so that we can create threads.
   //
   auto item = Find(thread);

   if(item != threads_.end())
   {
      threads_.erase(item);
      if(Restart::GetStatus() != Running) return;
      Singleton< InitThread >::Instance()->Interrupt(InitThread::RecreateMask);
   }
}
}
