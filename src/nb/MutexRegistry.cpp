//==============================================================================
//
//  MutexRegistry.cpp
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
#include "MutexRegistry.h"
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <new>
#include <sstream>
#include "Debug.h"
#include "Formatters.h"
#include "Log.h"
#include "Mutex.h"
#include "NbLogs.h"
#include "SysThread.h"
#include "SysTypes.h"
#include "Thread.h"

using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
//> The maximum number of mutexes.
//
constexpr id_t MaxMutexes = 250;

//------------------------------------------------------------------------------

MutexRegistry::MutexRegistry()
{
   Debug::ft("MutexRegistry.ctor");

   mutexes_.Init(MaxMutexes, Mutex::CellDiff(), MemPermanent);
}

//------------------------------------------------------------------------------

fn_name MutexRegistry_dtor = "MutexRegistry.dtor";

MutexRegistry::~MutexRegistry()
{
   Debug::ftnt(MutexRegistry_dtor);

   Debug::SwLog(MutexRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

void MutexRegistry::Abandon() const
{
   Debug::ftnt("MutexRegistry.Abandon");

   size_t count = 0;
   auto nid = SysThread::RunningThreadId();

   for(auto m = mutexes_.First(); m != nullptr; mutexes_.Next(m))
   {
      if(m->OwnerId() == nid)
      {
         m->Release(true);
         ++count;
      }
   }

   if(count > 0)
   {
      auto log = Log::Create(ThreadLogGroup, ThreadMutexesReleased);

      if(log != nullptr)
      {
         auto thr = Thread::RunningThread(std::nothrow);

         if(thr != nullptr)
            *log << Log::Tab << "thread=" << thr->to_str() << CRLF;
         else
            *log << Log::Tab << "nid=" << std::hex << nid << std::dec << CRLF;
         *log << Log::Tab << "mutexes=" << count;
         Log::Submit(log);
      }
   }
}

//------------------------------------------------------------------------------

fn_name MutexRegistry_BindMutex = "MutexRegistry.BindMutex";

bool MutexRegistry::BindMutex(Mutex& mutex)
{
   Debug::ft(MutexRegistry_BindMutex);

   if(Find(mutex.Name()) != nullptr)
   {
      Debug::SwLog(MutexRegistry_BindMutex, mutex.Name(), 0);
      return false;
   }

   return mutexes_.Insert(mutex);
}

//------------------------------------------------------------------------------

void MutexRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Permanent::Display(stream, prefix, options);

   stream << prefix << "mutexes [id_t]" << CRLF;
   mutexes_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

Mutex* MutexRegistry::Find(const std::string& name) const
{
   Debug::ft("MutexRegistry.Find");

   auto key = strUpper(name);

   for(auto m = mutexes_.First(); m != nullptr; mutexes_.Next(m))
   {
      if(strUpper(m->Name()) == key) return m;
   }

   return nullptr;
}

//------------------------------------------------------------------------------
//
//                         | 2..22                    . 2..<nid>

//------------------------------------------------------------------------------

void MutexRegistry::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}
fixed_string MutexHeader = "Id  Name                  Tid  NativeId";

//------------------------------------------------------------------------------

void MutexRegistry::Summarize(ostream& stream) const
{
   stream << MutexHeader << CRLF;

   for(auto m = mutexes_.First(); m != nullptr; mutexes_.Next(m))
   {
      stream << setw(2) << m->Mid();
      stream << spaces(2) << std::left << setw(22) << m->Name();
      stream << SPACE << std::right << setw(2);
      auto owner = m->Owner();
      if(owner != nullptr)
         stream << owner->Tid();
      else
         stream << NIL_ID;
      auto nid = m->OwnerId();
      stream << spaces(2) << std::hex << (nid & UINT32_MAX) << std::dec << CRLF;
   }
}

//------------------------------------------------------------------------------

void MutexRegistry::UnbindMutex(Mutex& mutex)
{
   Debug::ftnt("MutexRegistry.UnbindMutex");

   mutexes_.Erase(mutex);
}
}
