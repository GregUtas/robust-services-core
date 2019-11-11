//==============================================================================
//
//  DaemonRegistry.cpp
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
#include "DaemonRegistry.h"
#include <ostream>
#include <string>
#include "Daemon.h"
#include "Debug.h"
#include "Formatters.h"
#include "Thread.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name DaemonRegistry_ctor = "DaemonRegistry.ctor";

DaemonRegistry::DaemonRegistry()
{
   Debug::ft(DaemonRegistry_ctor);

   daemons_.Init(Thread::MaxId, Daemon::CellDiff(), MemProt);
}

//------------------------------------------------------------------------------

fn_name DaemonRegistry_dtor = "DaemonRegistry.dtor";

DaemonRegistry::~DaemonRegistry()
{
   Debug::ft(DaemonRegistry_dtor);
}

//------------------------------------------------------------------------------

fn_name DaemonRegistry_BindDaemon = "DaemonRegistry.BindDaemon";

bool DaemonRegistry::BindDaemon(Daemon& daemon)
{
   Debug::ft(DaemonRegistry_BindDaemon);

   if(FindDaemon(daemon.Name().c_str()) != nullptr)
   {
      Debug::SwLog(DaemonRegistry_BindDaemon, daemon.Name(), 0);
      return false;
   }

   return daemons_.Insert(daemon);
}

//------------------------------------------------------------------------------

void DaemonRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Permanent::Display(stream, prefix, options);

   stream << prefix << "daemons [id_t]" << CRLF;
   daemons_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

fn_name DaemonRegistry_FindDaemon = "DaemonRegistry.FindDaemon";

Daemon* DaemonRegistry::FindDaemon(fixed_string name) const
{
   Debug::ft(DaemonRegistry_FindDaemon);

   if(name == nullptr)
   {
      Debug::SwLog(DaemonRegistry_FindDaemon, "null pointer", 0);
      return nullptr;
   }

   for(auto d = daemons_.First(); d != nullptr; daemons_.Next(d))
   {
      if(d->Name() == name) return d;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void DaemonRegistry::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name DaemonRegistry_Startup = "DaemonRegistry.Startup";

void DaemonRegistry::Startup(RestartLevel level)
{
   Debug::ft(DaemonRegistry_Startup);

   for(auto d = daemons_.First(); d != nullptr; daemons_.Next(d))
   {
      d->Startup(level);
   }
}

//------------------------------------------------------------------------------

fn_name DaemonRegistry_UnbindDaemon = "DaemonRegistry.UnbindDaemon";

void DaemonRegistry::UnbindDaemon(Daemon& daemon)
{
   Debug::ft(DaemonRegistry_UnbindDaemon);

   daemons_.Erase(daemon);
}
}
