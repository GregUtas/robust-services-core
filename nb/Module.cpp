//==============================================================================
//
//  Module.cpp
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
#include "Module.h"
#include <ostream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "ModuleRegistry.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const ModuleId Module::MaxId = 4000;

//------------------------------------------------------------------------------

fn_name Module_ctor = "Module.ctor";

Module::Module(ModuleId mid)
{
   Debug::ft(Module_ctor);

   mid_.SetId(mid);
   Singleton< ModuleRegistry >::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

fn_name Module_dtor = "Module.dtor";

Module::~Module()
{
   Debug::ft(Module_dtor);

   Singleton< ModuleRegistry >::Instance()->UnbindModule(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t Module::CellDiff()
{
   int local;
   auto fake = reinterpret_cast< const Module* >(&local);
   return ptrdiff(&fake->mid_, fake);
}

//------------------------------------------------------------------------------

fn_name Module_Dependencies = "Module.Dependencies";

ModuleId* Module::Dependencies(size_t& count) const
{
   Debug::ft(Module_Dependencies);

   count = 0;
   return nullptr;
}

//------------------------------------------------------------------------------

void Module::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "mid : " << mid_.to_str() << CRLF;
}

//------------------------------------------------------------------------------

void Module::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name Module_Shutdown = "Module.Shutdown";

void Module::Shutdown(RestartLevel level)
{
   Debug::ft(Module_Shutdown);
}

//------------------------------------------------------------------------------

fn_name Module_Startup = "Module.Startup";

void Module::Startup(RestartLevel level)
{
   Debug::ft(Module_Startup);
}
}
