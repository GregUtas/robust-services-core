//==============================================================================
//
//  MutexRegistry.cpp
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
#include "MutexRegistry.h"
#include <cstddef>
#include <ostream>
#include "Debug.h"
#include "Formatters.h"
#include "SysMutex.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const id_t MutexRegistry::MaxMutexes = 250;

//------------------------------------------------------------------------------

fn_name MutexRegistry_ctor = "MutexRegistry.ctor";

MutexRegistry::MutexRegistry()
{
   Debug::ft(MutexRegistry_ctor);

   mutexes_.Init(MaxMutexes, SysMutex::CellDiff(), MemPerm);
}

//------------------------------------------------------------------------------

fn_name MutexRegistry_dtor = "MutexRegistry.dtor";

MutexRegistry::~MutexRegistry()
{
   Debug::ft(MutexRegistry_dtor);
}

//------------------------------------------------------------------------------

fn_name MutexRegistry_BindMutex = "MutexRegistry.BindMutex";

bool MutexRegistry::BindMutex(SysMutex& mutex)
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
   auto lead = prefix + spaces(2);

   stream << prefix << "mutexes [id_t]" << CRLF;
   mutexes_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

fn_name MutexRegistry_Find = "MutexRegistry.Find";

SysMutex* MutexRegistry::Find(const std::string& name) const
{
   Debug::ft(MutexRegistry_Find);

   auto key = strUpper(name);

   for(auto m = mutexes_.First(); m != nullptr; mutexes_.Next(m))
   {
      if(strUpper(m->Name()) == key) return m;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void MutexRegistry::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name MutexRegistry_UnbindMutex = "MutexRegistry.UnbindMutex";

void MutexRegistry::UnbindMutex(SysMutex& mutex)
{
   Debug::ft(MutexRegistry_UnbindMutex);

   mutexes_.Erase(mutex);
}
}
