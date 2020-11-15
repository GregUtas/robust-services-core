//==============================================================================
//
//  ClassRegistry.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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
#include "ClassRegistry.h"
#include <ostream>
#include <string>
#include "Class.h"
#include "Debug.h"
#include "Formatters.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
ClassRegistry::ClassRegistry()
{
   Debug::ft("ClassRegistry.ctor");

   classes_.Init(MaxClassId, Class::CellDiff(), MemImmutable);
}

//------------------------------------------------------------------------------

fn_name ClassRegistry_dtor = "ClassRegistry.dtor";

ClassRegistry::~ClassRegistry()
{
   Debug::ftnt(ClassRegistry_dtor);

   Debug::SwLog(ClassRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

bool ClassRegistry::BindClass(Class& cls)
{
   Debug::ft("ClassRegistry.BindClass");

   return classes_.Insert(cls);
}

//------------------------------------------------------------------------------

void ClassRegistry::ClaimBlocks()
{
   Debug::ft("ClassRegistry.ClaimBlocks");

   for(auto c = classes_.First(); c != nullptr; classes_.Next(c))
   {
      c->ClaimBlocks();
   }
}

//------------------------------------------------------------------------------

void ClassRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "classes [Object::ClassId]" << CRLF;
   classes_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

Class* ClassRegistry::Lookup(ClassId cid) const
{
   return classes_.At(cid);
}

//------------------------------------------------------------------------------

void ClassRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void ClassRegistry::Shutdown(RestartLevel level)
{
   Debug::ft("ClassRegistry.Shutdown");

   for(auto c = classes_.First(); c != nullptr; classes_.Next(c))
   {
      c->Shutdown(level);
   }
}

//------------------------------------------------------------------------------

void ClassRegistry::Startup(RestartLevel level)
{
   Debug::ft("ClassRegistry.Startup");

   for(auto c = classes_.First(); c != nullptr; classes_.Next(c))
   {
      c->Startup(level);
   }
}

//------------------------------------------------------------------------------

void ClassRegistry::UnbindClass(Class& cls)
{
   Debug::ftnt("ClassRegistry.UnbindClass");

   classes_.Erase(cls);
}
}
