//==============================================================================
//
//  Module.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "Module.h"
#include <cstdint>
#include <ostream>
#include "Algorithms.h"
#include "Debug.h"
#include "ModuleRegistry.h"
#include "Restart.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
const ModuleId Module::MaxId = 255;

//------------------------------------------------------------------------------

fn_name Module_ctor = "Module.ctor";

Module::Module(c_string symbol) :
   symbol_(symbol),
   enabled_(false)
{
   Debug::ft(Module_ctor);

   if(symbol_.size() > 6)
   {
      string expl = "module symbol \"" + symbol_ + "\" too long";
      Debug::SwLog(Module_ctor, expl, symbol_.size() - 6);
   }
}

//------------------------------------------------------------------------------

fn_name Module_dtor = "Module.dtor";

Module::~Module()
{
   Debug::ftnt(Module_dtor);

   Debug::SwLog(Module_dtor, UnexpectedInvocation, 0);
   Singleton<ModuleRegistry>::Extant()->UnbindModule(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t Module::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast<const Module*>(&local);
   return ptrdiff(&fake->mid_, fake);
}

//------------------------------------------------------------------------------

void Module::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "mid     : " << mid_.to_str() << CRLF;
   stream << prefix << "symbol  : " << symbol_ << CRLF;
   stream << prefix << "enabled : " << enabled_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name Module_Enable = "Module.Enable";

void Module::Enable()
{
   Debug::ft(Module_Enable);

   if(Restart::GetLevel() == RestartReboot)
      enabled_ = true;
   else
      Debug::SwLog(Module_Enable, "reboot not in progress", 0);
}

//------------------------------------------------------------------------------

void Module::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void Module::Shutdown(RestartLevel level)
{
   Debug::ft("Module.Shutdown");
}

//------------------------------------------------------------------------------

void Module::Startup(RestartLevel level)
{
   Debug::ft("Module.Startup");
}
}
